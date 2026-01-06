/*
 * ADC_FAST - TRUE 8000Hz mit ADC_ETC + DMA
 * Alle 4 Achsen komplett Hardware-gesteuert!
 */

#include "adc_fast.h"
#include "imxrt.h"
#include "config.h"
#include "calibration.h"
#include "eeprom_config.h"
#include "controller_config.h"  // Für ADC-Einstellungen und Deadzone/Clamp-Einstellungen aus Config
#include "DMAChannel.h"
#include <math.h>

#ifndef DEFAULT_RADIAL_DEADZONE_PERCENT
#define DEFAULT_RADIAL_DEADZONE_PERCENT 0
#endif

static constexpr float PI_F = 3.14159265358979323846f;
static constexpr float TWO_PI_F = 6.28318530717958647692f;
static constexpr float SECTOR_SIZE_F = PI_F / 4.0f;


ADC_FAST FastADC;

// ===== ADC_ETC RESULT BUFFERS (von Hardware gefüllt!) =====
// ADC_ETC schreibt Ergebnisse in eigene Result-Register
// DMA kopiert sie dann in unsere Buffers

static volatile uint16_t adc_results[4] __attribute__((aligned(32)));
// [0] = L_X (Pin 24, ADC1 Ch1)
// [1] = R_X (Pin 25, ADC1 Ch2)
// [2] = L_Y (Pin 39, ADC2 Ch2)
// [3] = R_Y (Pin 38, ADC2 Ch1)

static DMAChannel dma_adc_etc;

// Kalibrierung
struct AxisCalibration {
    uint16_t min;
    uint16_t max;
    uint16_t center;
    bool is_calibrated;  // Ist diese Achse fertig kalibriert?
};

static AxisCalibration cal_lx = {0, 4095, 2048, false};
static AxisCalibration cal_ly = {0, 4095, 2048, false};
static AxisCalibration cal_rx = {0, 4095, 2048, false};
static AxisCalibration cal_ry = {0, 4095, 2048, false};

static inline uint8_t map_axis_u8(uint16_t raw, AxisCalibration &cal);

static ManualCalibration manual_calibration;
static bool manual_calibration_loaded = false;
static float manual_left_radius_scale = 1.0f;
static float manual_right_radius_scale = 1.0f;
static uint32_t manual_settle_counter = 0;
static bool manual_settle_active = false;

// Globale Flag: Sind alle Achsen kalibriert? (Performance-Optimierung!)
static bool all_axes_calibrated = false;

enum class AutoCalibrationMode : uint8_t {
    Live,
    LoadAndLock,
    LiveAndStore
};

static AutoCalibration auto_calibration_data;
static bool auto_calibration_loaded = false;
static bool auto_calibration_save_pending = false;
static AutoCalibrationMode requested_auto_mode = AutoCalibrationMode::Live;
static bool stick_profile_loaded = false;
static bool outer_clamp_enabled = false;
static uint8_t radial_deadzone_radius = 0;  // Radial Deadzone Radius direkt in Units (0-127), Standard: 0 (AUS) ✅ AKTIV!
static uint8_t inline_deadzone_radius = 4;  // Inline Deadzone Radius direkt in Units (0-20), Standard: 4 Units (wie Backup) ✅ AKTIV!

// Validiert Prozent in 0.1% Schritten (0-200 = 0.0-20.0%)
// clamp_percent_tenths() entfernt - nicht mehr verwendet (Legacy, durch inline_deadzone_radius ersetzt)

static void reset_axis_calibration_state() {
    cal_lx.is_calibrated = false;
    cal_ly.is_calibrated = false;
    cal_rx.is_calibrated = false;
    cal_ry.is_calibrated = false;
    all_axes_calibrated = false;
}

static void reset_auto_calibration_runtime() {
    auto_calibration_save_pending = false;
}

static bool validate_auto_axis(const AxisAutoCalibration &axis) {
    if (axis.min > ADC_MAX_VALUE) return false;
    if (axis.min >= axis.max) return false;
    if (axis.center > ADC_MAX_VALUE) return false;
    if (axis.center <= axis.min || axis.center >= axis.max) return false;
    if (axis.max > ADC_MAX_VALUE) return false;
    return true;
}

static bool load_auto_calibration_data() {
    if (!loadAutoCalibration(auto_calibration_data)) {
        memset(&auto_calibration_data, 0, sizeof(auto_calibration_data));
        auto_calibration_loaded = false;
        return false;
    }

    bool valid =
        validate_auto_axis(auto_calibration_data.axis[0]) &&
        validate_auto_axis(auto_calibration_data.axis[1]) &&
        validate_auto_axis(auto_calibration_data.axis[2]) &&
        validate_auto_axis(auto_calibration_data.axis[3]);

    if (!valid) {
        memset(&auto_calibration_data, 0, sizeof(auto_calibration_data));
        auto_calibration_loaded = false;
        return false;
    }

    auto_calibration_loaded = true;
    return true;
}

static void apply_auto_calibration_locked(const AutoCalibration &cal) {
    cal_lx.min = cal.axis[0].min;
    cal_lx.max = cal.axis[0].max;
    cal_lx.center = cal.axis[0].center;
    cal_lx.is_calibrated = true;

    cal_ly.min = cal.axis[1].min;
    cal_ly.max = cal.axis[1].max;
    cal_ly.center = cal.axis[1].center;
    cal_ly.is_calibrated = true;

    cal_rx.min = cal.axis[2].min;
    cal_rx.max = cal.axis[2].max;
    cal_rx.center = cal.axis[2].center;
    cal_rx.is_calibrated = true;

    cal_ry.min = cal.axis[3].min;
    cal_ry.max = cal.axis[3].max;
    cal_ry.center = cal.axis[3].center;
    cal_ry.is_calibrated = true;

    all_axes_calibrated = true;
    auto_calibration_save_pending = false;
}

static void capture_current_auto_calibration(AutoCalibration &cal) {
    cal.magic = AUTO_CAL_MAGIC;
    cal.axis[0].min = cal_lx.min;
    cal.axis[0].max = cal_lx.max;
    cal.axis[0].center = cal_lx.center;

    cal.axis[1].min = cal_ly.min;
    cal.axis[1].max = cal_ly.max;
    cal.axis[1].center = cal_ly.center;

    cal.axis[2].min = cal_rx.min;
    cal.axis[2].max = cal_rx.max;
    cal.axis[2].center = cal_rx.center;

    cal.axis[3].min = cal_ry.min;
    cal.axis[3].max = cal_ry.max;
    cal.axis[3].center = cal_ry.center;
}

static void persist_stick_profile_state() {
    StickProfileConfig cfg;
    cfg.magic = STICK_PROFILE_MAGIC;
    cfg.outer_clamp_enabled = outer_clamp_enabled ? 1 : 0;
    cfg.center_clamp_percent_tenths = 0;  // Nicht mehr verwendet (Legacy für EEPROM-Kompatibilität)
    cfg.radial_deadzone_percent_tenths = 0;  // Legacy für EEPROM-Kompatibilität (nicht mehr verwendet)
    saveStickProfileConfig(cfg);
}

static void load_stick_profile_state() {
    if (stick_profile_loaded) {
        return;
    }
    
    // Lade Werte aus Controller Runtime Config (zentrale Konfiguration!)
    // Diese Werte kommen aus controller_config.cpp (CONFIG_xxx defines) oder EEPROM
    outer_clamp_enabled = g_controller_config.get_outer_clamp_enabled_internal();
    radial_deadzone_radius = g_controller_config.get_radial_deadzone_radius_internal();
    inline_deadzone_radius = g_controller_config.get_inline_deadzone_radius_internal();
    
    // Legacy: Lade auch aus EEPROM StickProfileConfig (für Rückwärtskompatibilität)
    // Wenn EEPROM vorhanden ist, überschreibt es die Config-Werte
    StickProfileConfig cfg;
    if (loadStickProfileConfig(cfg)) {
        outer_clamp_enabled = cfg.outer_clamp_enabled != 0;
        // radial_deadzone_percent_tenths ist deprecated (nicht mehr verwendet)
        // inline_deadzone_radius ist nicht in StickProfileConfig, bleibt von Config
        // center_clamp_percent_tenths ist deprecated (nicht mehr verwendet)
    }
    
    stick_profile_loaded = true;
}

static void update_manual_scale_factors() {
    if (!manual_calibration_loaded) {
        manual_left_radius_scale = 1.0f;
        manual_right_radius_scale = 1.0f;
        return;
    }

    uint16_t left_max = 0;
    uint16_t right_max = 0;

    for (uint8_t i = 0; i < 8; i++) {
        if (manual_calibration.left_radius[i] > left_max) {
            left_max = manual_calibration.left_radius[i];
        }
        if (manual_calibration.right_radius[i] > right_max) {
            right_max = manual_calibration.right_radius[i];
        }
    }

    if (left_max < 10) left_max = 127;
    if (right_max < 10) right_max = 127;

    manual_left_radius_scale = 127.0f / (float)left_max;
    manual_right_radius_scale = 127.0f / (float)right_max;

    if (manual_left_radius_scale < 0.5f) manual_left_radius_scale = 0.5f;
    if (manual_left_radius_scale > 2.0f) manual_left_radius_scale = 2.0f;
    if (manual_right_radius_scale < 0.5f) manual_right_radius_scale = 0.5f;
    if (manual_right_radius_scale > 2.0f) manual_right_radius_scale = 2.0f;
}

static void load_manual_calibration_state() {
    if (loadManualCalibration(manual_calibration)) {
        manual_calibration_loaded = true;
        update_manual_scale_factors();
        manual_settle_counter = 0;
        manual_settle_active = true;
    } else {
        manual_calibration_loaded = false;
        manual_left_radius_scale = 1.0f;
        manual_right_radius_scale = 1.0f;
        memset(&manual_calibration, 0, sizeof(manual_calibration));
        manual_settle_counter = 0;
        manual_settle_active = false;
    }
}

// PIT Timer für ADC_ETC Trigger
static void setup_pit_timer() {
    // Enable PIT Clock
    CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);

    // Enable PIT module
    PIT_MCR = 0;

    // Timer 0: 8 kHz (125µs)
    // Bus Clock = 150 MHz
    // 150,000,000 / 8,000 = 18,750
    PIT_LDVAL0 = 18749;
    PIT_TCTRL0 = PIT_TCTRL_TEN;  // Enable
}

// XBAR: Route PIT0 → ADC_ETC
static void setup_xbar() {
    // Enable XBAR clock
    CCM_CCGR2 |= CCM_CCGR2_XBAR1(CCM_CCGR_ON);

    // XBAR1_OUT17 = ADC_ETC Trigger (laut Reference Manual)
    // XBAR1_IN 106 = PIT Trigger 0

    // XBAR Select Register (Output 17 / 2 = Register 8)
    // Bits für Output 17: [23:16]
    XBARA1_SEL8 = (XBARA1_SEL8 & 0xFF00FFFF) | (106 << 16);
}

// ADC_ETC Setup
static void setup_adc_etc() {
    // ADC_ETC hat kein eigenes Clock-Gate - läuft mit ADC1/ADC2 Clocks!
    // Stelle sicher dass ADC Clocks aktiviert sind
    CCM_CCGR1 |= CCM_CCGR1_ADC1(CCM_CCGR_ON);
    CCM_CCGR1 |= CCM_CCGR1_ADC2(CCM_CCGR_ON);

    // Software Reset
    ADC_ETC_CTRL = ADC_ETC_CTRL_SOFTRST;
    ADC_ETC_CTRL = 0;

    // ADC_ETC Global Config
    ADC_ETC_CTRL = ADC_ETC_CTRL_TSC_BYPASS |  // Bypass touchscreen
                   ADC_ETC_CTRL_DMA_MODE_SEL; // DMA mode

    // === TRIGGER 0: ADC1 Chain (L_X → R_X) ===

    // Trigger Control: External trigger from XBAR, chain length 2
    ADC_ETC_TRIG0_CTRL = ADC_ETC_TRIG_CTRL_TRIG_PRIORITY(0) |
                         ADC_ETC_TRIG_CTRL_SYNC_MODE |
                         (1 << 16);  // Chain length = 2 (0-indexed: 1 = 2 conversions)

    // Chain 0: L_X (ADC1, Channel 1)
    ADC_ETC_TRIG0_CHAIN_1_0 =
        (1 << 0) |   // Channel Select = 1
        (1 << 4) |   // HWTS = ADC1
        (1 << 8);    // B2B = back-to-back enable

    // Chain 1: R_X (ADC1, Channel 2)
    ADC_ETC_TRIG0_CHAIN_1_0 |=
        (2 << 16) |  // Channel Select = 2
        (1 << 20) |  // HWTS = ADC1
        (1 << 24) |  // B2B = enable
        (1 << 28);   // IE = Interrupt enable (triggers DMA)

    // === TRIGGER 1: ADC2 Chain (L_Y → R_Y) ===

    ADC_ETC_TRIG1_CTRL = ADC_ETC_TRIG_CTRL_TRIG_PRIORITY(0) |
                         ADC_ETC_TRIG_CTRL_SYNC_MODE |
                         (1 << 16);  // Chain length = 2

    // Chain 0: L_Y (ADC2, Channel 2, Pin 39)
    ADC_ETC_TRIG1_CHAIN_1_0 =
        (2 << 0) |   // Channel Select = 2
        (2 << 4) |   // HWTS = ADC2
        (1 << 8);    // B2B = enable

    // Chain 1: R_Y (ADC2, Channel 1, Pin 38)
    ADC_ETC_TRIG1_CHAIN_1_0 |=
        (1 << 16) |  // Channel Select = 1
        (2 << 20) |  // HWTS = ADC2
        (1 << 24) |  // B2B = enable
        (1 << 28);   // IE = Interrupt enable

    // Enable DMA for both triggers
    ADC_ETC_DMA_CTRL = (1 << 0) |  // TRIG0 DMA enable
                       (1 << 16);  // TRIG1 DMA enable
}

// ADC Hardware Setup für ADC_ETC
// Läd ADC-Einstellungen aus g_controller_config (konfigurierbar!)
static void setup_adc_for_etc() {
    // ===== ADC-EINSTELLUNGEN AUS CONFIG LADEN =====
    uint8_t adc_resolution = g_controller_config.get_adc_resolution_internal();
    uint8_t adc_averaging = g_controller_config.get_adc_averaging_internal();
    // TODO: adc_clock_mhz könnte für Clock-Divider verwendet werden, aber aktuell nutzen wir fest 150 MHz
    // uint8_t adc_clock_mhz = g_controller_config.get_adc_clock_mhz_internal();  // Nicht verwendet (fest 150 MHz)
    uint8_t adc_sample_time = g_controller_config.get_adc_sample_time_internal();
    
    // ===== ADC RESOLUTION =====
    analogReadResolution(adc_resolution);
    
    // ===== ADC AVERAGING (konvertieren: 0=1x, 2=4x, 3=8x, 4=16x, 5=32x) =====
    uint8_t averaging_value = 1;  // Default: 1x
    switch(adc_averaging) {
        case 0: averaging_value = 1; break;   // 1x
        case 2: averaging_value = 4; break;   // 4x
        case 3: averaging_value = 8; break;   // 8x
        case 4: averaging_value = 16; break;  // 16x
        case 5: averaging_value = 32; break;  // 32x
        default: averaging_value = 8; break;  // Default: 8x
    }
    analogReadAveraging(averaging_value);

    // ===== ADC CONFIGURATION REGISTER =====
    uint32_t cfg = ADC_CFG_ADHSC;  // High speed
    
    // Clock Source & Divider (für adc_clock_mhz)
    // IPG Clock = 150 MHz (Standard), Divider /1 = 150 MHz
    // TODO: Clock-Divider könnte konfigurierbar gemacht werden, aber Standard 150 MHz ist optimal
    cfg &= ~(3 << 0); cfg |= (0 << 0);  // IPG clock
    cfg &= ~(3 << 5); cfg |= (0 << 5);  // Divider /1 = 150 MHz (MAXIMUM für TMR!)
    
    // Sample Time: 0 = SHORT, 1 = LONG
    if (adc_sample_time == 1) {
        cfg &= ~(1 << 4);  // LONG sample time
    } else {
        cfg |= (1 << 4);   // SHORT sample time (optimal für TMR low-impedance Sticks!)
    }
    
    // Resolution: 12-bit, 10-bit, oder 8-bit
    cfg &= ~(3 << 2);  // Bits löschen
    if (adc_resolution == 10) {
        cfg |= (2 << 2);   // 10-bit mode
    } else if (adc_resolution == 8) {
        cfg |= (0 << 2);   // 8-bit mode
    } else {
        cfg |= (1 << 2);   // 12-bit mode (Standard)
    }

    // Hardware Averaging (Bits 15:14)
    // 0=1x (00), 2=4x (01), 3=8x (10), 4=16x (11), 5=32x (nicht direkt unterstützt, nutze 16x)
    cfg &= ~(3 << 14);  // Bits 15:14 löschen
    switch(adc_averaging) {
        case 0: cfg |= (0 << 14); break;  // 1x (00)
        case 2: cfg |= (1 << 14); break;  // 4x (01)
        case 3: cfg |= (2 << 14); break;  // 8x (10) - Standard
        case 4: cfg |= (3 << 14); break;  // 16x (11)
        case 5: cfg |= (3 << 14); break;  // 32x nicht direkt, nutze 16x
        default: cfg |= (2 << 14); break; // Default: 8x
    }

    ADC1_CFG = cfg;
    ADC2_CFG = cfg;

    // Hardware Averaging aktivieren (nur wenn > 1x)
    if (adc_averaging > 0) {
        ADC1_GC |= ADC_GC_AVGE;  // Averaging AN
        ADC2_GC |= ADC_GC_AVGE;  // Averaging AN
    } else {
        ADC1_GC &= ~ADC_GC_AVGE; // Averaging AUS
        ADC2_GC &= ~ADC_GC_AVGE; // Averaging AUS
    }

    // DEAKTIVIERT: Hardware trigger mode (ADC_ETC funktioniert noch nicht)
    // ADC1_HC0 = 16;
    // ADC2_HC0 = 16;
}

// DMA Setup für ADC_ETC Results
static void setup_dma() {
    // DMA kopiert ADC_ETC Result Register → unser Buffer
    // Trigger 0 hat 2 Results (L_X, R_X) in TRIG0_RESULT_1_0
    // Trigger 1 hat 2 Results (L_Y, R_Y) in TRIG1_RESULT_1_0

    // === SCATTER-GATHER DMA für BEIDE Triggers! ===
    // Major Loop kopiert 4x 16-bit (alle 4 Achsen)

    dma_adc_etc.begin(true);

    // Source: ADC_ETC TRIG0_RESULT_1_0 (L_X + R_X)
    dma_adc_etc.TCD->SADDR = &ADC_ETC_TRIG0_RESULT_1_0;
    dma_adc_etc.TCD->SOFF = 2;  // 16-bit increment
    dma_adc_etc.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);  // 16-bit
    dma_adc_etc.TCD->NBYTES_MLNO = 2;  // 1x 16-bit pro Minor Loop

    // Nach 2x Minor Loop (L_X, R_X): Jump zu TRIG1_RESULT_1_0
    dma_adc_etc.TCD->SLAST = ((uint32_t)&ADC_ETC_TRIG1_RESULT_1_0 - (uint32_t)&ADC_ETC_TRIG0_RESULT_1_0) - 4;

    // Destination: adc_results[0..3]
    dma_adc_etc.TCD->DADDR = (void*)&adc_results[0];
    dma_adc_etc.TCD->DOFF = 2;  // 16-bit increment

    // Major Loop: 4 Transfers (L_X, R_X, L_Y, R_Y)
    dma_adc_etc.TCD->CITER_ELINKNO = 4;
    dma_adc_etc.TCD->BITER_ELINKNO = 4;

    // Nach Major Loop: Zurück zum Start
    dma_adc_etc.TCD->DLASTSGA = -8;  // -4x 16-bit
    dma_adc_etc.TCD->CSR = 0;  // Continuous

    // Trigger source: ADC_ETC Done (TRIG1 kommt nach TRIG0!)
    dma_adc_etc.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC_ETC);
    dma_adc_etc.enable();
}

void ADC_FAST::begin() {
    // LED blinkt 3x = Setup läuft
    for(int i=0; i<3; i++) {
        digitalWrite(13, HIGH);
        delay(150);
        digitalWrite(13, LOW);
        delay(150);
    }

    delay(500);

    reset_axis_calibration_state();
    reset_auto_calibration_runtime();

    // === HARDWARE SETUP ===
    setup_pit_timer();
    setup_xbar();
    setup_adc_for_etc();
    setup_adc_etc();
    setup_dma();

    // === CENTER-KALIBRIERUNG (OPTIMIERT FÜR MAXIMALE STABILITÄT) ===
    // Einfacher Mittelwert über viele Samples - gute Balance zwischen Präzision und Toleranz!
    // Warte kurz bis ADCs stabil sind
    delay(100);

    const int NUM_SAMPLES = 1024;  // Gute Balance: Präzise aber nicht zu präzise (toleranter gegen Schwankungen)
    
    uint32_t lx_sum = 0, ly_sum = 0, rx_sum = 0, ry_sum = 0;
    
    // Sammle Samples mit langen Delays für maximale Stabilität
    // Längere Delays geben dem Stick Zeit, sich zu beruhigen (wie im STABLE Backup)
    for (int i = 0; i < NUM_SAMPLES; i++) {
        // L_X + L_Y parallel
        ADC1_HC0 = 1; ADC2_HC0 = 2;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        lx_sum += ADC1_R0 & 0xFFF;
        ly_sum += ADC2_R0 & 0xFFF;

        // R_X + R_Y parallel
        ADC1_HC0 = 2; ADC2_HC0 = 1;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        rx_sum += ADC1_R0 & 0xFFF;
        ry_sum += ADC2_R0 & 0xFFF;
        
        // Lange Pause für maximale Stabilität (500µs wie im STABLE Backup)
        // Gibt dem Stick mehr Zeit, sich zu beruhigen → stabilerer Center-Wert
        delayMicroseconds(500);
    }
    
    // Einfacher Mittelwert = Center!
    cal_lx.center = lx_sum / NUM_SAMPLES;
    cal_ly.center = ly_sum / NUM_SAMPLES;
    cal_rx.center = rx_sum / NUM_SAMPLES;
    cal_ry.center = ry_sum / NUM_SAMPLES;

    // Start mit ±10 bei 12-bit
    cal_lx.min = (cal_lx.center > 10) ? cal_lx.center - 10 : 0;
    cal_lx.max = (cal_lx.center < 4085) ? cal_lx.center + 10 : 4095;
    cal_ly.min = (cal_ly.center > 10) ? cal_ly.center - 10 : 0;
    cal_ly.max = (cal_ly.center < 4085) ? cal_ly.center + 10 : 4095;
    cal_rx.min = (cal_rx.center > 10) ? cal_rx.center - 10 : 0;
    cal_rx.max = (cal_rx.center < 4085) ? cal_rx.center + 10 : 4095;
    cal_ry.min = (cal_ry.center > 10) ? cal_ry.center - 10 : 0;
    cal_ry.max = (cal_ry.center < 4085) ? cal_ry.center + 10 : 4095;

    // LED kurz an = Bereit!
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);

    bool manual_load_requested = true;
    switch (requested_auto_mode) {
        case AutoCalibrationMode::LoadAndLock:
            manual_load_requested = false;
            if (load_auto_calibration_data()) {
                apply_auto_calibration_locked(auto_calibration_data);
            } else {
                auto_calibration_save_pending = true;
            }
            break;
        case AutoCalibrationMode::LiveAndStore:
            manual_load_requested = true;
            auto_calibration_save_pending = true;
            break;
        case AutoCalibrationMode::Live:
        default:
            manual_load_requested = true;
            load_auto_calibration_data();  // Nur preload, keine Anwendung
            auto_calibration_save_pending = false;
            break;
    }

    if (manual_load_requested) {
        load_manual_calibration_state();
    } else {
        clear_manual_state();  // Runtime zurücksetzen, EEPROM bleibt unangetastet
    }

    requested_auto_mode = AutoCalibrationMode::Live;
}

// Mapping-Funktion (12-bit ADC → signed)
static inline int16_t map_calibrated_12bit_signed(uint16_t raw, AxisCalibration &cal) {
    if (raw <= cal.min) return -128;
    if (raw >= cal.max) return 127;
    if (raw == cal.center) return 0;

    int32_t value;
    if (raw < cal.center) {
        uint32_t range = cal.center - cal.min;
        if (range == 0) return 0;
        value = -128 + ((uint32_t)(raw - cal.min) * 128) / range;
    } else {
        uint32_t range = cal.max - cal.center;
        if (range == 0) return 0;
        value = ((uint32_t)(raw - cal.center) * 127) / range;
    }

    if (value < -128) return -128;
    if (value > 127) return 127;
    return (int16_t)value;
}

// Quadrat → Kreis Mapping (radiale Normalisierung)
// percent_tenths: 0-200 = 0.0-20.0% (in 0.1% Schritten)
static inline void apply_radial_deadzone(int16_t *x, int16_t *y, uint8_t deadzone_radius) {
    if (deadzone_radius == 0) {
        return;
    }

    // Berechne Distanz vom Center
    int32_t dx = *x;
    int32_t dy = *y;

    // Radiale Distanz (approximiert mit max(|x|,|y|) + 0.5*min(|x|,|y|))
    int32_t ax = (dx < 0) ? -dx : dx;
    int32_t ay = (dy < 0) ? -dy : dy;
    int32_t dist = (ax > ay) ? (ax + (ay >> 1)) : (ay + (ax >> 1));

    const int32_t max_radius = 128;
    
    // Deadzone direkt in Units (0-127), keine Konvertierung mehr nötig!
    if (deadzone_radius >= max_radius) {
        *x = 0;
        *y = 0;
        return;
    }

    if (dist < deadzone_radius) {
        *x = 0;
        *y = 0;
        return;
    }

    // Skaliere auf 128 unter Berücksichtigung der Deadzone
    if (dist > max_radius) dist = max_radius;

    int32_t scale = ((dist - deadzone_radius) * max_radius) / (max_radius - deadzone_radius);
    *x = (dx * scale) / dist;
    *y = (dy * scale) / dist;
}

static inline uint8_t map_axis_u8(uint16_t raw, AxisCalibration &cal) {
    const uint16_t INREACH_MARGIN = 0;

    int32_t map_min = (int32_t)cal.min + INREACH_MARGIN;
    int32_t map_max = (int32_t)cal.max - INREACH_MARGIN;

    if (map_min >= cal.center) map_min = cal.min;
    if (map_max <= cal.center) map_max = cal.max;

    if (raw <= map_min) return 0;
    if (raw >= map_max) return 255;
    if (raw == cal.center) return 128;

    int32_t value;
    if (raw < cal.center) {
        int32_t range = cal.center - map_min;
        if (range == 0) return 128;
        value = ((int32_t)(raw - map_min) * 128) / range;
    } else {
        int32_t range = map_max - cal.center;
        if (range == 0) return 128;
        value = ((int32_t)(raw - cal.center) * 127) / range + 128;
    }

    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

static inline void clamp_to_unit_circle(int16_t &x, int16_t &y) {
    const float max_radius = 127.0f;
    const float max_radius_sq = max_radius * max_radius;

    float dx = (float)x;
    float dy = (float)y;
    float dist_sq = dx * dx + dy * dy;

    if (dist_sq <= max_radius_sq) {
        return;
    }

    float length = sqrtf(dist_sq);
    if (length == 0.0f) {
        return;
    }

    float ratio = max_radius / length;
    dx *= ratio;
    dy *= ratio;

    x = (int16_t)lrintf(dx);
    y = (int16_t)lrintf(dy);
}

static inline float safe_radius_value(const uint16_t radius_table[8], uint8_t idx) {
    uint16_t r = radius_table[idx];
    if (r < 10) {
        return 127.0f;
    }
    return (float)r;
}

static inline float compute_manual_target_radius(int16_t x, int16_t y, const uint16_t radius_table[8], float scale) {
    if (x == 0 && y == 0) {
        return 0.0f;
    }

    float angle = atan2f(-(float)y, (float)x);  // Y invertiert, damit N oben bleibt
    if (angle < 0.0f) {
        angle += TWO_PI_F;
    }

    float sector = angle / SECTOR_SIZE_F;
    float base_index = floorf(sector);
    uint8_t idx0 = ((int)base_index) & 0x7;
    uint8_t idx1 = (idx0 + 1) & 0x7;
    float t = sector - base_index;

    float r0 = safe_radius_value(radius_table, idx0);
    float r1 = safe_radius_value(radius_table, idx1);
    float interpolated = r0 + (r1 - r0) * t;

    if (interpolated < 10.0f) {
        interpolated = 127.0f;
    }

    return interpolated * scale;
}

static inline void apply_manual_radius(int16_t &x, int16_t &y, const uint16_t radius_table[8], float scale) {
    float target_radius = compute_manual_target_radius(x, y, radius_table, scale);
    if (target_radius <= 0.0f) {
        return;
    }

    float current_radius = sqrtf((float)x * (float)x + (float)y * (float)y);
    if (current_radius == 0.0f) {
        return;
    }

    if (fabsf(current_radius - target_radius) < 0.5f) {
        return;
    }

    float ratio = target_radius / current_radius;
    if (ratio > 2.0f) {
        ratio = 2.0f;
    } else if (ratio < 0.5f) {
        ratio = 0.5f;
    }

    float nx = (float)x * ratio;
    float ny = (float)y * ratio;

    if (nx > 127.0f) nx = 127.0f;
    if (nx < -128.0f) nx = -128.0f;
    if (ny > 127.0f) ny = 127.0f;
    if (ny < -128.0f) ny = -128.0f;

    x = (int16_t)lrintf(nx);
    y = (int16_t)lrintf(ny);
}

static inline void update_radius_table(uint16_t *radius_table, int16_t x, int16_t y) {
    if (x == 0 && y == 0) {
        return;
    }

    uint8_t dir = get_direction_8(x, y);
    if (dir > 7) {
        return;
    }

    float radius = sqrtf((float)x * (float)x + (float)y * (float)y);
    if (radius > 255.0f) {
        radius = 255.0f;
    }
    uint16_t radius_u = (uint16_t)(radius + 0.5f);

    if (radius_u > radius_table[dir]) {
        radius_table[dir] = radius_u;
    }
}

// Phase 1: Pre-Trigger ADC - Trigger erste 2 ADC früh (parallel zu Buttons)
void ADC_FAST::trigger_first_stick_pair() {
    // Erste parallele Messung: L_X (ADC1 Ch1) + L_Y (ADC2 Ch2)
    // Trigger, aber NICHT warten! CPU macht parallel Buttons lesen!
    ADC1_HC0 = 1;
    ADC2_HC0 = 2;
    // KEIN Warten hier - ADC konvertiert während CPU andere Sachen macht!
}

// ULTRA-LOW LATENCY: Erste beiden Sticks SOFORT triggern!
// OPTIMIERUNG: Trigger HC0 für beide ADCs am Anfang (während Buttons gelesen werden)
// Bewährte Methode: Nur HC0 verwenden, sequenziell triggern - funktioniert garantiert!
__attribute__((hot))
void ADC_FAST::trigger_all_sticks_now() {
    // Phase 1: Trigger erste beiden Sticks für beide ADCs GLEICHZEITIG (HC0):
    // L_X (ADC1 Ch1) + L_Y (ADC2 Ch2) - wie in der bewährten Methode!
    ADC1_HC0 = 1;  // ADC1 Ch1 = L_X (Pin 24)
    ADC2_HC0 = 2;  // ADC2 Ch2 = L_Y (Pin 39) - WICHTIG: Ch2, nicht Ch1!
    // KEIN Warten hier - ADC konvertiert während CPU andere Sachen macht!
}

void ADC_FAST::read_all_sticks(uint16_t *lx, uint16_t *ly, uint16_t *rx, uint16_t *ry) {
    // Zurück auf stabile DIREKTE ADC-REGISTER READS (bewährt)
    uint16_t lx_raw, ly_raw, rx_raw, ry_raw;

    // Phase 1: Warte auf erste beiden Sticks (HC0) - wurden bereits getriggert!
    // L_X + L_Y parallel (wie in der bewährten Methode)
    while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
    lx_raw = ADC1_R0 & 0xFFF;  // ADC1 HC0 = Ch1 = L_X
    ly_raw = ADC2_R0 & 0xFFF;  // ADC2 HC0 = Ch2 = L_Y

    // Phase 2: Trigger zweite beiden Sticks für beide ADCs GLEICHZEITIG (HC0 erneut!)
    // R_X + R_Y parallel (wie in der bewährten Methode)
    ADC1_HC0 = 2;  // ADC1 Ch2 = R_X (Pin 25)
    ADC2_HC0 = 1;  // ADC2 Ch1 = R_Y (Pin 38) - WICHTIG: Ch1, nicht Ch2!
    while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
    rx_raw = ADC1_R0 & 0xFFF;  // ADC1 HC0 (jetzt Ch2) = R_X
    ry_raw = ADC2_R0 & 0xFFF;  // ADC2 HC0 (jetzt Ch1) = R_Y

    // 12-bit Masken
    lx_raw &= 0xFFF;
    ly_raw &= 0xFFF;
    rx_raw &= 0xFFF;
    ry_raw &= 0xFFF;

    // ADAPTIVE KALIBRIERUNG: Per-Achse unabhängig!
    // Jede Achse kalibriert sich selbstständig, bis sie ihren vollen Bereich erreicht hat
    // Danach wird diese Achse gesperrt (is_calibrated = true)
    // WICHTIG: Jedes Frame updaten (keine Throttling), schnellste Konvergenz!

    // Performance-Optimierung: Sobald alle Achsen kalibriert sind, überspringen wir den ganzen Block!
    if (!all_axes_calibrated) {
        // Minimum Range-Threshold für "vollständig kalibriert"
        // Bei 12-bit ADC (0..4095): Range > 3500 ~85% Abdeckung
        const uint16_t MIN_CALIBRATED_RANGE = 3500;

        // L_X kalibrieren (wenn noch nicht fertig)
        if (!cal_lx.is_calibrated) {
            if (lx_raw < cal_lx.min) cal_lx.min = lx_raw;
            if (lx_raw > cal_lx.max) cal_lx.max = lx_raw;

            // Prüfen ob Achse vollständig kalibriert
            if ((cal_lx.max - cal_lx.min) > MIN_CALIBRATED_RANGE) {
                cal_lx.is_calibrated = true;  // Diese Achse ist fertig!
            }
        }

        // L_Y kalibrieren (wenn noch nicht fertig)
        if (!cal_ly.is_calibrated) {
            if (ly_raw < cal_ly.min) cal_ly.min = ly_raw;
            if (ly_raw > cal_ly.max) cal_ly.max = ly_raw;

            if ((cal_ly.max - cal_ly.min) > MIN_CALIBRATED_RANGE) {
                cal_ly.is_calibrated = true;
            }
        }

        // R_X kalibrieren (wenn noch nicht fertig)
        if (!cal_rx.is_calibrated) {
            if (rx_raw < cal_rx.min) cal_rx.min = rx_raw;
            if (rx_raw > cal_rx.max) cal_rx.max = rx_raw;

            if ((cal_rx.max - cal_rx.min) > MIN_CALIBRATED_RANGE) {
                cal_rx.is_calibrated = true;
            }
        }

        // R_Y kalibrieren (wenn noch nicht fertig)
        if (!cal_ry.is_calibrated) {
            if (ry_raw < cal_ry.min) cal_ry.min = ry_raw;
            if (ry_raw > cal_ry.max) cal_ry.max = ry_raw;

            if ((cal_ry.max - cal_ry.min) > MIN_CALIBRATED_RANGE) {
                cal_ry.is_calibrated = true;
            }
        }

        // Prüfe ob ALLE Achsen fertig sind
        if (cal_lx.is_calibrated && cal_ly.is_calibrated &&
            cal_rx.is_calibrated && cal_ry.is_calibrated) {
            all_axes_calibrated = true;  // Nie wieder diesen Block betreten!
        }
    }

    if (auto_calibration_save_pending &&
        cal_lx.is_calibrated && cal_ly.is_calibrated &&
        cal_rx.is_calibrated && cal_ry.is_calibrated) {
        AutoCalibration snapshot;
        capture_current_auto_calibration(snapshot);
        saveAutoCalibration(snapshot);
        auto_calibration_data = snapshot;
        auto_calibration_save_pending = false;
        auto_calibration_loaded = true;
    }
    // Sobald eine Achse is_calibrated=true hat, wird sie nicht mehr verändert!
    // Rohe asymmetrische Kalibrierung - mappt die physische Realität des Sticks!

    *lx = map_axis_u8(lx_raw, cal_lx);
    *ly = map_axis_u8(ly_raw, cal_ly);
    *rx = map_axis_u8(rx_raw, cal_rx);
    *ry = map_axis_u8(ry_raw, cal_ry);

    int16_t lx_signed = (int16_t)(*lx) - 128;
    int16_t ly_signed = (int16_t)(*ly) - 128;
    if (manual_calibration_loaded) {
        apply_manual_radius(lx_signed, ly_signed, manual_calibration.left_radius, manual_left_radius_scale);
    } else if (outer_clamp_enabled) {
        clamp_to_unit_circle(lx_signed, ly_signed);
    }
    if (radial_deadzone_radius > 0) {
        apply_radial_deadzone(&lx_signed, &ly_signed, radial_deadzone_radius);
    }
    *lx = (uint16_t)(lx_signed + 128);
    *ly = (uint16_t)(ly_signed + 128);

    int16_t rx_signed = (int16_t)(*rx) - 128;
    int16_t ry_signed = (int16_t)(*ry) - 128;
    if (manual_calibration_loaded) {
        apply_manual_radius(rx_signed, ry_signed, manual_calibration.right_radius, manual_right_radius_scale);
    } else if (outer_clamp_enabled) {
        clamp_to_unit_circle(rx_signed, ry_signed);
    }
    if (radial_deadzone_radius > 0) {
        apply_radial_deadzone(&rx_signed, &ry_signed, radial_deadzone_radius);
    }
    *rx = (uint16_t)(rx_signed + 128);
    *ry = (uint16_t)(ry_signed + 128);

    if (manual_calibration_loaded && manual_settle_active) {
        const uint32_t settle_frames = 64;
        int16_t ax = (lx_signed >= 0) ? lx_signed : -lx_signed;
        int16_t ay = (ly_signed >= 0) ? ly_signed : -ly_signed;
        int16_t bx = (rx_signed >= 0) ? rx_signed : -rx_signed;
        int16_t by = (ry_signed >= 0) ? ry_signed : -ry_signed;
        int16_t max_abs = ax;
        if (ay > max_abs) max_abs = ay;
        if (bx > max_abs) max_abs = bx;
        if (by > max_abs) max_abs = by;

        if (max_abs > 4) {
            manual_settle_active = false;
        } else if (manual_settle_counter < settle_frames) {
            *lx = 128;
            *ly = 128;
            *rx = 128;
            *ry = 128;
            manual_settle_counter++;
        } else {
            manual_settle_active = false;
        }
    }
}

void ADC_FAST::run_manual_calibration() {
    ManualCalibration new_cal;
    memset(&new_cal, 0, sizeof(new_cal));
    new_cal.magic = 0xCAFEBABE;
    new_cal.is_valid = false;

    // Warte, bis die Start-Kombination losgelassen wurde
    uint32_t release_blink_start = millis();
    while (digitalRead(PIN_R2) == LOW || digitalRead(PIN_TRIANGLE) == LOW) {
        bool on = ((millis() - release_blink_start) % 300) < 150;
        digitalWrite(13, on ? HIGH : LOW);
        delay(10);
    }
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);

    const uint32_t settle_ms = 1500;
    const uint32_t capture_ms = 12000;

    uint32_t start_time = millis();
    while (millis() - start_time < settle_ms) {
        // Dummy reads zum Stabilisieren (Resultate ignorieren)
        ADC1_HC0 = 1;
        ADC2_HC0 = 2;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        (void)ADC1_R0;
        (void)ADC2_R0;

        ADC1_HC0 = 2;
        ADC2_HC0 = 1;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        (void)ADC1_R0;
        (void)ADC2_R0;

        delayMicroseconds(250);
    }

    for (uint8_t i = 0; i < 8; i++) {
        new_cal.left_radius[i] = 0;
        new_cal.right_radius[i] = 0;
    }

    uint32_t capture_start = millis();
    while (millis() - capture_start < capture_ms) {
        uint16_t lx_raw, ly_raw, rx_raw, ry_raw;

        ADC1_HC0 = 1;
        ADC2_HC0 = 2;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        lx_raw = ADC1_R0 & 0xFFF;
        ly_raw = ADC2_R0 & 0xFFF;

        ADC1_HC0 = 2;
        ADC2_HC0 = 1;
        while (!(ADC1_HS & ADC_HS_COCO0) || !(ADC2_HS & ADC_HS_COCO0));
        rx_raw = ADC1_R0 & 0xFFF;
        ry_raw = ADC2_R0 & 0xFFF;

        uint8_t lx_u8 = map_axis_u8(lx_raw, cal_lx);
        uint8_t ly_u8 = map_axis_u8(ly_raw, cal_ly);
        uint8_t rx_u8 = map_axis_u8(rx_raw, cal_rx);
        uint8_t ry_u8 = map_axis_u8(ry_raw, cal_ry);

        int16_t lx_s = (int16_t)lx_u8 - 128;
        int16_t ly_s = (int16_t)ly_u8 - 128;
        int16_t rx_s = (int16_t)rx_u8 - 128;
        int16_t ry_s = (int16_t)ry_u8 - 128;

        update_radius_table(new_cal.left_radius, lx_s, ly_s);
        update_radius_table(new_cal.right_radius, rx_s, ry_s);

        delayMicroseconds(125);
    }

    auto finalize_table = [](uint16_t *table) {
        uint16_t global_max = 0;
        for (uint8_t i = 0; i < 8; i++) {
            if (table[i] > global_max) {
                global_max = table[i];
            }
        }
        if (global_max < 10) {
            global_max = 127;
        }

        for (uint8_t i = 0; i < 8; i++) {
            if (table[i] < 10) {
                uint16_t prev = table[(i + 7) & 0x7];
                uint16_t next = table[(i + 1) & 0x7];
                if (prev < 10 && next < 10) {
                    table[i] = global_max;
                } else if (prev < 10) {
                    table[i] = next;
                } else if (next < 10) {
                    table[i] = prev;
                } else {
                    table[i] = (uint16_t)((prev + next) / 2);
                }
            }
        }

        uint16_t smoothed[8];
        for (uint8_t i = 0; i < 8; i++) {
            uint32_t sum = (uint32_t)table[i] +
                           (uint32_t)table[(i + 7) & 0x7] +
                           (uint32_t)table[(i + 1) & 0x7];
            smoothed[i] = (uint16_t)((sum + 1) / 3);
        }
        for (uint8_t i = 0; i < 8; i++) {
            table[i] = smoothed[i];
        }
    };

    finalize_table(new_cal.left_radius);
    finalize_table(new_cal.right_radius);

    new_cal.is_valid = true;
    manual_calibration = new_cal;
    manual_calibration_loaded = true;
    manual_settle_counter = 0;
    manual_settle_active = true;
    update_manual_scale_factors();
    saveManualCalibration(new_cal);
}

void ADC_FAST::begin_with_manual_calibration() {
    begin();
    run_manual_calibration();
}

void ADC_FAST::begin_with_auto_saved() {
    requested_auto_mode = AutoCalibrationMode::LoadAndLock;
    begin();
}

void ADC_FAST::clear_manual_state() {
    manual_calibration_loaded = false;
    manual_settle_active = false;
    manual_settle_counter = 0;
    manual_left_radius_scale = 1.0f;
    manual_right_radius_scale = 1.0f;
    memset(&manual_calibration, 0, sizeof(manual_calibration));
}

void ADC_FAST::clear_auto_state() {
    reset_axis_calibration_state();
    reset_auto_calibration_runtime();
    auto_calibration_loaded = false;
    memset(&auto_calibration_data, 0, sizeof(auto_calibration_data));
    requested_auto_mode = AutoCalibrationMode::Live;
}

void ADC_FAST::init_profile_state() {
    load_stick_profile_state();
}

void ADC_FAST::reload_profile_state() {
    stick_profile_loaded = false;
    load_stick_profile_state();
}

void ADC_FAST::set_outer_clamp_enabled(bool enabled, bool persist) {
    load_stick_profile_state();
    outer_clamp_enabled = enabled;
    if (persist) {
        persist_stick_profile_state();
    }
}

bool ADC_FAST::is_outer_clamp_enabled() const {
    return outer_clamp_enabled;
}

// Radial Deadzone API (direkt in Units: 0-127) ✅ AKTIV!
void ADC_FAST::set_radial_deadzone_radius(uint8_t radius, bool persist) {
    load_stick_profile_state();
    if (radius > 127) radius = 127;  // Max 127 Units
    radial_deadzone_radius = radius;
    if (persist) {
        // Speichere über g_controller_config (wird automatisch in EEPROM gespeichert)
        g_controller_config.set_radial_deadzone_radius(radius, true);
    }
}

uint8_t ADC_FAST::get_radial_deadzone_radius() const {
    return radial_deadzone_radius;
}

// Inline Deadzone API (direkt in Units, keine Prozent-Konvertierung!) ✅ AKTIV!
void ADC_FAST::set_inline_deadzone_radius(uint8_t radius, bool persist) {
    load_stick_profile_state();
    if (radius > 20) radius = 20;  // Max 20 Units
    inline_deadzone_radius = radius;
    if (persist) {
        // Speichere über g_controller_config (wird automatisch in EEPROM gespeichert)
        g_controller_config.set_inline_deadzone_radius(radius, true);
    }
}

uint8_t ADC_FAST::get_inline_deadzone_radius() const {
    return inline_deadzone_radius;
}


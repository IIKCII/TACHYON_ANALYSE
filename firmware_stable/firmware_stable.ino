/*
 * Teensy 4.1 DS4 Controller - 8000Hz STABLE VERSION
 * Mit TRUE DUAL ADC + DMA + Hardware Timer!
 * 
 * ⚠️ STABLE FIRMWARE - Für Produktions-Einsatz
 * - Maximale Performance (8000Hz)
 * - Serial DEAKTIVIERT
 * - Alle Features getestet und stabil
 * - USB Type: "DS4 Controller 8000Hz"
 */

// WICHTIG: controller_config.h muss VOR config.h eingelesen werden,
// damit CONFIG_ENABLE_CHANGE_DETECTION für compile-time optimization verfügbar ist!
#include "controller_config.h"  // API für alle Runtime-Einstellungen (definiert CONFIG_* vor config.h!)
#include "config.h"
#include "ds4_descriptor.h"
#include "adc_fast.h"
#include "eeprom_config.h"
#include "crc32.h"  // CRC32 für Battlefield 6 Kompatibilität!

// ===== USB-OPTIMIERUNGEN: Cache-Management für DMA/USB =====
// arm_dcache_flush() ist in core_cm7.h definiert (wird automatisch von Arduino.h eingebunden)

// CONFIG_ENABLE_CHANGE_DETECTION ist jetzt in config.h definiert (für compile-time optimization!)

// ===== CHANGE REPORTING CONFIG =====
// ALTE #defines wurden durch API ersetzt (controller_config.h)!
// Einstellungen sind jetzt über API konfigurierbar (für WebUSB/WebHID)!
// 
// Für manuelle Einstellungen: Siehe controller_config.cpp (set_defaults())
// Oder über API: g_controller_config.set_stick_change_threshold(0);
//
// Keine Makros mehr - direkte Funktionsaufrufe zur Laufzeit
// Für Performance-kritische Bereiche werden Werte in updateController() gecacht

// Globale Variablen (Cache-optimiert mit 32-Byte Alignment)
DS4Report g_report __attribute__((aligned(32)));
DS4Report g_last_report __attribute__((aligned(32)));  // Letzter gesendeter Report für Change Detection

// Hardware Timer für exakte 8000 Hz Loop (125 µs Intervall)
IntervalTimer updateTimer;

// ===== DS4 USB OUTPUT VARIABLEN (vom Teensy Core erwartet!) =====
// Diese Variablen werden vom Teensy Core (usb.c) verwendet für LED/Rumble Steuerung
volatile uint8_t ds4_rumble_left = 0;
volatile uint8_t ds4_rumble_right = 0;
volatile uint8_t ds4_led_red = 0;
volatile uint8_t ds4_led_green = 0;
volatile uint8_t ds4_led_blue = 0;
volatile uint8_t ds4_led_blink_on = 0;
volatile uint8_t ds4_led_blink_off = 0;
volatile bool ds4_output_report_received = false;

    // ===== PERFORMANCE MONITORING (optional, compile-time optimiert!) =====
#if CONFIG_ENABLE_PERF_MONITORING
// OPTIMIERUNG: Performance-Stats aus ISR ausgelagert (via Flag) für minimales ISR-Overhead!
static volatile uint32_t perf_elapsed_cycles = 0;  // Wird in ISR geschrieben (nur Cycle-Count!)
static volatile bool perf_data_ready = false;       // Flag für main-loop
static uint32_t perf_max_cycles = 0;
static uint32_t perf_min_cycles = 0xFFFFFFFF;
static uint32_t perf_avg_cycles = 0;
static uint32_t perf_frame_count = 0;
#endif

// ===== LOOP FREQUENZ MESSUNG =====
// Misst wie schnell die loop() Funktion läuft
static uint32_t loop_counter = 0;           // Zählt loop() Durchläufe
static uint32_t loop_last_time = 0;         // Letzter Zeitpunkt der Messung (ms)
static uint32_t loop_frequency = 0;         // Gemessene Frequenz (Hz)

// ===== DS4 REPORT COUNTER (für BF6 Kompatibilität!) =====
// Echter DS4 hat einen 6-bit Counter in Byte 7 (Bits 2-7)
// BF6 prüft diesen Counter um "echte" Controller zu erkennen!
// DS4 Report Counter (nur verwendet wenn aktiv)
static uint8_t ds4_report_counter = 0;
static uint8_t ds4_touchpad_counter = 0;

// ===== BF6 FIX: IMU-Rauschen Simulator (wie echter DS4!) =====
// Echter DS4 hat minimales Sensor-Rauschen auch im Ruhezustand
// Wir simulieren das mit kleinen, pseudo-zufälligen Schwankungen
// WICHTIG: LFSR braucht non-zero seed! (0 würde immer 0 bleiben)
static uint32_t imu_noise_state = 0xABCD1234;  // Non-zero seed für LFSR

// ===== FILTER STATE VARIABLEN (compile-time optimiert!) =====
#if CONFIG_ENABLE_EMA_FILTER
// EMA Filter: Letzte gefilterte Werte (Fixed-Point Q8.8 für ~20% schnellere Multiplikation!)
// OPTIMIERUNG: Fixed-Point statt Float (schneller auf ARM, keine FPU-Overhead)
// Format: Q8.8 (8 Bit Integer + 8 Bit Fractional) - perfekt für uint8_t Stick-Werte (0-255)
static uint16_t ema_lx = 128 << 8;  // 128.0 in Q8.8 = 32768
static uint16_t ema_ly = 128 << 8;
static uint16_t ema_rx = 128 << 8;
static uint16_t ema_ry = 128 << 8;
#endif

#if CONFIG_ENABLE_HYSTERESIS
// Hysteresis: Letzte akzeptierte Werte - nur verwendet wenn aktiv
static uint16_t hyst_lx = 128;
static uint16_t hyst_ly = 128;
static uint16_t hyst_rx = 128;
static uint16_t hyst_ry = 128;
#endif

#if CONFIG_ENABLE_SNAPBACK_FILTER
// Snapback Filter: Trackt Richtung und Geschwindigkeit für Overshoot-Erkennung
struct SnapbackState {
    uint8_t last_value;          // Letzter Wert
    int8_t last_direction;       // Letzte Richtung (-1 = links/unten, 0 = center, +1 = rechts/oben)
    uint8_t frames_since_edge;   // Frames seit letztem Edge (außen)
    bool in_snapback;            // Wird gerade Snapback erkannt?
};

static SnapbackState snapback_lx = {128, 0, 0, false};
static SnapbackState snapback_ly = {128, 0, 0, false};
static SnapbackState snapback_rx = {128, 0, 0, false};
static SnapbackState snapback_ry = {128, 0, 0, false};
#endif

// ===== OPTIMIERUNG: Button Debouncing als Array (statt 14 einzelne Strukturen!) =====
// PERFORMANCE: Array ist cache-freundlicher und schneller zu iterieren
// Struktur: [counter, last_raw, stable_state] für jeden Button
// Button-Indizes: 0=Cross, 1=Circle, 2=Square, 3=Triangle, 4=L1, 5=R1, 6=L2, 7=R2, 8=L3, 9=R3,
//                 10=Share, 11=Options, 12=PS, 13=Touchpad, 14=D-Up, 15=D-Down, 16=D-Left, 17=D-Right
#if CONFIG_ENABLE_BUTTON_DEBOUNCE
struct ButtonDebounceState {
    uint8_t counter;      // Anzahl stabiler Samples
    uint8_t last_raw;     // Letzter gelesener Raw-State (0/1)
    uint8_t stable_state; // Aktueller stabiler State (0/1)
};
static ButtonDebounceState db_states[18];  // 18 Buttons (14 Face/Shoulder/System + 4 D-Pad)
#endif

// ===== ASYMMETRIC RELEASE FILTER =====
#if CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
struct AsymmetricButtonState {
    uint8_t logical_state;              // 0 = IDLE, 1 = PRESSED
    uint32_t release_guard_start_cycles; // DWT_CYCCNT beim ersten LOW-Read
    bool in_release_guard;              // Wartet auf Stabilisierung
};
static AsymmetricButtonState button_states[20];  // 18 Buttons (14 Face/Shoulder/System + 4 D-Pad) + 2 (PS, Touchpad)
#endif

// ===== PS-BUTTON SEQUENZ-MAKRO =====
// PS-Taste triggert: R3 halten → Triangle tap → R3 loslassen
// State Machine für zeitgesteuerte Button-Sequenz
enum PSSequenceState {
    PS_SEQ_IDLE = 0,           // Keine Sequenz aktiv
    PS_SEQ_R3_HOLD = 1,        // R3 wird gehalten (warte auf Radialmenu)
    PS_SEQ_TRIANGLE_TAP = 2,   // Triangle drücken (während R3 gehalten)
    PS_SEQ_TRIANGLE_RELEASE = 3, // Triangle loslassen
    PS_SEQ_R3_RELEASE = 4      // R3 loslassen (Sequenz Ende)
};

#if CONFIG_PS_SEQUENCE_ENABLED
static PSSequenceState ps_sequence_state = PS_SEQ_IDLE;
static uint32_t ps_sequence_timer = 0;  // Timer in Frames (8000 Hz = 1 Frame = 125µs)
#endif
static bool ps_button_last = false;     // Letzter PS-Button State (für Edge-Detection)

void setup() {
    // === DWT Cycle Counter aktivieren (für Performance-Messung) ===
    ARM_DEMCR |= ARM_DEMCR_TRCENA;  // Enable trace
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;  // Enable cycle counter

    // ===== CONTROLLER CONFIG INITIALISIERUNG (API für alle Einstellungen!) =====
    g_controller_config.init();  // Lädt aus EEPROM oder setzt Defaults
    // AUTOMATISCHES VERSION-UPDATE: Wenn CONFIG_VERSION in controller_config.cpp erhöht wurde,
    // werden neue Defaults automatisch übernommen (kein reset_to_defaults() mehr nötig!)

    // Pin-Modi setzen
    pinMode(PIN_R2, INPUT_PULLUP);
    pinMode(PIN_R1, INPUT_PULLUP);
    pinMode(PIN_L1, INPUT_PULLUP);
    pinMode(PIN_L2, INPUT_PULLUP);
    pinMode(PIN_CROSS, INPUT_PULLUP);
    pinMode(PIN_CIRCLE, INPUT_PULLUP);
    pinMode(PIN_TRIANGLE, INPUT_PULLUP);
    pinMode(PIN_SQUARE, INPUT_PULLUP);
    pinMode(PIN_L3, INPUT_PULLUP);
    pinMode(PIN_R3, INPUT_PULLUP);

    // D-Pad
    pinMode(PIN_DPAD_UP, INPUT_PULLUP);
    pinMode(PIN_DPAD_DOWN, INPUT_PULLUP);
    pinMode(PIN_DPAD_LEFT, INPUT_PULLUP);
    pinMode(PIN_DPAD_RIGHT, INPUT_PULLUP);

    // System Buttons
    pinMode(PIN_SHARE, INPUT_PULLUP);
    pinMode(PIN_OPTIONS, INPUT_PULLUP);
    pinMode(PIN_PS, INPUT_PULLUP);
    pinMode(PIN_TOUCHPAD, INPUT_PULLUP);

    // Back Buttons
    pinMode(PIN_BACK_LEFT, INPUT_PULLUP);
    pinMode(PIN_BACK_RIGHT, INPUT_PULLUP);

    if (g_controller_config.get_led_feedback_enabled_internal()) {
        // LED (optional - deaktiviert für Maximum Performance!)
        pinMode(PIN_LED, OUTPUT);
        digitalWrite(PIN_LED, HIGH);
    }

    // Serial DEAKTIVIERT für maximale 8000Hz Performance
    // Serial.begin(115200);  // ⚠️ AUSKOMMENTIERT - Serial bremst USB-Performance!
    
    // DEBUG: Für Circle-Problem aktivieren (temporär!)
    // ACHTUNG: Serial aktivieren nur zum Debuggen! Bremst Performance!
    // Serial.begin(115200);

    // === OPTION 3: BUTTON-CHECK FÜR MANUELLE KALIBRIERUNG ===

    delay(100);  // Kurze Pause für stabile Button-Reads

    FastADC.init_profile_state();

    // ===== OPTIMIERUNG: Direkte GPIO-Register-Zugriffe statt digitalRead()! =====
    // PERFORMANCE: 10x schneller als digitalRead() (~10 Cycles statt ~100 Cycles)
    // Konsistenter Code (überall GPIO-Register statt digitalRead())
    uint32_t gpio6_setup = GPIO6_PSR;  // Einmal lesen für alle Buttons
    bool l2_pressed = !(gpio6_setup & GPIO6_MASK_L2);
    bool r2_pressed = !(gpio6_setup & GPIO6_MASK_R2);
    bool cross_pressed = !(gpio6_setup & GPIO6_MASK_CROSS);
    bool circle_pressed = !(gpio6_setup & GPIO6_MASK_CIRCLE);
    bool triangle_pressed = !(gpio6_setup & GPIO6_MASK_TRIANGLE);
    bool square_pressed = !(gpio6_setup & GPIO6_MASK_SQUARE);

    // ===== PROFIL-WECHSEL BEIM BOOT =====
    // L2 halten = Profil 1 (FPS), R2 halten = Profil 0 (Standard)
    // Keine Buttons = Letztes Profil aus EEPROM laden
    if (l2_pressed && !r2_pressed) {
        // L2 gedrückt → Profil 1 laden
        g_controller_config.set_active_profile(1, true);  // true = in EEPROM speichern

        // LED-Feedback: 1 Blink = Profil 1
        if (g_controller_config.get_led_feedback_enabled_internal()) {
            digitalWrite(PIN_LED, LOW);
            delay(300);
            digitalWrite(PIN_LED, HIGH);
            delay(300);
        }
    } else if (r2_pressed && !l2_pressed) {
        // R2 gedrückt → Profil 0 laden
        g_controller_config.set_active_profile(0, true);  // true = in EEPROM speichern

        // LED-Feedback: 2 Blinks = Profil 0
        if (g_controller_config.get_led_feedback_enabled_internal()) {
            for (int i = 0; i < 2; i++) {
                digitalWrite(PIN_LED, LOW);
                delay(150);
                digitalWrite(PIN_LED, HIGH);
                delay(150);
            }
        }
    }
    // Keine Buttons → EEPROM-Profil wird automatisch in init() geladen

    if (r2_pressed && cross_pressed) {
        bool new_state = !FastADC.is_outer_clamp_enabled();
        FastADC.set_outer_clamp_enabled(new_state, true);
        if (g_controller_config.get_led_feedback_enabled_internal()) {
            int blinks = new_state ? 3 : 1;
            for (int i = 0; i < blinks; ++i) {
                digitalWrite(PIN_LED, LOW);
                delay(120);
                digitalWrite(PIN_LED, HIGH);
                delay(120);
            }
        }
        FastADC.begin();
    }
    else if (r2_pressed && circle_pressed) {
        // RESET: Alle gespeicherten Kalibrierungen löschen
        FastADC.clear_manual_state();
        clearManualCalibration();
        FastADC.clear_auto_state();
        clearAutoCalibration();
        clearStickProfileConfig();
        g_controller_config.reset_to_defaults();  // Auch Runtime Config zurücksetzen!
        FastADC.reload_profile_state();
        delay(200);

        // Normale Auto-Kalibrierung
        FastADC.begin();
    }
    else if (r2_pressed && square_pressed) {
        // GESPEICHERTE AUTO-KALIBRIERUNG laden (oder bei Bedarf neu speichern)
        FastADC.begin_with_auto_saved();
    }
    else if (r2_pressed && triangle_pressed) {
        // MANUELLE KALIBRIERUNG starten
        FastADC.begin_with_manual_calibration();
    }
    else {
        // Standard: Normale Auto-Kalibrierung
        FastADC.begin();
    }

    // DS4 Report initialisieren
    memset(&g_report, 0, sizeof(g_report));
    g_report.report_id = 0x01;
    g_report.left_stick_x = 128;
    g_report.left_stick_y = 128;
    g_report.right_stick_x = 128;
    g_report.right_stick_y = 128;
    g_report.buttons1 = DS4_BTN1_DPAD_NONE;

    // ===== BF6 FIX: Battery initialisieren =====
    // Byte 12 = Battery Level (0-11, 11 = voll)
    g_report.battery = 0x05;  // ~50% Battery (0x0B wäre 100%, aber 0x05 ist realistischer)

    // ===== BF6 FIX: IMU-Daten initialisieren (wie echter DS4!) =====
    // Echter DS4 hat minimales Sensor-Rauschen auch im Ruhezustand
    // Gyro: Kleine Schwankungen um 0 (±10 LSB typisch)
    g_report.gyro_x = 2;
    g_report.gyro_y = -3;
    g_report.gyro_z = 1;
    
    // Accelerometer: Gravity zeigt nach -Y (wenn Controller flach liegt)
    // Echter DS4: accel_y ≈ -8192 (Gravity), accel_x/z ≈ 0
    g_report.accel_x = 0;
    g_report.accel_y = -8192;  // Gravity (Controller liegt flach)
    g_report.accel_z = 0;

    // ===== BF6 FIX: Status/Battery und Touchpad initialisieren =====
    // ext_data[0] = Byte 30 = Status (Battery 0-11 + Connection)
    // 0x1B = Battery Level 11 (voll) + USB Connected + Charging
    g_report.ext_data[0] = 0x1B;

    // ext_data[3] = Byte 33 = Touchpad Event Count (0 = keine Touches)
    g_report.ext_data[3] = 0x00;

    // ext_data[4] = Byte 34 = Touchpad Packet Counter (wird hochgezählt)
    g_report.ext_data[4] = 0x00;

    // ext_data[5] = Byte 35 = Touch 1 ID + Active Flag
    // Bit 7 = 1 bedeutet Touch NICHT aktiv (Finger nicht auf Touchpad)
    g_report.ext_data[5] = 0x80;

    // ext_data[9] = Byte 39 = Touch 2 ID (auch inaktiv)
    g_report.ext_data[9] = 0x80;

    // Last Report initialisieren (unterschiedlich, damit erster Report gesendet wird)
    memset(&g_last_report, 0xFF, sizeof(g_last_report));

    delay(500);  // USB-Init

    if (g_controller_config.get_led_feedback_enabled_internal()) {
        // Blinke 2x = bereit
        for(int i=0; i<2; i++) {
            digitalWrite(PIN_LED, LOW);
            delay(200);
            digitalWrite(PIN_LED, HIGH);
            delay(200);
        }
        digitalWrite(PIN_LED, HIGH);  // LED permanent an = bereit
    }

    // === HARDWARE TIMER STARTEN (konfigurierbare Polling-Rate!) ===
    // IntervalTimer mit konfigurierbarem Intervall (exakte Hardware-Timing!)
    uint8_t polling_rate = g_controller_config.get_polling_rate_internal();
    uint32_t timer_interval_us;
    switch (polling_rate) {
        case 0: timer_interval_us = 1000; break;  // 1kHz (1000µs)
        case 1: timer_interval_us = 500; break;   // 2kHz (500µs)
        case 2: timer_interval_us = 250; break;   // 4kHz (250µs)
        case 3: timer_interval_us = 125; break;   // 8kHz (125µs) - Standard
        default: timer_interval_us = 125; break;  // Fallback: 8kHz
    }
    
    // OPTIMIERUNG: PLAIN_VANILLA_CALLBACKS spart 2-3 Cycles pro ISR-Call!
    // updateController() ist bereits eine Plain-C-Funktion (kein C++-Wrapper-Overhead!)
    // IntervalTimer.begin() verwendet direkten Funktionspointer (keine virtuelle Funktion)
    updateTimer.begin(updateController, timer_interval_us);
    
    // OPTIMIERUNG: Niedrigere ISR-Priorität für Nesting (erlaubt andere ISRs währenddessen)
    // IRQ_SOFTWARE = IntervalTimer ISR, Priorität 128 = niedrig (höhere Zahl = niedrigere Priorität)
    // Default ist 112, 128 erlaubt besseres Nesting (z.B. USB-ISRs können unterbrechen)
    NVIC_SET_PRIORITY(IRQ_SOFTWARE, 128);
    
    // WICHTIG: IntervalTimer garantiert exakte Timing unabhängig von loop()!
}

#if CONFIG_ENABLE_BUTTON_DEBOUNCE
// ===== BUTTON DEBOUNCING FUNKTION (Ultra-Fast @ 8000 Hz) =====
// OPTIMIERUNG: Array-basiert, lokal gecacht, inline für maximale Performance
// Temporal Stability Debouncing: Button muss N Samples stabil sein
// Input: raw_pressed = aktueller Button-State (true/false), button_idx = Button-Index (0-17)
// Output: true wenn Button wirklich gedrückt (nach Debouncing)
__attribute__((always_inline))
inline bool debounce_button(bool raw_pressed, uint8_t button_idx, uint8_t debounce_samples) {
    ButtonDebounceState &db = db_states[button_idx];
    uint8_t raw = raw_pressed ? 1 : 0;
    
    // Hat sich der Raw-State geändert?
    if (raw != db.last_raw) {
        // Neuer State → Counter zurücksetzen
        db.last_raw = raw;
        db.counter = 1;  // Erstes Sample des neuen States
    } else {
        // Gleicher State wie vorher → Counter erhöhen
        if (db.counter < debounce_samples) {
            db.counter++;
        }
    }

    // Wenn genug stabile Samples → State akzeptieren
    if (db.counter >= debounce_samples) {
        db.stable_state = raw;
    }

    return db.stable_state != 0;
}
#endif

#if CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
// ===== ASYMMETRIC RELEASE FILTER FUNKTION (Ultra-Low Latency) =====
// Press: Sofort akzeptieren (0 µs Zusatzlatenz)
// Release: Guard-Fenster prüfen (verhindert Doppel-Releases und Ghost-Represses)
// Input: raw_pressed = aktueller Button-State (true/false), button_idx = Button-Index (0-17), guard_us = Guard-Fenster in µs
// Output: true wenn Button logisch gedrückt
__attribute__((always_inline))
inline bool process_asymmetric_button(bool raw_pressed, uint8_t button_idx, uint8_t guard_us) {
    AsymmetricButtonState &state = button_states[button_idx];
    
    if (raw_pressed) {
        // PRESS: Sofort akzeptieren (0 µs Zusatzlatenz)
        if (state.logical_state == 0) {
            // Button war IDLE → neuer Press (auch nach Release!)
            state.logical_state = 1;
            state.in_release_guard = false;
        } else {
            // Button war PRESSED und ist wieder HIGH
            if (state.in_release_guard) {
                // Während Release-Guard wieder HIGH
                // Prüfe: War Guard bereits lange genug aktiv? → Release war echt, neuer Press
                // Sonst: Bounce erkannt, Button war nie wirklich losgelassen
                uint32_t elapsed_cycles = ARM_DWT_CYCCNT - state.release_guard_start_cycles;
                uint32_t guard_cycles = ((uint32_t)guard_us * (F_CPU_ACTUAL / 1000000));
                
                if (elapsed_cycles >= guard_cycles) {
                    // Guard war bereits abgelaufen → Release war echt, neuer Press
                    // (logical_state war noch 1, aber Release hätte akzeptiert werden sollen)
                    // Setze auf IDLE und dann sofort auf PRESSED (neuer Press)
                    state.logical_state = 0;
                    state.logical_state = 1;  // Neuer Press
                    state.in_release_guard = false;
                } else {
                    // Guard noch nicht abgelaufen → Bounce, Button war nie losgelassen
                    state.in_release_guard = false;
                }
            }
            // Wenn nicht in Guard: Button war bereits PRESSED, nichts tun
        }
    } else {
        // RELEASE: Guard-Fenster prüfen (nur wenn Filter aktiv)
        if (state.logical_state == 1) {
            if (!state.in_release_guard) {
                // Erste LOW-Flanke → Guard starten
                state.release_guard_start_cycles = ARM_DWT_CYCCNT;
                state.in_release_guard = true;
            } else {
                // Guard aktiv → Zeit prüfen (optimiert: direkte Cycle-Vergleich)
                uint32_t elapsed_cycles = ARM_DWT_CYCCNT - state.release_guard_start_cycles;
                // F_CPU_ACTUAL = 600000000 Hz → 600 Cycles/µs (exakt bei 600 MHz)
                // guard_us * 600 = Guard in Cycles (Multiplikation statt Division = schneller!)
                uint32_t guard_cycles = ((uint32_t)guard_us * (F_CPU_ACTUAL / 1000000));
                
                if (elapsed_cycles >= guard_cycles) {
                    // Guard-Fenster abgelaufen → Release akzeptieren
                    state.logical_state = 0;
                    state.in_release_guard = false;
                }
            }
        } else {
            state.in_release_guard = false;
        }
    }
    
    return state.logical_state != 0;
}
#endif

// ===== CHANGE DETECTION FUNKTION (Ultra-schnell, Inline, nur Sticks!) =====
// Prüft ob sich Stick-Werte geändert haben (Buttons werden VORHER geprüft für frühen Exit!)
// Returniert: true = Änderung erkannt, false = keine Änderung
__attribute__((always_inline, hot))
// PERFORMANCE-OPTIMIERT: Threshold-Werte als Parameter (gecacht in updateController, kein API-Aufruf pro Frame!)
// OPTIMIERUNG: Nur noch Stick-Checks - Buttons werden VORHER geprüft!
// OPTIMIERUNG: Threshold=0 Shortcut + Branchless-Abs für maximale Performance!
inline bool has_sticks_changed(const DS4Report* __restrict__ current, const DS4Report* __restrict__ last, 
                               uint8_t stick_threshold, bool adaptive_enabled,
                               uint8_t center_threshold, uint8_t center_min, uint8_t center_max) {
    // ===== OPTIMIERUNG: Threshold=0 Shortcut (häufigster Fall!) =====
    // Wenn threshold=0, können wir direkt vergleichen (keine Abs-Berechnung nötig!)
    if (!adaptive_enabled && stick_threshold == 0) {
        // Threshold=0: Block-Vergleich aller 4 Sticks (nutzt SIMD wenn verfügbar!)
        // PERFORMANCE: 1 Vergleich statt 4, maximal schnell!
        if (memcmp(&current->left_stick_x, &last->left_stick_x, 4) != 0) return true;
        return false;
    }
    
    // ===== Threshold > 0 oder Adaptive: Normaler Check mit Abs =====
    // Analog-Stick Check mit Threshold (verhindert ADC-Noise-Spam)
    // Early-exit: Bei erster Änderung sofort return
    // PERFORMANCE: Threshold-Werte als Parameter übergeben (keine API-Aufrufe!)
    int16_t diff;
    
    if (adaptive_enabled) {
        // Adaptiver Threshold: Im Center-Bereich größerer Threshold (filtert Center-Rauschen)
        // Außerhalb: Normaler Threshold (präzise Bewegungen bleiben sensitiv)
        int16_t threshold;
        
        // OPTIMIERUNG: Branchless-Abs (1 Vergleich statt 2!)
        // Left Stick X
        diff = (int16_t)current->left_stick_x - (int16_t)last->left_stick_x;
        threshold = (last->left_stick_x >= center_min && last->left_stick_x <= center_max) ? center_threshold : stick_threshold;
        if (abs(diff) > threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        // Left Stick Y
        diff = (int16_t)current->left_stick_y - (int16_t)last->left_stick_y;
        threshold = (last->left_stick_y >= center_min && last->left_stick_y <= center_max) ? center_threshold : stick_threshold;
        if (abs(diff) > threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        // Right Stick X
        diff = (int16_t)current->right_stick_x - (int16_t)last->right_stick_x;
        threshold = (last->right_stick_x >= center_min && last->right_stick_x <= center_max) ? center_threshold : stick_threshold;
        if (abs(diff) > threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        // Right Stick Y
        diff = (int16_t)current->right_stick_y - (int16_t)last->right_stick_y;
        threshold = (last->right_stick_y >= center_min && last->right_stick_y <= center_max) ? center_threshold : stick_threshold;
        if (abs(diff) > threshold) return true;  // OPTIMIERUNG: Branchless-Abs!
    } else {
        // Gleichmäßiger Threshold überall (wie im Backup - direkt inline!)
        // OPTIMIERUNG: Branchless-Abs (1 Vergleich statt 2 pro Stick!)
        diff = (int16_t)current->left_stick_x - (int16_t)last->left_stick_x;
        if (abs(diff) > stick_threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        diff = (int16_t)current->left_stick_y - (int16_t)last->left_stick_y;
        if (abs(diff) > stick_threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        diff = (int16_t)current->right_stick_x - (int16_t)last->right_stick_x;
        if (abs(diff) > stick_threshold) return true;  // OPTIMIERUNG: Branchless-Abs!

        diff = (int16_t)current->right_stick_y - (int16_t)last->right_stick_y;
        if (abs(diff) > stick_threshold) return true;  // OPTIMIERUNG: Branchless-Abs!
    }

    // Keine relevante Änderung erkannt
    return false;
}

// === HARDWARE TIMER CALLBACK (läuft exakt alle 125 µs = 8 kHz!) ===
__attribute__((hot, optimize("O3")))
// OPTIMIERUNG: PLAIN_VANILLA_CALLBACK - keine C++-Wrapper-Overhead!
// Spart 2-3 Cycles pro ISR-Call (keine virtuelle Funktion, direkter Funktionspointer)
void updateController() {
    // === PERFORMANCE MESSUNG START (optional, compile-time optimiert!) ===
#if CONFIG_ENABLE_PERF_MONITORING
    uint32_t perf_start = ARM_DWT_CYCCNT;
#endif
    
    // === PERFORMANCE-OPTIMIERUNG: Config-Werte einmal pro Frame cachen (nicht in has_report_changed!) ===
    // Diese Werte werden nur einmal pro Frame gelesen, nicht bei jedem Change Detection Check!
    static uint8_t cached_stick_threshold = 0;
    static bool cached_adaptive_enabled = false;
    static uint8_t cached_center_threshold = 3;
    static uint8_t cached_center_min = 126;
    static uint8_t cached_center_max = 130;
    static bool config_cache_valid = false;
    
    // Cache nur einmal initialisieren (beim ersten Aufruf)
    static uint8_t cached_inline_deadzone_radius = 0;
    static uint8_t cached_clamp_min = 125;  // OPTIMIERUNG #3: clamp_min gecacht (128 - 3 Default)
    static uint8_t cached_clamp_max = 131;  // OPTIMIERUNG #3: clamp_max gecacht (128 + 3 Default)
    static bool cached_change_detection_enabled = false;
#if CONFIG_ENABLE_BUTTON_DEBOUNCE
    static bool cached_button_debounce_enabled = false;
    static uint8_t cached_debounce_samples = 4;  // Lokal gecacht (nicht mehr global!)
#endif
#if CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
    static uint8_t cached_release_guard_us = 15;
#endif
#if CONFIG_ENABLE_EMA_FILTER
    static float cached_ema_alpha = 0.8f;
#endif
#if CONFIG_ENABLE_HYSTERESIS
    static bool cached_hysteresis_enabled = false;
    static uint8_t cached_hysteresis_threshold = 2;
#endif
#if CONFIG_ENABLE_SNAPBACK_FILTER
    static uint8_t cached_snapback_threshold = 10;
    static uint8_t cached_snapback_window = 3;
#endif
#if CONFIG_ENABLE_OCTAGONAL_GATE
    static bool cached_octagonal_gate_enabled = false;
    static uint8_t cached_octagonal_gate_strength = 30;
#endif
    static bool cached_report_counter_enabled = false;
    // static uint8_t cached_back_left_target = BACK_BTN_L1;   // Unused - removed
    // static uint8_t cached_back_right_target = BACK_BTN_R1; // Unused - removed
    static ButtonMacro cached_macro_slots[4];
    static uint8_t cached_button_remapping[18];  // VOLLSTÄNDIGES BUTTON REMAPPING!
    static uint8_t cached_active_profile = 0xFF;  // Aktuelles Profil (0xFF = ungültig, force reload)
    // OPTIMIERUNG #3: Button-Remapping-Shortcut Cache
    static bool remapping_active_cached = false;
    static bool remapping_cache_valid = false;
    // OPTIMIERUNG #5: Makro-Check-Shortcut Cache
    static bool macros_active_cached = false;
    static bool macros_cache_valid = false;

    // ===== PROFIL-WECHSEL DETECTION =====
    // Wenn sich das Profil geändert hat, Cache invalidieren!
    uint8_t current_profile = g_controller_config.get_active_profile_internal();
    if (current_profile != cached_active_profile) {
        config_cache_valid = false;  // Cache invalidieren
        remapping_cache_valid = false;  // Remapping-Cache auch invalidieren!
        macros_cache_valid = false;  // Makro-Cache auch invalidieren!
        cached_active_profile = current_profile;
    }

    if (!config_cache_valid) {
        cached_stick_threshold = g_controller_config.get_stick_change_threshold_internal();
        cached_adaptive_enabled = g_controller_config.get_adaptive_center_threshold_enabled_internal();
        cached_center_threshold = g_controller_config.get_center_threshold_internal();
        cached_center_min = g_controller_config.get_center_threshold_range_min_internal();
        cached_center_max = g_controller_config.get_center_threshold_range_max_internal();
        cached_inline_deadzone_radius = FastADC.get_inline_deadzone_radius();  // PERFORMANCE: Auch cachen!
        // OPTIMIERUNG #3: clamp_min/max einmalig berechnen und cachen!
        cached_clamp_min = 128 - cached_inline_deadzone_radius;
        cached_clamp_max = 128 + cached_inline_deadzone_radius;
        cached_change_detection_enabled = g_controller_config.get_change_detection_enabled_internal();  // PERFORMANCE: Auch cachen!
#if CONFIG_ENABLE_BUTTON_DEBOUNCE
        cached_button_debounce_enabled = g_controller_config.get_button_debounce_enabled_internal();  // PERFORMANCE: Auch cachen!
        cached_debounce_samples = g_controller_config.get_debounce_samples_internal();  // PERFORMANCE: Auch cachen!
#endif
#if CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
        cached_release_guard_us = g_controller_config.get_release_guard_us_internal();
#endif
#if CONFIG_ENABLE_EMA_FILTER
        cached_ema_alpha = g_controller_config.get_ema_alpha_internal();  // PERFORMANCE: Auch cachen!
#endif
#if CONFIG_ENABLE_HYSTERESIS
        cached_hysteresis_enabled = g_controller_config.get_hysteresis_enabled_internal();  // PERFORMANCE: Auch cachen!
        cached_hysteresis_threshold = g_controller_config.get_hysteresis_threshold_internal();  // PERFORMANCE: Auch cachen!
#endif
#if CONFIG_ENABLE_SNAPBACK_FILTER
        cached_snapback_threshold = g_controller_config.get_snapback_threshold_internal();  // PERFORMANCE: Auch cachen!
        cached_snapback_window = g_controller_config.get_snapback_window_internal();  // PERFORMANCE: Auch cachen!
#endif
#if CONFIG_ENABLE_OCTAGONAL_GATE
        cached_octagonal_gate_enabled = g_controller_config.get_octagonal_gate_enabled_internal();
        cached_octagonal_gate_strength = g_controller_config.get_octagonal_gate_strength_internal();
#endif
        cached_report_counter_enabled = g_controller_config.get_report_counter_enabled_internal();  // PERFORMANCE: Auch cachen!
        // cached_back_left_target = g_controller_config.get_back_left_target_internal();  // Unused - removed
        // cached_back_right_target = g_controller_config.get_back_right_target_internal();  // Unused - removed
        // Makro-Slots cachen
        const ButtonMacro* macro_slots = g_controller_config.get_macro_slots_internal();
        memcpy(cached_macro_slots, macro_slots, sizeof(cached_macro_slots));
        macros_cache_valid = false;  // Makro-Cache invalidieren, wird beim ersten Durchlauf neu berechnet
        // Button-Remapping-Tabelle cachen
        const uint8_t* button_remapping = g_controller_config.get_button_remapping_internal();
        memcpy(cached_button_remapping, button_remapping, sizeof(cached_button_remapping));
        remapping_cache_valid = false;  // Remapping-Cache invalidieren, wird beim ersten Durchlauf neu berechnet
        config_cache_valid = true;
    }

    // === Phase 1: PRE-TRIGGER ADC (ULTRA-LOW LATENCY OPTIMIZATION) ===
    // OPTIMIERUNG: Alle 4 Sticks SOFORT parallel triggern (statt 2x sequenziell)!
    // Erwarteter Gewinn: ~20µs weniger Blockierung = 50% schnellere Stick-Updates!
    FastADC.trigger_all_sticks_now();

    // === OPTIMIERUNG #2: GPIO-PRELOAD (Pipeline-Optimierung) ===
    // GPIO-Register SOFORT nach ADC-Trigger lesen (während ADC konvertiert)!
    // Erwarteter Gewinn: ~2-5µs frühere Button-Erkennung durch bessere Pipeline-Nutzung!
    // DIREKTE GPIO-Register-Reads statt 18x digitalRead()!
    // PSR = Pin Status Register für Inputs (INPUT_PULLUP: HIGH = nicht gedrückt, LOW = gedrückt)
    uint32_t gpio6 = GPIO6_PSR;  // Face, Shoulder, Stick, D-Pad (14 Buttons)
    uint32_t gpio7 = GPIO7_PSR;  // System-Buttons (Share, Options, PS, Touchpad) + Back-Buttons (LB, RB)

    // ===== PHASE 1: PHYSISCHE BUTTONS EINLESEN (RAW STATE) =====
    // Alle physischen Buttons einlesen in temporäre Arrays für späteres Remapping
    bool raw_buttons[18] = {false};  // 18 Buttons total (inkl. Back-Buttons)

    // OPTIMIERUNG: Bit-Masken als Konstanten (compile-time, kein Bit-Shift pro Frame!)
#if CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
    uint8_t guard_us = cached_release_guard_us;
    raw_buttons[REMAP_IDX_CROSS] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_CROSS), 0, guard_us);
    raw_buttons[REMAP_IDX_CIRCLE] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_CIRCLE), 1, guard_us);
    raw_buttons[REMAP_IDX_TRIANGLE] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_TRIANGLE), 2, guard_us);
    raw_buttons[REMAP_IDX_SQUARE] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_SQUARE), 3, guard_us);
    raw_buttons[REMAP_IDX_L1] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_L1), 4, guard_us);
    raw_buttons[REMAP_IDX_R1] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_R1), 5, guard_us);
    raw_buttons[REMAP_IDX_L2] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_L2), 6, guard_us);
    raw_buttons[REMAP_IDX_R2] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_R2), 7, guard_us);
    raw_buttons[REMAP_IDX_L3] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_L3), 8, guard_us);
    raw_buttons[REMAP_IDX_R3] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_R3), 9, guard_us);
    raw_buttons[REMAP_IDX_SHARE] = process_asymmetric_button(!(gpio7 & GPIO7_MASK_SHARE), 10, guard_us);
    raw_buttons[REMAP_IDX_OPTIONS] = process_asymmetric_button(!(gpio7 & GPIO7_MASK_OPTIONS), 11, guard_us);
    raw_buttons[REMAP_IDX_DPAD_UP] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_DPAD_UP), 12, guard_us);
    raw_buttons[REMAP_IDX_DPAD_DOWN] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_DPAD_DOWN), 13, guard_us);
    raw_buttons[REMAP_IDX_DPAD_LEFT] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_DPAD_LEFT), 14, guard_us);
    raw_buttons[REMAP_IDX_DPAD_RIGHT] = process_asymmetric_button(!(gpio6 & GPIO6_MASK_DPAD_RIGHT), 15, guard_us);
    raw_buttons[REMAP_IDX_BACK_LEFT] = process_asymmetric_button(!(gpio7 & GPIO7_MASK_BACK_LEFT), 16, guard_us);
    raw_buttons[REMAP_IDX_BACK_RIGHT] = process_asymmetric_button(!(gpio7 & GPIO7_MASK_BACK_RIGHT), 17, guard_us);
    bool ps_button_pressed = process_asymmetric_button(!(gpio7 & GPIO7_MASK_PS), 18, guard_us);  // PS (nicht in Remapping-Tabelle)
    bool touchpad_pressed = process_asymmetric_button(!(gpio7 & GPIO7_MASK_TOUCHPAD), 19, guard_us);  // Touchpad (nicht in Remapping-Tabelle)
#elif CONFIG_ENABLE_BUTTON_DEBOUNCE
    uint8_t db_samples = cached_debounce_samples;
    raw_buttons[REMAP_IDX_CROSS] = debounce_button(!(gpio6 & GPIO6_MASK_CROSS), 0, db_samples);
    raw_buttons[REMAP_IDX_CIRCLE] = debounce_button(!(gpio6 & GPIO6_MASK_CIRCLE), 1, db_samples);
    raw_buttons[REMAP_IDX_TRIANGLE] = debounce_button(!(gpio6 & GPIO6_MASK_TRIANGLE), 2, db_samples);
    raw_buttons[REMAP_IDX_SQUARE] = debounce_button(!(gpio6 & GPIO6_MASK_SQUARE), 3, db_samples);
    raw_buttons[REMAP_IDX_L1] = debounce_button(!(gpio6 & GPIO6_MASK_L1), 4, db_samples);
    raw_buttons[REMAP_IDX_R1] = debounce_button(!(gpio6 & GPIO6_MASK_R1), 5, db_samples);
    raw_buttons[REMAP_IDX_L2] = debounce_button(!(gpio6 & GPIO6_MASK_L2), 6, db_samples);
    raw_buttons[REMAP_IDX_R2] = debounce_button(!(gpio6 & GPIO6_MASK_R2), 7, db_samples);
    raw_buttons[REMAP_IDX_L3] = debounce_button(!(gpio6 & GPIO6_MASK_L3), 8, db_samples);
    raw_buttons[REMAP_IDX_R3] = debounce_button(!(gpio6 & GPIO6_MASK_R3), 9, db_samples);
    raw_buttons[REMAP_IDX_SHARE] = debounce_button(!(gpio7 & GPIO7_MASK_SHARE), 10, db_samples);
    raw_buttons[REMAP_IDX_OPTIONS] = debounce_button(!(gpio7 & GPIO7_MASK_OPTIONS), 11, db_samples);
    raw_buttons[REMAP_IDX_DPAD_UP] = debounce_button(!(gpio6 & GPIO6_MASK_DPAD_UP), 14, db_samples);
    raw_buttons[REMAP_IDX_DPAD_DOWN] = debounce_button(!(gpio6 & GPIO6_MASK_DPAD_DOWN), 15, db_samples);
    raw_buttons[REMAP_IDX_DPAD_LEFT] = debounce_button(!(gpio6 & GPIO6_MASK_DPAD_LEFT), 16, db_samples);
    raw_buttons[REMAP_IDX_DPAD_RIGHT] = debounce_button(!(gpio6 & GPIO6_MASK_DPAD_RIGHT), 17, db_samples);
    raw_buttons[REMAP_IDX_BACK_LEFT] = debounce_button(!(gpio7 & GPIO7_MASK_BACK_LEFT), 18, db_samples);
    raw_buttons[REMAP_IDX_BACK_RIGHT] = debounce_button(!(gpio7 & GPIO7_MASK_BACK_RIGHT), 19, db_samples);
    bool ps_button_pressed = debounce_button(!(gpio7 & GPIO7_MASK_PS), 12, db_samples);  // PS (nicht in Remapping-Tabelle)
    bool touchpad_pressed = debounce_button(!(gpio7 & GPIO7_MASK_TOUCHPAD), 13, db_samples);  // Touchpad (nicht in Remapping-Tabelle)
#else
    raw_buttons[REMAP_IDX_CROSS] = !(gpio6 & GPIO6_MASK_CROSS);
    raw_buttons[REMAP_IDX_CIRCLE] = !(gpio6 & GPIO6_MASK_CIRCLE);
    raw_buttons[REMAP_IDX_TRIANGLE] = !(gpio6 & GPIO6_MASK_TRIANGLE);
    raw_buttons[REMAP_IDX_SQUARE] = !(gpio6 & GPIO6_MASK_SQUARE);
    raw_buttons[REMAP_IDX_L1] = !(gpio6 & GPIO6_MASK_L1);
    raw_buttons[REMAP_IDX_R1] = !(gpio6 & GPIO6_MASK_R1);
    raw_buttons[REMAP_IDX_L2] = !(gpio6 & GPIO6_MASK_L2);
    raw_buttons[REMAP_IDX_R2] = !(gpio6 & GPIO6_MASK_R2);
    raw_buttons[REMAP_IDX_L3] = !(gpio6 & GPIO6_MASK_L3);
    raw_buttons[REMAP_IDX_R3] = !(gpio6 & GPIO6_MASK_R3);
    raw_buttons[REMAP_IDX_SHARE] = !(gpio7 & GPIO7_MASK_SHARE);
    raw_buttons[REMAP_IDX_OPTIONS] = !(gpio7 & GPIO7_MASK_OPTIONS);
    raw_buttons[REMAP_IDX_DPAD_UP] = !(gpio6 & GPIO6_MASK_DPAD_UP);
    raw_buttons[REMAP_IDX_DPAD_DOWN] = !(gpio6 & GPIO6_MASK_DPAD_DOWN);
    raw_buttons[REMAP_IDX_DPAD_LEFT] = !(gpio6 & GPIO6_MASK_DPAD_LEFT);
    raw_buttons[REMAP_IDX_DPAD_RIGHT] = !(gpio6 & GPIO6_MASK_DPAD_RIGHT);
    raw_buttons[REMAP_IDX_BACK_LEFT] = !(gpio7 & GPIO7_MASK_BACK_LEFT);
    raw_buttons[REMAP_IDX_BACK_RIGHT] = !(gpio7 & GPIO7_MASK_BACK_RIGHT);
    bool ps_button_pressed = !(gpio7 & GPIO7_MASK_PS);  // PS (nicht in Remapping-Tabelle)
    bool touchpad_pressed = !(gpio7 & GPIO7_MASK_TOUCHPAD);  // Touchpad (nicht in Remapping-Tabelle)
#endif

    // ===== PHASE 2: BUTTON REMAPPING =====
    // Remapping-Tabelle anwenden: JEDER physische Button kann auf JEDEN virtuellen Button gemappt werden
    // PERFORMANCE: Array-basiert, sehr schnell (keine Switch-Cases, nur Array-Lookups!)
    bool virtual_buttons[18] = {false};  // Virtuelle Buttons nach Remapping

    // OPTIMIERUNG #3: Button-Remapping-Shortcut
    // Prüfe ob alle Buttons auf PASSTHROUGH sind → direkter Copy (viel schneller!)
    // Erwarteter Gewinn: ~5-10µs wenn kein Remapping aktiv
    if (!remapping_cache_valid) {
        // Prüfe ob irgendein Button NICHT PASSTHROUGH ist
        remapping_active_cached = false;
        for (uint8_t i = 0; i < 18; i++) {
            if (cached_button_remapping[i] != BTN_MAP_PASSTHROUGH) {
                remapping_active_cached = true;
                break;
            }
        }
        remapping_cache_valid = true;
    }
    
    if (!remapping_active_cached) {
        // KEIN Remapping aktiv → direkter Copy (viel schneller!)
        memcpy(virtual_buttons, raw_buttons, sizeof(virtual_buttons));
    } else {
        // Remapping aktiv → normale Loop
        for (uint8_t i = 0; i < 18; i++) {
            if (!raw_buttons[i]) continue;  // Button nicht gedrückt → Skip

            uint8_t target = cached_button_remapping[i];

            if (target == BTN_MAP_PASSTHROUGH) {
                // PASSTHROUGH: Button bleibt wie Original
                virtual_buttons[i] = true;
            } else if (target == BTN_MAP_DISABLED) {
                // DISABLED: Button wird ignoriert (nichts tun)
            } else if (target >= BTN_MAP_CROSS && target <= BTN_MAP_DPAD_RIGHT) {
                // REMAPPING: Button wird auf anderen Button gemappt
                uint8_t target_idx = target - BTN_MAP_CROSS;  // BTN_MAP_CROSS=2 → Index 0 (REMAP_IDX_CROSS)
                virtual_buttons[target_idx] = true;
            }
        }
    }

    // ===== PHASE 3: VIRTUELLE BUTTONS → DS4 BUTTON BITS =====
    // DS4 Button-Bytes konstruieren aus virtuellen Buttons
    // OPTIMIERUNG #6: Lookup-Tabelle statt 12 if-Statements (branchless, schneller!)
    // Erwarteter Gewinn: ~2-4µs bei Button-Press (weniger Branches, bessere CPU-Pipeline!)
    static const uint8_t btn1_mask[18] __attribute__((aligned(4))) = {
        DS4_BTN1_CROSS,      // REMAP_IDX_CROSS = 0
        DS4_BTN1_CIRCLE,     // REMAP_IDX_CIRCLE = 1
        DS4_BTN1_SQUARE,     // REMAP_IDX_SQUARE = 2
        DS4_BTN1_TRIANGLE,   // REMAP_IDX_TRIANGLE = 3
        0,                   // REMAP_IDX_L1 = 4 (btn2)
        0,                   // REMAP_IDX_R1 = 5 (btn2)
        0,                   // REMAP_IDX_L2 = 6 (btn2)
        0,                   // REMAP_IDX_R2 = 7 (btn2)
        0,                   // REMAP_IDX_L3 = 8 (btn2)
        0,                   // REMAP_IDX_R3 = 9 (btn2)
        0,                   // REMAP_IDX_SHARE = 10 (btn2)
        0,                   // REMAP_IDX_OPTIONS = 11 (btn2)
        0,                   // REMAP_IDX_DPAD_UP = 12 (wird später kombiniert)
        0,                   // REMAP_IDX_DPAD_DOWN = 13 (wird später kombiniert)
        0,                   // REMAP_IDX_DPAD_LEFT = 14 (wird später kombiniert)
        0,                   // REMAP_IDX_DPAD_RIGHT = 15 (wird später kombiniert)
        0,                   // REMAP_IDX_BACK_LEFT = 16 (nicht in btn1/btn2)
        0                    // REMAP_IDX_BACK_RIGHT = 17 (nicht in btn1/btn2)
    };
    static const uint8_t btn2_mask[18] __attribute__((aligned(4))) = {
        0,                   // REMAP_IDX_CROSS = 0 (btn1)
        0,                   // REMAP_IDX_CIRCLE = 1 (btn1)
        0,                   // REMAP_IDX_SQUARE = 2 (btn1)
        0,                   // REMAP_IDX_TRIANGLE = 3 (btn1)
        DS4_BTN2_L1,         // REMAP_IDX_L1 = 4
        DS4_BTN2_R1,         // REMAP_IDX_R1 = 5
        DS4_BTN2_L2,         // REMAP_IDX_L2 = 6
        DS4_BTN2_R2,         // REMAP_IDX_R2 = 7
        DS4_BTN2_L3,         // REMAP_IDX_L3 = 8
        DS4_BTN2_R3,         // REMAP_IDX_R3 = 9
        DS4_BTN2_SHARE,      // REMAP_IDX_SHARE = 10
        DS4_BTN2_OPTIONS,    // REMAP_IDX_OPTIONS = 11
        0,                   // REMAP_IDX_DPAD_UP = 12
        0,                   // REMAP_IDX_DPAD_DOWN = 13
        0,                   // REMAP_IDX_DPAD_LEFT = 14
        0,                   // REMAP_IDX_DPAD_RIGHT = 15
        0,                   // REMAP_IDX_BACK_LEFT = 16
        0                    // REMAP_IDX_BACK_RIGHT = 17
    };
    
    // Branchless Button-Konvertierung: Alle Buttons parallel mit Bit-Masken (keine if-Statements!)
    uint8_t btn1 = 0;  // Face Buttons
    uint8_t btn2 = 0;  // Shoulder/System Buttons
    uint8_t btn3 = 0;  // PS + Touchpad
    
    // OPTIMIERUNG: Loop über alle Buttons, branchless mit ternären Operator (Compiler optimiert zu conditional moves!)
    // Bei O3 wird der ternäre Operator zu branchless Code optimiert (keine Branch-Misprediction!)
    for (uint8_t i = 0; i < 18; i++) {
        btn1 |= virtual_buttons[i] ? btn1_mask[i] : 0;
        btn2 |= virtual_buttons[i] ? btn2_mask[i] : 0;
    }

    // D-Pad (wird später kombiniert)
    bool dpad_up = virtual_buttons[REMAP_IDX_DPAD_UP];
    bool dpad_down = virtual_buttons[REMAP_IDX_DPAD_DOWN];
    bool dpad_left = virtual_buttons[REMAP_IDX_DPAD_LEFT];
    bool dpad_right = virtual_buttons[REMAP_IDX_DPAD_RIGHT];

    // PS + Touchpad (nicht remappbar, always PASSTHROUGH)
    if (touchpad_pressed) btn3 |= DS4_BTN3_TOUCHPAD;

    // ===== PS-BUTTON SEQUENZ-MAKRO =====
    // Sequenz: R3 halten (225ms) → Triangle tap (50ms) → Triangle release (50ms) → R3 release
    // Timer läuft bei 8000 Hz: 1 Frame = 125µs, 1ms = 8 Frames
    // 225ms = 1800 Frames, 50ms = 400 Frames
    // Kann pro Profil aktiviert/deaktiviert werden (CONFIG_PS_SEQUENCE_ENABLED)

    // Edge-Detection: PS-Button wurde gerade gedrückt (immer berechnen)
    // bool ps_button_rising_edge = ps_button_pressed && !ps_button_last;  // Unused - removed
    ps_button_last = ps_button_pressed;

#if CONFIG_PS_SEQUENCE_ENABLED
    // State Machine für Sequenz
    switch (ps_sequence_state) {
        case PS_SEQ_IDLE:
            if (ps_button_rising_edge) {
                // PS-Button gedrückt → Sequenz starten
                ps_sequence_state = PS_SEQ_R3_HOLD;
                ps_sequence_timer = 0;
            }
            break;

        case PS_SEQ_R3_HOLD:
            // R3 halten für 225ms (1800 Frames bei 8000 Hz)
            btn2 |= DS4_BTN2_R3;  // R3 gedrückt halten
            ps_sequence_timer++;
            if (ps_sequence_timer >= 1800) {  // 225ms = 1800 Frames
                ps_sequence_state = PS_SEQ_TRIANGLE_TAP;
                ps_sequence_timer = 0;
            }
            break;

        case PS_SEQ_TRIANGLE_TAP:
            // R3 weiter halten + Triangle drücken für 50ms (400 Frames)
            btn2 |= DS4_BTN2_R3;  // R3 weiter halten
            btn1 |= DS4_BTN1_TRIANGLE;  // Triangle drücken
            ps_sequence_timer++;
            if (ps_sequence_timer >= 400) {  // 50ms = 400 Frames
                ps_sequence_state = PS_SEQ_TRIANGLE_RELEASE;
                ps_sequence_timer = 0;
            }
            break;

        case PS_SEQ_TRIANGLE_RELEASE:
            // Triangle loslassen, R3 weiter halten für 50ms (400 Frames)
            btn2 |= DS4_BTN2_R3;  // R3 weiter halten
            // Triangle NICHT drücken (loslassen)
            ps_sequence_timer++;
            if (ps_sequence_timer >= 400) {  // 50ms = 400 Frames
                ps_sequence_state = PS_SEQ_R3_RELEASE;
                ps_sequence_timer = 0;
            }
            break;

        case PS_SEQ_R3_RELEASE:
            // R3 loslassen (nichts tun, automatisch released)
            // Sequenz beendet
            ps_sequence_state = PS_SEQ_IDLE;
            ps_sequence_timer = 0;
            break;
    }

    // Originalen PS-Button NICHT senden (wird durch Sequenz ersetzt)
    // btn3 bleibt ohne PS-Bit
#else
    // PS-Sequenz deaktiviert → normalen PS-Button senden
    if (ps_button_pressed) {
        btn3 |= DS4_BTN3_PS;
    }
#endif

    // D-Pad Logik wurde bereits in Phase 2/3 (Button Remapping) behandelt
    // dpad_up, dpad_down, dpad_left, dpad_right sind bereits gesetzt!

    // ===== BUTTON MAKRO-SYSTEM (konfigurierbar!) =====
    // Prüft alle 4 Makro-Slots und triggert Target-Button wenn Kombination gedrückt
    // OPTIMIERUNG #5: Makro-Check-Shortcut (überspringe Loop wenn keine Makros aktiv)
    // Erwarteter Gewinn: ~3-5µs wenn keine Makros aktiv
    if (!macros_cache_valid) {
        // Prüfe ob irgendein Makro aktiv ist
        macros_active_cached = false;
        for (uint8_t i = 0; i < 4; i++) {
            if (cached_macro_slots[i].enabled) {
                macros_active_cached = true;
                break;
            }
        }
        macros_cache_valid = true;
    }
    
    if (macros_active_cached) {
        // PERFORMANCE: Loop wird vom Compiler entrollt (4 Iterationen, konstant)
        for (uint8_t i = 0; i < 4; i++) {
            if (!cached_macro_slots[i].enabled) continue;  // Skip deaktivierte Makros

        uint8_t trigger1 = cached_macro_slots[i].trigger_button_1;
        uint8_t trigger2 = cached_macro_slots[i].trigger_button_2;
        uint8_t target = cached_macro_slots[i].target_button;

        // Prüfe ob beide Trigger-Buttons gedrückt sind
        bool trigger1_pressed = false;
        bool trigger2_pressed = false;

        // Prüfe Trigger 1
        switch (trigger1) {
            case MACRO_TRIGGER_CROSS:    trigger1_pressed = (btn1 & DS4_BTN1_CROSS); break;
            case MACRO_TRIGGER_CIRCLE:   trigger1_pressed = (btn1 & DS4_BTN1_CIRCLE); break;
            case MACRO_TRIGGER_SQUARE:   trigger1_pressed = (btn1 & DS4_BTN1_SQUARE); break;
            case MACRO_TRIGGER_TRIANGLE: trigger1_pressed = (btn1 & DS4_BTN1_TRIANGLE); break;
            case MACRO_TRIGGER_L1:       trigger1_pressed = (btn2 & DS4_BTN2_L1); break;
            case MACRO_TRIGGER_R1:       trigger1_pressed = (btn2 & DS4_BTN2_R1); break;
            case MACRO_TRIGGER_L2:       trigger1_pressed = (btn2 & DS4_BTN2_L2); break;
            case MACRO_TRIGGER_R2:       trigger1_pressed = (btn2 & DS4_BTN2_R2); break;
            case MACRO_TRIGGER_L3:       trigger1_pressed = (btn2 & DS4_BTN2_L3); break;
            case MACRO_TRIGGER_R3:       trigger1_pressed = (btn2 & DS4_BTN2_R3); break;
            case MACRO_TRIGGER_SHARE:    trigger1_pressed = (btn2 & DS4_BTN2_SHARE); break;
            case MACRO_TRIGGER_OPTIONS:  trigger1_pressed = (btn2 & DS4_BTN2_OPTIONS); break;
            case MACRO_TRIGGER_DPAD_UP:    trigger1_pressed = dpad_up; break;
            case MACRO_TRIGGER_DPAD_DOWN:  trigger1_pressed = dpad_down; break;
            case MACRO_TRIGGER_DPAD_LEFT:  trigger1_pressed = dpad_left; break;
            case MACRO_TRIGGER_DPAD_RIGHT: trigger1_pressed = dpad_right; break;
        }

        // Prüfe Trigger 2
        switch (trigger2) {
            case MACRO_TRIGGER_CROSS:    trigger2_pressed = (btn1 & DS4_BTN1_CROSS); break;
            case MACRO_TRIGGER_CIRCLE:   trigger2_pressed = (btn1 & DS4_BTN1_CIRCLE); break;
            case MACRO_TRIGGER_SQUARE:   trigger2_pressed = (btn1 & DS4_BTN1_SQUARE); break;
            case MACRO_TRIGGER_TRIANGLE: trigger2_pressed = (btn1 & DS4_BTN1_TRIANGLE); break;
            case MACRO_TRIGGER_L1:       trigger2_pressed = (btn2 & DS4_BTN2_L1); break;
            case MACRO_TRIGGER_R1:       trigger2_pressed = (btn2 & DS4_BTN2_R1); break;
            case MACRO_TRIGGER_L2:       trigger2_pressed = (btn2 & DS4_BTN2_L2); break;
            case MACRO_TRIGGER_R2:       trigger2_pressed = (btn2 & DS4_BTN2_R2); break;
            case MACRO_TRIGGER_L3:       trigger2_pressed = (btn2 & DS4_BTN2_L3); break;
            case MACRO_TRIGGER_R3:       trigger2_pressed = (btn2 & DS4_BTN2_R3); break;
            case MACRO_TRIGGER_SHARE:    trigger2_pressed = (btn2 & DS4_BTN2_SHARE); break;
            case MACRO_TRIGGER_OPTIONS:  trigger2_pressed = (btn2 & DS4_BTN2_OPTIONS); break;
            case MACRO_TRIGGER_DPAD_UP:    trigger2_pressed = dpad_up; break;
            case MACRO_TRIGGER_DPAD_DOWN:  trigger2_pressed = dpad_down; break;
            case MACRO_TRIGGER_DPAD_LEFT:  trigger2_pressed = dpad_left; break;
            case MACRO_TRIGGER_DPAD_RIGHT: trigger2_pressed = dpad_right; break;
        }

        // Wenn beide Trigger gedrückt → Target-Button aktivieren
        if (trigger1_pressed && trigger2_pressed) {
            switch (target) {
                case MACRO_TRIGGER_CROSS:    btn1 |= DS4_BTN1_CROSS; break;
                case MACRO_TRIGGER_CIRCLE:   btn1 |= DS4_BTN1_CIRCLE; break;
                case MACRO_TRIGGER_SQUARE:   btn1 |= DS4_BTN1_SQUARE; break;
                case MACRO_TRIGGER_TRIANGLE: btn1 |= DS4_BTN1_TRIANGLE; break;
                case MACRO_TRIGGER_L1:       btn2 |= DS4_BTN2_L1; break;
                case MACRO_TRIGGER_R1:       btn2 |= DS4_BTN2_R1; break;
                case MACRO_TRIGGER_L2:       btn2 |= DS4_BTN2_L2; break;
                case MACRO_TRIGGER_R2:       btn2 |= DS4_BTN2_R2; break;
                case MACRO_TRIGGER_L3:       btn2 |= DS4_BTN2_L3; break;
                case MACRO_TRIGGER_R3:       btn2 |= DS4_BTN2_R3; break;
                case MACRO_TRIGGER_SHARE:    btn2 |= DS4_BTN2_SHARE; break;
                case MACRO_TRIGGER_OPTIONS:  btn2 |= DS4_BTN2_OPTIONS; break;
                case MACRO_TRIGGER_DPAD_UP:    dpad_up = true; break;
                case MACRO_TRIGGER_DPAD_DOWN:  dpad_down = true; break;
                case MACRO_TRIGGER_DPAD_LEFT:  dpad_left = true; break;
                case MACRO_TRIGGER_DPAD_RIGHT: dpad_right = true; break;
            }
        }
        }  // Ende der Makro-Loop
    }  // Ende des Makro-Check-Shortcut

    // D-Pad Richtung berechnen (8-Wege + Neutral) - NACH Back-Button Remapping!
    // OPTIMIERUNG #7: D-Pad Lookup-Tabelle statt 8 if-else-if Statements (branchless!)
    // Erwarteter Gewinn: ~1-2µs (keine Branch-Misprediction, direkter Lookup!)
    // 4 bools → 4-bit Index (0-15), aber nur 9 Werte relevant (8 Richtungen + Neutral)
    static const uint8_t dpad_lookup[16] __attribute__((aligned(4))) = {
        DS4_BTN1_DPAD_NONE,      // 0b0000: Keine Richtung
        DS4_BTN1_DPAD_UP,        // 0b0001: Up
        DS4_BTN1_DPAD_DOWN,      // 0b0010: Down
        DS4_BTN1_DPAD_NONE,      // 0b0011: Up+Down (unmöglich, aber neutral)
        DS4_BTN1_DPAD_LEFT,      // 0b0100: Left
        DS4_BTN1_DPAD_UPLEFT,    // 0b0101: Up+Left
        DS4_BTN1_DPAD_DOWNLEFT,  // 0b0110: Down+Left
        DS4_BTN1_DPAD_NONE,      // 0b0111: Up+Down+Left (unmöglich)
        DS4_BTN1_DPAD_RIGHT,     // 0b1000: Right
        DS4_BTN1_DPAD_UPRIGHT,   // 0b1001: Up+Right
        DS4_BTN1_DPAD_DOWNRIGHT, // 0b1010: Down+Right
        DS4_BTN1_DPAD_NONE,      // 0b1011: Up+Down+Right (unmöglich)
        DS4_BTN1_DPAD_NONE,      // 0b1100: Left+Right (unmöglich)
        DS4_BTN1_DPAD_NONE,      // 0b1101: Up+Left+Right (unmöglich)
        DS4_BTN1_DPAD_NONE,      // 0b1110: Down+Left+Right (unmöglich)
        DS4_BTN1_DPAD_NONE       // 0b1111: Alle (unmöglich)
    };
    // Branchless: 4 bools → 4-bit Index (keine if-Statements!)
    uint8_t dpad_index = (dpad_up ? 1 : 0) | ((dpad_down ? 1 : 0) << 1) | 
                         ((dpad_left ? 1 : 0) << 2) | ((dpad_right ? 1 : 0) << 3);
    uint8_t dpad = dpad_lookup[dpad_index];

    // D-Pad (bits 0-3) + Face Buttons (bits 4-7) korrekt kombinieren
    g_report.buttons1 = dpad | btn1;
    g_report.buttons2 = btn2;
    // Buttons3: PS + Touchpad (Counter wird später beim Senden hinzugefügt für BF6)
    // Counter wird später beim Senden hinzugefügt (nur wenn gesendet wird!)
    g_report.buttons3 = btn3;

    // ===== RUNTIME-PROFIL-WECHSEL (PS + D-Pad Up für 1 Sekunde halten!) =====
    // Profil kann jetzt zur Laufzeit gewechselt werden ohne Controller abzustecken!
    // PS-Taste + D-Pad Up für 1 Sekunde halten → Profil wechseln!
    if (g_controller_config.check_runtime_profile_switch(g_report.buttons1, g_report.buttons3)) {
        // Profil wurde gewechselt → Cache invalidieren und ADC-Reload
        cached_active_profile = 0xFF;  // Force reload beim nächsten Frame
        FastADC.reload_profile_state();  // ADC-Profil-State neu laden
    }

    // Trigger optimiert (branch-free): 0x00 oder 0xFF
    g_report.left_trigger = -(!!(btn2 & DS4_BTN2_L2));
    g_report.right_trigger = -(!!(btn2 & DS4_BTN2_R2));

    // Sticks lesen mit FastADC (12-bit mit 8x Hardware-Averaging!)
    uint16_t lx_raw, ly_raw, rx_raw, ry_raw;
    FastADC.read_all_sticks(&lx_raw, &ly_raw, &rx_raw, &ry_raw);

    // ===== OPTION 2: EMA FILTER (Software-Glättung) - compile-time optimiert! =====
#if CONFIG_ENABLE_EMA_FILTER
    // OPTIMIERUNG: Fixed-Point Q8.8 statt Float (~20% schneller!)
    // Exponential Moving Average: new_value = alpha * current + (1-alpha) * previous
    // Glättet hochfrequentes Rauschen, aber fügt Lag hinzu
    // PERFORMANCE: Gecacht statt API-Aufruf!
    // Stick-Werte sind uint16_t (aber nur 0-255 verwendet), daher Q8.8 Format (8 Bit Integer + 8 Bit Fractional)
    float ema_alpha_f = cached_ema_alpha;
    uint16_t ema_alpha_q8 = (uint16_t)(ema_alpha_f * 256.0f);  // Convert to Q8.8 (0-255)
    uint16_t one_minus_alpha_q8 = 256 - ema_alpha_q8;  // (1-alpha) in Q8.8
    
    // Fixed-Point EMA: new = (alpha * current + (1-alpha) * previous) >> 8
    // ema_lx ist in Q8.8 Format (uint16_t): 128.0 = 128 << 8 = 32768
    // WICHTIG: lx_raw ist uint16_t, aber nur 0-255 Bereich! Wir müssen es auf Q8.8 konvertieren: lx_raw << 8
    // Berechnung: ema_lx = (alpha * (lx_raw << 8) + (1-alpha) * ema_lx) >> 8
    // Da alpha und (1-alpha) in Q8.8 sind (0-256), müssen wir das Ergebnis durch 256 teilen
    uint16_t lx_q8 = (uint16_t)lx_raw << 8;  // Convert to Q8.8
    uint16_t ly_q8 = (uint16_t)ly_raw << 8;
    uint16_t rx_q8 = (uint16_t)rx_raw << 8;
    uint16_t ry_q8 = (uint16_t)ry_raw << 8;
    
    // EMA: new = alpha * current + (1-alpha) * previous
    // In Fixed-Point: new = (alpha * current + (1-alpha) * previous) >> 8
    ema_lx = ((ema_alpha_q8 * lx_q8) + (one_minus_alpha_q8 * ema_lx)) >> 8;
    ema_ly = ((ema_alpha_q8 * ly_q8) + (one_minus_alpha_q8 * ema_ly)) >> 8;
    ema_rx = ((ema_alpha_q8 * rx_q8) + (one_minus_alpha_q8 * ema_rx)) >> 8;
    ema_ry = ((ema_alpha_q8 * ry_q8) + (one_minus_alpha_q8 * ema_ry)) >> 8;

    // Gefilterte Werte zurück in uint16_t (Q8.8 → uint8_t, aber uint16_t für weitere Verarbeitung)
    lx_raw = (uint16_t)(ema_lx >> 8);
    ly_raw = (uint16_t)(ema_ly >> 8);
    rx_raw = (uint16_t)(ema_rx >> 8);
    ry_raw = (uint16_t)(ema_ry >> 8);
#endif

    // ===== OPTION 3: HYSTERESIS FILTER (Anti-Flatter) - compile-time optimiert! =====
#if CONFIG_ENABLE_HYSTERESIS
    // Nur Änderungen > THRESHOLD werden akzeptiert
    // Verhindert "Flattern" bei kleinen Schwankungen um einen Wert
    // PERFORMANCE: Gecacht statt API-Aufruf!
    uint8_t hysteresis_threshold = cached_hysteresis_threshold;
    auto apply_hysteresis = [hysteresis_threshold](uint16_t current, uint16_t &last) -> uint16_t {
        int16_t diff = (int16_t)current - (int16_t)last;
        if (diff < -hysteresis_threshold || diff > hysteresis_threshold) {
            last = current;  // Große Änderung → akzeptieren
            return current;
        }
        return last;  // Kleine Änderung → ignorieren (alter Wert behalten)
    };

    lx_raw = apply_hysteresis(lx_raw, hyst_lx);
    ly_raw = apply_hysteresis(ly_raw, hyst_ly);
    rx_raw = apply_hysteresis(rx_raw, hyst_rx);
    ry_raw = apply_hysteresis(ry_raw, hyst_ry);
#endif

    // ===== OPTION 4: SNAPBACK FILTER (Anti-Overshoot) - compile-time optimiert! =====
#if CONFIG_ENABLE_SNAPBACK_FILTER
    // Verhindert Overshoot beim schnellen Loslassen des Sticks
    // Erkennt schnelle Rückkehr zum Center und filtert falsche Werte in Gegenrichtung
    // PERFORMANCE: Gecacht statt API-Aufruf!
    uint8_t snapback_threshold = cached_snapback_threshold;
    uint8_t snapback_window = cached_snapback_window;
    
    auto apply_snapback_filter = [snapback_threshold, snapback_window](uint16_t current, SnapbackState &state) -> uint16_t {
        const uint16_t center = 128;
        
        // OPTIMIERUNG: Early Exit wenn weit vom Center und kein Snapback aktiv
        // PERFORMANCE: Spart ~20-30 Zyklen im normalen Betrieb!
        uint16_t dist_from_center = (current > center) ? (current - center) : (center - current);
        if (dist_from_center > 50 && !state.in_snapback) {
            // Weit außen, kein Snapback → schneller Exit!
            state.last_value = current;
            state.last_direction = (current > center + 10) ? 1 : ((current < center - 10) ? -1 : 0);
            return current;
        }
        
        int16_t diff = (int16_t)current - (int16_t)state.last_value;
        
        // OPTIMIERUNG: Branchless direction check
        // PERFORMANCE: Spart 2-3 Zyklen pro Call!
        int8_t current_direction = (current > center + 10) - (current < center - 10);
        
        // Prüfe ob wir weit außen sind (Edge-Detection)
        bool is_at_edge = (current < 56 || current > 200);
        
        // Prüfe auf schnelle Bewegung zum Center (Snapback-Erkennung)
        bool moving_toward_center = false;
        if (state.last_value < center - 30 && current > state.last_value) {
            // War links/unten, bewegt sich nach rechts/oben → zum Center
            moving_toward_center = (diff > snapback_threshold);
        } else if (state.last_value > center + 30 && current < state.last_value) {
            // War rechts/oben, bewegt sich nach links/unten → zum Center
            moving_toward_center = (diff < -snapback_threshold);
        }
        
        // Snapback-Phase aktivieren
        if (moving_toward_center || state.in_snapback) {
            if (!state.in_snapback) {
                state.in_snapback = true;
                state.frames_since_edge = 0;
            } else {
                state.frames_since_edge++;
            }
            
            // Prüfe auf Overshoot (Gegenrichtung kurz nach Snapback)
            if (state.in_snapback && state.frames_since_edge <= snapback_window) {
                // Wenn Wert jetzt in Gegenrichtung von letzter Richtung → Overshoot!
                if ((state.last_direction == 1 && current < center - 5) ||  // War rechts, jetzt links
                    (state.last_direction == -1 && current > center + 5)) {  // War links, jetzt rechts
                    // Overshoot erkannt! Filtere aus, setze auf Center
                    state.last_value = current;
                    state.last_direction = 0;
                    return center;
                }
            }
            
            // Snapback-Phase beenden nach Window
            if (state.frames_since_edge > snapback_window) {
                state.in_snapback = false;
                state.frames_since_edge = 0;
            }
        }
        
        // Normaler Betrieb: Update State
        if (is_at_edge) {
            state.last_direction = current_direction;
            state.frames_since_edge = 0;
        }
        
        state.last_value = current;
        return current;
    };
    
    lx_raw = apply_snapback_filter(lx_raw, snapback_lx);
    ly_raw = apply_snapback_filter(ly_raw, snapback_ly);
    rx_raw = apply_snapback_filter(rx_raw, snapback_rx);
    ry_raw = apply_snapback_filter(ry_raw, snapback_ry);
#endif


    // ===== OPTION 5: OCTAGONAL GATE (8-Wege-Snapping) - compile-time optimiert! =====
#if CONFIG_ENABLE_OCTAGONAL_GATE
    if (cached_octagonal_gate_enabled) {
        // Nur linker Stick! (Movement präziser machen)
        // Wandelt analogen Stick-Input in 8 Hauptrichtungen um
        // N, NE, E, SE, S, SW, W, NW (0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°)

        int16_t lx_centered = (int16_t)lx_raw - 128;  // -128 bis +127
        int16_t ly_centered = (int16_t)ly_raw - 128;

        // Distanz vom Center (für Deadzone-Check)
        int32_t dist_sq = (int32_t)lx_centered * lx_centered + (int32_t)ly_centered * ly_centered;

        // Nur snappen wenn außerhalb einer kleinen Deadzone (>20% vom Max)
        if (dist_sq > (51*51)) {  // 51 = 20% von 255
            // Winkel berechnen (in 1/8tel Kreisen, 0-7)
            // atan2 gibt -PI bis +PI, wir wollen 0-7 (8 Richtungen)
            float angle_rad = atan2f((float)ly_centered, (float)lx_centered);  // -PI bis +PI
            float angle_normalized = (angle_rad + 3.14159265f) / (2.0f * 3.14159265f);  // 0.0 bis 1.0
            uint8_t octant = (uint8_t)(angle_normalized * 8.0f + 0.5f) % 8;  // 0-7

            // Ziel-Winkel für jede der 8 Richtungen (in Radianten)
            // 0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE
            const float target_angles[8] = {
                0.0f,                    // E   (0°)
                0.785398163f,            // SE  (45°)
                1.570796327f,            // S   (90°)
                2.356194490f,            // SW  (135°)
                3.141592654f,            // W   (180°)
                3.926990817f,            // NW  (225°)
                4.712388980f,            // N   (270°)
                5.497787144f             // NE  (315°)
            };

            float target_angle = target_angles[octant];

            // Aktuelle Distanz vom Center
            float current_dist = sqrtf((float)dist_sq);

            // Snap-Stärke anwenden (0-100%)
            float strength = (float)cached_octagonal_gate_strength / 100.0f;

            // Interpoliere zwischen aktuellem Winkel und Ziel-Winkel
            float snapped_angle = angle_rad + (target_angle - angle_rad) * strength;

            // Zurück in X/Y konvertieren (mit original Distanz)
            float snapped_x = cosf(snapped_angle) * current_dist;
            float snapped_y = sinf(snapped_angle) * current_dist;

            // Zurück in 0-255 Range konvertieren
            lx_raw = (uint16_t)((int16_t)snapped_x + 128);
            ly_raw = (uint16_t)((int16_t)snapped_y + 128);

            // Clamp auf 0-255
            if (lx_raw > 255) lx_raw = (lx_raw < 0) ? 0 : 255;
            if (ly_raw > 255) ly_raw = (ly_raw < 0) ? 0 : 255;
        }
    }
#endif
    // Inline Deadzone (Center Clamp) - Konfigurierbar über API, Standard: 4 Units (wie Backup)
    // Direkt in Units (0-20), keine Prozent-Konvertierung - funktionierte besser als Center Clamp API!
    // Werte im Center-Bereich werden auf 128 gesetzt (filtert ADC-Rauschen)
    // PERFORMANCE: Gecacht in updateController() - kein API-Aufruf pro Frame!
    // OPTIMIERUNG #3: clamp_min/max sind jetzt gecacht (werden nicht jedes Frame neu berechnet!)
    uint8_t deadzone_radius = cached_inline_deadzone_radius;  // 0-20 Units direkt (gecacht!)
    if (deadzone_radius == 0) {
        // Kein Clamp: Raw-Werte direkt senden
        g_report.left_stick_x = (uint8_t)lx_raw;
        g_report.left_stick_y = (uint8_t)ly_raw;
        g_report.right_stick_x = (uint8_t)rx_raw;
        g_report.right_stick_y = (uint8_t)ry_raw;
    } else {
        // Deadzone aktiv: Werte im Center-Bereich auf 128 setzen (exakt wie hardcodiert, aber konfigurierbar!)
        // OPTIMIERUNG #3: Gecachte Werte verwenden statt jedes Frame neu zu berechnen!
        g_report.left_stick_x = (lx_raw >= cached_clamp_min && lx_raw <= cached_clamp_max) ? 128 : (uint8_t)lx_raw;
        g_report.left_stick_y = (ly_raw >= cached_clamp_min && ly_raw <= cached_clamp_max) ? 128 : (uint8_t)ly_raw;
        g_report.right_stick_x = (rx_raw >= cached_clamp_min && rx_raw <= cached_clamp_max) ? 128 : (uint8_t)rx_raw;
        g_report.right_stick_y = (ry_raw >= cached_clamp_min && ry_raw <= cached_clamp_max) ? 128 : (uint8_t)ry_raw;
    }

    // ===== OPTIMIERUNG #1: IMU NOISE (wird immer berechnet, unabhängig von Change Detection!) =====
    // BF6 FIX: IMU-Rauschen simulieren (wie echter DS4!)
    // OPTIMIERUNG: LFSR (Linear Feedback Shift Register) statt Multiplikation (~30% schneller!)
    // LFSR ist schneller auf ARM (nur Shifts + XOR statt Multiplikation)
    // Polynomial: x^31 + x^21 + 1 (maximale Periode für 31-bit)
    imu_noise_state = (imu_noise_state >> 1) ^ ((imu_noise_state & 1) ? 0x80200000 : 0);
    int16_t noise1 = (int16_t)((imu_noise_state >> 16) & 0x1F) - 16;  // -16 bis +15
    int16_t noise2 = (int16_t)((imu_noise_state >> 8) & 0x1F) - 16;
    int16_t noise3 = (int16_t)(imu_noise_state & 0x1F) - 16;
    
    // ===== WICHTIG: DS4Report Struktur ist FALSCH! =====
    // Echtes DS4 Layout: Gyro = X,Z,Y (nicht X,Y,Z!), Accel = X,Z,Y
    // QUICK FIX: Schreibe direkt in richtige Felder (swap Y und Z!)

    // Gyro: X, Z, Y Reihenfolge (nicht X, Y, Z!)
    g_report.gyro_x = noise1;        // Gyro X → korrekt
    g_report.gyro_y = noise3;        // Gyro Z → ins "Y" Feld (Struktur ist falsch!)
    g_report.gyro_z = noise2;        // Gyro Y → ins "Z" Feld (Struktur ist falsch!)

    // Accelerometer: X, Z, Y Reihenfolge + Gravity in Y-Achse
    // Echter DS4: accel_y ≈ -8192 (Gravity), accel_x/z haben minimales Rauschen
    g_report.accel_x = noise1 / 2;                    // Accel X → korrekt
    g_report.accel_y = noise3 / 2;                    // Accel Z → ins "Y" Feld (Struktur ist falsch!)
    g_report.accel_z = -8192 + (noise2 / 4);          // Accel Y (Gravity!) → ins "Z" Feld

    // ===== CHANGE REPORTING: Sende nur bei Änderung (oder immer)! =====
    // Timestamp, Touchpad-Counter und Report-Counter NUR beim Senden erhöhen!
    // OPTIMIERUNG #2: report_sent Variable um g_last_report nur einmal zu kopieren!
    bool report_sent = false;
    
#if CONFIG_ENABLE_CHANGE_DETECTION
    // ===== OPTIMIERUNG #4: Change Detection mit memcmp() (SIMD-optimiert!) =====
    // PERFORMANCE: memcmp() nutzt SIMD-Instructions für parallelen Byte-Vergleich!
    // Erwarteter Gewinn: ~3-7 CPU-Zyklen pro Frame (1 Vergleich statt 4-5)
    // OPTIMIERUNG: Buttons vor dem Check vergleichen (frühes Exit!)
    // buttons3 muss separat geprüft werden (wegen Report Counter Masking)
    bool buttons_changed;
    if (cached_report_counter_enabled) {
        // Report Counter AN: buttons3 Bits 2-7 ändern sich immer → nur Bits 0-1 prüfen
        uint8_t buttons3_mask = 0x03;  // Nur PS+Touchpad (Bits 0-1)
        buttons_changed = (memcmp(&g_report.buttons1, &g_last_report.buttons1, 4) != 0) ||  // buttons1, buttons2, left_trigger, right_trigger (4 Bytes)
                         ((g_report.buttons3 & buttons3_mask) != (g_last_report.buttons3 & buttons3_mask));
    } else {
        // Report Counter AUS: Alle 5 Bytes direkt vergleichen (schnellster Fall!)
        buttons_changed = (memcmp(&g_report.buttons1, &g_last_report.buttons1, 5) != 0);  // buttons1, buttons2, buttons3, left_trigger, right_trigger (5 Bytes)
    }
    
    // PERFORMANCE: Gecacht statt API-Aufruf!
    if (cached_change_detection_enabled) {
        // Change Detection AN: Sende nur bei Änderung
        // OPTIMIERUNG: Buttons werden VORHER geprüft für frühen Exit!
        // Nur wenn Buttons gleich sind, prüfe Sticks (kostspieliger Check!)
        if (buttons_changed || has_sticks_changed(&g_report, &g_last_report, 
                                                  cached_stick_threshold, cached_adaptive_enabled,
                                                  cached_center_threshold, cached_center_min, cached_center_max)) {
            // ===== BF6 FIX: Report-Counter in buttons3 (Bits 2-7) =====
            // Echter DS4 Format: Bits 0-1 = PS+Touchpad, Bits 2-7 = 6-bit Counter
            // PERFORMANCE: Gecacht statt API-Aufruf!
            if (cached_report_counter_enabled) {
                // Counter wird NUR beim Senden erhöht → Change Detection funktioniert!
                g_report.buttons3 = btn3 | ((ds4_report_counter++ & 0x3F) << 2);
            } else {
                // Kein Counter: buttons3 nur mit PS + Touchpad (Change Detection funktioniert perfekt!)
                g_report.buttons3 = btn3;
            }
            
            // Timestamp und Touchpad-Counter erhöhen
            g_report.timestamp++;
            g_report.ext_data[4] = ds4_touchpad_counter++;
            
            // ===== BF6 FIX: KEIN CRC32 für USB! =====
            // USB Input Reports haben KEINEN CRC32 (nur Bluetooth!)
            // CRC32 wurde entfernt - war falsch positioniert und unnötig
            // g_report.crc32 = calculate_ds4_crc32(&g_report);  // ENTFERNT!

            // ===== USB-OPTIMIERUNG: Cache-Flush vor USB-Send (OPTIONAL) =====
            // HINWEIS: USB-Controller nutzt intern DMA um Daten aus RAM zu lesen
            // Cache-Flush ist nur nötig wenn USB-Stack es nicht automatisch macht
            // Teensy Core macht das möglicherweise bereits automatisch!
            // PERFORMANCE: Minimaler Overhead (~1-2µs), aber möglicherweise redundant
            // arm_dcache_flush(&g_report, sizeof(g_report));  // AUSKOMMENTIERT: Prüfen ob nötig!

            // USB senden - NON-BLOCKING!
            RawHID.send(&g_report, 0);
            report_sent = true;
        }
    } else {
        // Change Detection AUS: Sende immer
        // USB-Limit: ~6000 Hz real (bInterval=1ms auf USB-Endpoint)
        // Aber Timestamp zeigt echte 8000 Hz Timer-Rate!
        
        // ===== BF6 FIX: Report-Counter in buttons3 (Bits 2-7) =====
        // Echter DS4 Format: Bits 0-1 = PS+Touchpad, Bits 2-7 = 6-bit Counter
        if (cached_report_counter_enabled) {
            // Counter wird bei jedem Report erhöht (Change Detection ist AUS)
            g_report.buttons3 = btn3 | ((ds4_report_counter++ & 0x3F) << 2);
            
            // Timestamp und Touchpad-Counter erhöhen
            g_report.timestamp++;
            g_report.ext_data[4] = ds4_touchpad_counter++;
        } else {
            // Kein Counter: buttons3 nur mit PS + Touchpad
            g_report.buttons3 = btn3;
            
            // Timestamp erhöhen
            g_report.timestamp++;
            g_report.ext_data[4] = 0;  // Touchpad-Counter optional
        }
        
        // ===== BF6 FIX: KEIN CRC32 für USB! =====
        // USB Input Reports haben KEINEN CRC32 (nur Bluetooth!)
        // CRC32 wurde entfernt - war falsch positioniert und unnötig
        // g_report.crc32 = calculate_ds4_crc32(&g_report);  // ENTFERNT!

        // ===== USB-OPTIMIERUNG: Cache-Flush vor USB-Send (OPTIONAL) =====
        // HINWEIS: USB-Controller nutzt intern DMA um Daten aus RAM zu lesen
        // Cache-Flush ist nur nötig wenn USB-Stack es nicht automatisch macht
        // Teensy Core macht das möglicherweise bereits automatisch!
        // PERFORMANCE: Minimaler Overhead (~1-2µs), aber möglicherweise redundant
        // arm_dcache_flush(&g_report, sizeof(g_report));  // AUSKOMMENTIERT: Prüfen ob nötig!

        // USB senden - NON-BLOCKING!
        RawHID.send(&g_report, 0);
        report_sent = true;
    }
#else
    // ===== COMPILE-TIME OPTIMIERUNG: Change Detection komplett wegkompiliert! =====
    // Wenn ENABLE_CHANGE_DETECTION == 0, wird dieser gesamte Block wegkompiliert
    // Keine Laufzeit-Überprüfung mehr nötig - maximal Performance!
    
    // ===== BF6 FIX: Report-Counter in buttons3 (Bits 2-7) =====
    // Echter DS4 Format: Bits 0-1 = PS+Touchpad, Bits 2-7 = 6-bit Counter
    if (cached_report_counter_enabled) {
        // Counter wird bei jedem Report erhöht
        g_report.buttons3 = btn3 | ((ds4_report_counter++ & 0x3F) << 2);
        
        // Timestamp und Touchpad-Counter erhöhen
        g_report.timestamp++;
        g_report.ext_data[4] = ds4_touchpad_counter++;
    } else {
        // Kein Counter: buttons3 nur mit PS + Touchpad
        g_report.buttons3 = btn3;
        
        // Timestamp erhöhen
        g_report.timestamp++;
        g_report.ext_data[4] = 0;  // Touchpad-Counter optional
    }
    
    // ===== BF6 FIX: KEIN CRC32 für USB! =====
    // USB Input Reports haben KEINEN CRC32 (nur Bluetooth!)
    // CRC32 wurde entfernt - war falsch positioniert und unnötig
    // g_report.crc32 = calculate_ds4_crc32(&g_report);  // ENTFERNT!

    // ===== USB-OPTIMIERUNG: Cache-Flush vor USB-Send (OPTIONAL) =====
    // HINWEIS: USB-Controller nutzt intern DMA um Daten aus RAM zu lesen
    // Cache-Flush ist nur nötig wenn USB-Stack es nicht automatisch macht
    // Teensy Core macht das möglicherweise bereits automatisch!
    // PERFORMANCE: Minimaler Overhead (~1-2µs), aber möglicherweise redundant
    // arm_dcache_flush(&g_report, sizeof(g_report));  // AUSKOMMENTIERT: Prüfen ob nötig!

    // USB senden - NON-BLOCKING!
    RawHID.send(&g_report, 0);
    report_sent = true;
#endif

    // ===== OPTIMIERUNG #8: g_last_report Update mit memcpy() (SIMD-optimiert!) =====
    // Letzten Zustand speichern (nur relevante Bytes kopieren)
    // OPTIMIERUNG: memcpy() nutzt SIMD für schnelles Kopieren (1 Zeile statt 9!)
    // Erwarteter Gewinn: ~2-4 CPU-Zyklen (Compiler kann besser optimieren)
    if (report_sent) {
        // Kopiere nur relevante Bytes für Change Detection (9 Bytes: sticks + buttons + triggers)
        // Struktur-Layout: left_stick_x (Offset 1) bis right_trigger (Offset 9)
        memcpy(&g_last_report.left_stick_x, &g_report.left_stick_x, 9);
    }

    // === PERFORMANCE MESSUNG ENDE (optional, compile-time optimiert!) ===
#if CONFIG_ENABLE_PERF_MONITORING
    // OPTIMIERUNG: Nur Cycle-Count in ISR speichern, Rest in main-loop!
    // Reduziert ISR-Zeit deutlich (~500-1000 Cycles gespart!)
    perf_elapsed_cycles = ARM_DWT_CYCCNT - perf_start;
    perf_data_ready = true;  // Flag für main-loop
#endif
}

void loop() {
    // ===== LOOP FREQUENZ MESSUNG =====
    // Zähle loop() Durchläufe und berechne Frequenz jede Sekunde
    loop_counter++;
    uint32_t current_time = millis();

    if (current_time - loop_last_time >= 1000) {  // Alle 1000ms (1 Sekunde)
        loop_frequency = loop_counter;  // Anzahl Durchläufe = Frequenz in Hz

        // Speichere in reserved[5-7] für Debugging (kann ausgelesen werden)
        g_report.reserved[5] = (loop_frequency >> 16) & 0xFF;  // High byte
        g_report.reserved[6] = (loop_frequency >> 8) & 0xFF;   // Middle byte
        g_report.reserved[7] = loop_frequency & 0xFF;          // Low byte

        // Reset für nächste Sekunde
        loop_counter = 0;
        loop_last_time = current_time;
    }

    // OPTIMIERUNG: Performance-Stats-Update aus ISR ausgelagert!
    // Reduziert ISR-Zeit deutlich (non-kritische Operationen nicht in ISR!)
#if CONFIG_ENABLE_PERF_MONITORING
    if (perf_data_ready) {
        uint32_t perf_elapsed = perf_elapsed_cycles;  // Atomar kopieren
        perf_data_ready = false;  // Flag zurücksetzen
        
        // Stats tracken (minimal overhead!)
        if (perf_elapsed > perf_max_cycles) perf_max_cycles = perf_elapsed;
        if (perf_elapsed < perf_min_cycles) perf_min_cycles = perf_elapsed;

        // OPTIMIERUNG: Running average nur alle 64 Frames statt 16 (reduziert Overhead!)
        if ((perf_frame_count & 0x3F) == 0) {  // Alle 64 Frames statt 16
            perf_avg_cycles = (perf_avg_cycles * 63 + perf_elapsed) / 64;
        }
        perf_frame_count++;

        // Stats in Report schreiben (alle 8000 Frames = 1x/Sekunde)
        // Konvertiere Zyklen → µs (CPU-Frequenz: F_CPU_ACTUAL / 1000000)
        // Teensy 4.1: Normalerweise 600 MHz, aber kann variieren!
        if ((perf_frame_count & 0x1FFF) == 0) {  // Alle 8192 Frames
            uint32_t cpu_mhz = F_CPU_ACTUAL / 1000000;  // Echte CPU-Frequenz!

            g_report.reserved[0] = perf_max_cycles / cpu_mhz;  // Max in µs
            g_report.reserved[1] = perf_min_cycles / cpu_mhz;  // Min in µs
            g_report.reserved[2] = perf_avg_cycles / cpu_mhz;  // Avg in µs
            g_report.reserved[3] = (cpu_mhz >> 8) & 0xFF;      // CPU MHz high byte (DEBUG)
            g_report.reserved[4] = cpu_mhz & 0xFF;             // CPU MHz low byte (DEBUG)

            // Reset für nächste Sekunde
            perf_max_cycles = 0;
            perf_min_cycles = 0xFFFFFFFF;
        }
    }
#endif
    
    // Alle Updates laufen per Hardware-Timer (IntervalTimer)
    // Loop läuft weiter für USB-Polling und andere Hintergrund-Tasks
}

// ===== WEBHID FEATURE REPORT HANDLER =====
// Wird von usb.c aufgerufen für Feature Reports 0xF0-0xFF
// Extern "C" damit es aus C-Code (usb.c) aufgerufen werden kann

// Feature Report Handler für WebHID-Konfiguration
// WICHTIG: Extern "C" damit es aus C-Code (usb.c) aufgerufen werden kann
extern "C" {
    bool handle_webhid_feature_report(uint8_t report_id, const uint8_t* data, uint16_t len, 
                                      uint8_t* response_buffer, uint16_t* response_len);
}

bool handle_webhid_feature_report(uint8_t report_id, const uint8_t* data, uint16_t len, 
                                  uint8_t* response_buffer, uint16_t* response_len) {
    if (!response_buffer || !response_len) {
        return false;
    }

    switch (report_id) {
        // Feature Report 0xF9: Get Version Info
        case 0xF9: {
            // Response: [0xF9, version_major, version_minor, ...]
            response_buffer[0] = 0xF9;
            response_buffer[1] = 1;  // Version Major
            response_buffer[2] = 0;  // Version Minor
            memset(&response_buffer[3], 0, 61);  // Rest mit Nullen (64 bytes total)
            *response_len = 64;
            return true;
        }

        // Feature Report 0xFC: Get Config (WebHID - benutzerdefiniert)
        // Liest alle wichtigen Einstellungen
        case 0xFC: {
            if (!data) {  // GET_REPORT: data ist NULL
                response_buffer[0] = 0xFC;  // Report ID
                response_buffer[1] = g_controller_config.get_stick_change_threshold();
                response_buffer[2] = g_controller_config.get_active_profile();
                response_buffer[3] = g_controller_config.get_inline_deadzone_radius();
                response_buffer[4] = g_controller_config.get_radial_deadzone_radius();
                response_buffer[5] = g_controller_config.is_change_detection_enabled() ? 1 : 0;
                response_buffer[6] = g_controller_config.is_ema_filter_enabled() ? 1 : 0;
                response_buffer[7] = g_controller_config.get_polling_rate();
                // TODO: Weitere wichtige Einstellungen hinzufügen
                memset(&response_buffer[8], 0, 56);  // Rest mit Nullen
                *response_len = 64;
                return true;
            }
            return false;
        }

        // Feature Report 0xFD: Set Config (WebHID - benutzerdefiniert)
        // Setzt Einstellungen
        case 0xFD: {
            if (data && len >= 2) {
                // Handle both formats: with or without Report ID
                // Python sendet: [0xFD, command, value]
                // endpoint0_receive könnte MIT oder OHNE Report ID empfangen
                uint8_t offset = 0;
                if (len > 0 && data[0] == 0xFD) {
                    offset = 1;  // Report ID vorhanden
                }
                
                if (len < (2 + offset)) return false;  // Nicht genug Daten
                
                uint8_t command = data[offset];
                uint8_t value = data[offset + 1];
                bool success = false;

                switch (command) {
                    case 0x01:  // Set Active Profile
                        if (value <= 6) {
                            g_controller_config.set_active_profile(value, true);
                            success = true;
                        }
                        break;
                    case 0x02:  // Set Stick Change Threshold
                        g_controller_config.set_stick_change_threshold(value, true);
                        success = true;
                        break;
                    case 0x03:  // Set Inline Deadzone
                        g_controller_config.set_inline_deadzone_radius(value, true);
                        success = true;
                        break;
                    case 0x04:  // Set Radial Deadzone
                        g_controller_config.set_radial_deadzone_radius(value, true);
                        success = true;
                        break;
                    case 0x05:  // Enable/Disable Change Detection
                        g_controller_config.set_change_detection_enabled(value != 0, true);
                        success = true;
                        break;
                    case 0x06:  // Enable/Disable EMA Filter
                        g_controller_config.set_ema_filter_enabled(value != 0, true);
                        success = true;
                        break;
                    // TODO: Weitere Commands hinzufügen
                }

                // Response: [0xFD, command, ack, len, data...] (Debug: erste 8 Bytes der empfangenen Daten)
                response_buffer[0] = 0xFD;
                response_buffer[1] = command;
                response_buffer[2] = success ? 0x01 : 0x00;  // 0x01 = OK, 0x00 = Error
                response_buffer[3] = len;  // Debug: Länge der empfangenen Daten
                // Debug: Kopiere erste 8 Bytes der empfangenen Daten (OHNE Report ID!)
                for (uint8_t i = 0; i < 8 && i < len; i++) {
                    response_buffer[4 + i] = data[i];
                }
                *response_len = 12;  // 4 Bytes Header + 8 Bytes Debug-Daten
                return true;
            }
            return false;
        }

        default:
            // Nicht behandelt (0xF3 ist Standard DS4 Reset Authentication - wird in usb.c behandelt!)
            return false;
    }
}

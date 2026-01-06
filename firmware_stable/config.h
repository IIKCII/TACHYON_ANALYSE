#ifndef CONFIG_H
#define CONFIG_H

// ===== PIN DEFINITIONEN =====
// ===== 2-PORT GPIO OPTIMIERUNG (NUR GPIO6 + GPIO7!) =====
// Buttons (alle Microswitches)
// Face Buttons (GPIO6)
#define PIN_CROSS       0   // GPIO6 Bit 3
#define PIN_CIRCLE      1   // GPIO6 Bit 2
#define PIN_TRIANGLE    14  // GPIO6 Bit 18
#define PIN_SQUARE      15  // GPIO6 Bit 19

// Shoulder Buttons (GPIO6)
#define PIN_L1          16  // GPIO6 Bit 23
#define PIN_R1          17  // GPIO6 Bit 22
#define PIN_L2          20  // GPIO6 Bit 26 (BLEIBT GLEICH!)
#define PIN_R2          21  // GPIO6 Bit 27

// Stick Buttons (GPIO6)
#define PIN_L3          18  // GPIO6 Bit 17
#define PIN_R3          19  // GPIO6 Bit 16

// D-Pad (GPIO6)
#define PIN_DPAD_UP     22  // GPIO6 Bit 24
#define PIN_DPAD_DOWN   23  // GPIO6 Bit 25
#define PIN_DPAD_LEFT   40  // GPIO6 Bit 20
#define PIN_DPAD_RIGHT  41  // GPIO6 Bit 21

// System Buttons (GPIO7)
#define PIN_SHARE       6   // GPIO7 Bit 10
#define PIN_OPTIONS     7   // GPIO7 Bit 17
#define PIN_PS          8   // GPIO7 Bit 16
#define PIN_TOUCHPAD    9   // GPIO7 Bit 11

// Back Buttons (GPIO7) - Zusätzliche Hardware-Buttons
#define PIN_BACK_LEFT   10  // GPIO7 Bit 0 (LB - Left Back Button)
#define PIN_BACK_RIGHT  11  // GPIO7 Bit 2 (RB - Right Back Button)

// Analog Sticks (TMR Hall-Effect: K-SILVER JS13PRO)
// OPTIMIERT für DUAL ADC: ADC1-only + ADC2-only Pins!
// ⚠️ Pins 26/27 NICHT nutzen (Audio-reserved)!
#define PIN_L_STICK_X   24  // A10 - ADC1 ONLY, Channel 1
#define PIN_L_STICK_Y   39  // A15 - ADC2 ONLY, Channel 2 (VERTAUSCHT!)
#define PIN_R_STICK_X   25  // A11 - ADC1 ONLY, Channel 2
#define PIN_R_STICK_Y   38  // A14 - ADC2 ONLY, Channel 1 (VERTAUSCHT!)

// Status LED (Teensy onboard)
#define PIN_LED         13

// ===== GPIO BIT-MASKEN (für direkte Register-Zugriffe - compile-time optimiert!) =====
// OPTIMIERUNG: Bit-Shifts werden nicht jedes Frame berechnet, sondern compile-time!
// GPIO6 Masken (Face, Shoulder, Stick, D-Pad)
#define GPIO6_MASK_CROSS      (1 << 3)   // Pin 0
#define GPIO6_MASK_CIRCLE     (1 << 2)   // Pin 1
#define GPIO6_MASK_TRIANGLE   (1 << 18)  // Pin 14
#define GPIO6_MASK_SQUARE     (1 << 19)  // Pin 15
#define GPIO6_MASK_L1         (1 << 23)  // Pin 16
#define GPIO6_MASK_R1         (1 << 22)  // Pin 17
#define GPIO6_MASK_L3         (1 << 17)  // Pin 18
#define GPIO6_MASK_R3         (1 << 16)  // Pin 19
#define GPIO6_MASK_DPAD_LEFT  (1 << 20)  // Pin 40
#define GPIO6_MASK_DPAD_RIGHT (1 << 21)  // Pin 41
#define GPIO6_MASK_L2         (1 << 26)  // Pin 20
#define GPIO6_MASK_R2         (1 << 27)  // Pin 21
#define GPIO6_MASK_DPAD_UP    (1 << 24)  // Pin 22
#define GPIO6_MASK_DPAD_DOWN  (1 << 25)  // Pin 23

// GPIO7 Masken (System Buttons)
#define GPIO7_MASK_SHARE      (1 << 10)  // Pin 6
#define GPIO7_MASK_OPTIONS    (1 << 17)  // Pin 7
#define GPIO7_MASK_PS         (1 << 16)  // Pin 8
#define GPIO7_MASK_TOUCHPAD   (1 << 11)  // Pin 9

// GPIO7 Masken (Back Buttons)
#define GPIO7_MASK_BACK_LEFT  (1 << 0)   // Pin 10
#define GPIO7_MASK_BACK_RIGHT (1 << 2)   // Pin 11

// ===== KONFIGURATION =====
// LEGACY: DEFAULT_POLLING_RATE und DEFAULT_DEADZONE entfernt - werden jetzt in controller_config.cpp gehandhabt
#define ADC_RESOLUTION          12      // Bit (Teensy 4.1 unterstützt 12-bit)
#define ADC_MAX_VALUE           4095    // 2^12 - 1

// ===== COMPILE-TIME OPTIMIERUNGEN (für maximale Performance!) =====
// Wird für #if Präprozessor-Direktiven verwendet (compile-time optimization!)
// ⚠️ WICHTIG: CONFIG_ENABLE_CHANGE_DETECTION wird in controller_config.cpp definiert (zentrale Konfiguration)!
// Falls nicht definiert, hier als Fallback (nur wenn controller_config.cpp noch nicht eingelesen wurde):
#ifndef CONFIG_ENABLE_CHANGE_DETECTION
#define CONFIG_ENABLE_CHANGE_DETECTION 0  // 0 = AUS (compile-time), 1 = AN (Fallback)
#endif
#define CONFIG_ENABLE_BUTTON_DEBOUNCE  0  // 0 = AUS (compile-time), 1 = AN
#ifndef CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER
#define CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER  1  // 0 = AUS (compile-time), 1 = AN (wird aus controller_config.cpp übernommen)
#endif
#define CONFIG_ENABLE_EMA_FILTER       1  // 0 = AUS (compile-time), 1 = AN
#define CONFIG_ENABLE_HYSTERESIS       0  // 0 = AUS (compile-time), 1 = AN
#define CONFIG_ENABLE_SNAPBACK_FILTER  1  // 0 = AUS (compile-time), 1 = AN (Verhindert Overshoot beim schnellen Loslassen)
#define CONFIG_ENABLE_OCTAGONAL_GATE   1  // 0 = AUS (compile-time), 1 = AN (8-Wege-Snapping für präzises Movement)
// ⚠️ WICHTIG: CONFIG_ENABLE_PERF_MONITORING wird in controller_config.cpp definiert (zentrale Konfiguration)!
// Falls nicht definiert, hier als Fallback (nur wenn controller_config.cpp noch nicht eingelesen wurde):
#ifndef CONFIG_ENABLE_PERF_MONITORING
#define CONFIG_ENABLE_PERF_MONITORING  1  // 0 = AUS (compile-time), 1 = AN (Fallback)
#endif
#ifndef CONFIG_ENABLE_BUTTON_FILTER_DEBUG
#define CONFIG_ENABLE_BUTTON_FILTER_DEBUG  0  // 0 = AUS (compile-time), 1 = AN (nur für Tests - Serial aktivieren!)
#endif
// Timer-Option: 0 = IntervalTimer (Standard, gut für 8000Hz), 1 = GPT-Timer (experimentell, schneller)
#define CONFIG_USE_GPT_TIMER           0  // 0 = IntervalTimer, 1 = GPT-Timer (für extrem niedrigen Jitter)

// 8-Richtungs-Kalibrierung für mechanischen Stick-Kreis
// Trackt die tatsächliche Form die der Stick durch das Gehäuse macht
struct StickCalibration8Dir {
    uint16_t center_x;
    uint16_t center_y;

    // 8 Richtungen: N, NE, E, SE, S, SW, W, NW (im Uhrzeigersinn)
    // Jede Richtung speichert den maximalen Abstand vom Center
    uint16_t radius[8];  // Radius in jede der 8 Richtungen

    bool is_calibrated;  // Wurde schon kalibriert?
};

// ===== LEGACY-CODE ENTFERNT =====
// StickCalibration wurde entfernt - wird nicht mehr verwendet.
// Aktuelle Kalibrierung läuft über AutoCalibration (AxisAutoCalibration) und ManualCalibration.

// Manuelle Kalibrierung (Option 3 - Hybrid)
// Wird nur verwendet wenn User manuelle Kalibrierung durchführt
struct ManualCalibration {
    bool is_valid;              // Magic flag ob manuell kalibriert
    uint16_t left_radius[8];    // 8 Richtungen linker Stick (E, SE, S, SW, W, NW, N, NE)
    uint16_t right_radius[8];   // 8 Richtungen rechter Stick
    uint32_t magic;             // 0xCAFEBABE für Validierung
};

struct AxisAutoCalibration {
    uint16_t min;
    uint16_t max;
    uint16_t center;
};

struct AutoCalibration {
    uint32_t magic;
    AxisAutoCalibration axis[4];  // 0 = LX, 1 = LY, 2 = RX, 3 = RY
};

#define AUTO_CAL_MAGIC 0xA0CA11B5u

struct StickProfileConfig {
    uint32_t magic;
    uint8_t outer_clamp_enabled;
    uint8_t center_clamp_percent_tenths;  // 0-200 = 0.0-20.0% (in 0.1% Schritten)
    uint8_t radial_deadzone_percent_tenths;  // 0-200 = 0.0-20.0% (in 0.1% Schritten)
    uint8_t reserved;
};

#define STICK_PROFILE_MAGIC 0x53545250u

// ===== BUTTON REMAPPING SYSTEM =====
// Vollständiges Button-Remapping: JEDER Button kann auf JEDEN anderen gemappt werden!

// Enum für alle mappbaren Buttons (Source und Target)
enum ButtonMapping : uint8_t {
    BTN_MAP_PASSTHROUGH = 0, // Keine Änderung (Button wie Original)
    BTN_MAP_DISABLED = 1,    // Deaktiviert (Button sendet nichts)
    BTN_MAP_CROSS = 2,       // Cross (X)
    BTN_MAP_CIRCLE = 3,      // Circle (O)
    BTN_MAP_SQUARE = 4,      // Square
    BTN_MAP_TRIANGLE = 5,    // Triangle
    BTN_MAP_L1 = 6,          // L1
    BTN_MAP_R1 = 7,          // R1
    BTN_MAP_L2 = 8,          // L2
    BTN_MAP_R2 = 9,          // R2
    BTN_MAP_L3 = 10,         // L3
    BTN_MAP_R3 = 11,         // R3
    BTN_MAP_SHARE = 12,      // Share
    BTN_MAP_OPTIONS = 13,    // Options
    BTN_MAP_DPAD_UP = 14,    // D-Pad Up
    BTN_MAP_DPAD_DOWN = 15,  // D-Pad Down
    BTN_MAP_DPAD_LEFT = 16,  // D-Pad Left
    BTN_MAP_DPAD_RIGHT = 17, // D-Pad Right
};

// Indizes für button_remapping Array in ControllerRuntimeConfig
enum ButtonRemapIndex : uint8_t {
    REMAP_IDX_CROSS = 0,
    REMAP_IDX_CIRCLE = 1,
    REMAP_IDX_SQUARE = 2,
    REMAP_IDX_TRIANGLE = 3,
    REMAP_IDX_L1 = 4,
    REMAP_IDX_R1 = 5,
    REMAP_IDX_L2 = 6,
    REMAP_IDX_R2 = 7,
    REMAP_IDX_L3 = 8,
    REMAP_IDX_R3 = 9,
    REMAP_IDX_SHARE = 10,
    REMAP_IDX_OPTIONS = 11,
    REMAP_IDX_DPAD_UP = 12,
    REMAP_IDX_DPAD_DOWN = 13,
    REMAP_IDX_DPAD_LEFT = 14,
    REMAP_IDX_DPAD_RIGHT = 15,
    REMAP_IDX_BACK_LEFT = 16,
    REMAP_IDX_BACK_RIGHT = 17,
};

// ===== LEGACY BACK BUTTON REMAPPING (für Rückwärtskompatibilität) =====
// Diese Enums bleiben für bestehende Profile, werden aber auf ButtonMapping gemappt
enum BackButtonTarget : uint8_t {
    BACK_BTN_DISABLED = BTN_MAP_DISABLED,
    BACK_BTN_CROSS = BTN_MAP_CROSS,
    BACK_BTN_CIRCLE = BTN_MAP_CIRCLE,
    BACK_BTN_SQUARE = BTN_MAP_SQUARE,
    BACK_BTN_TRIANGLE = BTN_MAP_TRIANGLE,
    BACK_BTN_L1 = BTN_MAP_L1,
    BACK_BTN_R1 = BTN_MAP_R1,
    BACK_BTN_L2 = BTN_MAP_L2,
    BACK_BTN_R2 = BTN_MAP_R2,
    BACK_BTN_L3 = BTN_MAP_L3,
    BACK_BTN_R3 = BTN_MAP_R3,
    BACK_BTN_SHARE = BTN_MAP_SHARE,
    BACK_BTN_OPTIONS = BTN_MAP_OPTIONS,
    BACK_BTN_DPAD_UP = BTN_MAP_DPAD_UP,
    BACK_BTN_DPAD_DOWN = BTN_MAP_DPAD_DOWN,
    BACK_BTN_DPAD_LEFT = BTN_MAP_DPAD_LEFT,
    BACK_BTN_DPAD_RIGHT = BTN_MAP_DPAD_RIGHT,
};

// ===== BUTTON MAKROS =====
// Makro-System: Triggert einen Button wenn eine Kombination gedrückt wird
// Unterstützt 2-Button-Kombinationen (z.B. L3+R3 = Circle)
enum MacroTriggerButton : uint8_t {
    MACRO_TRIGGER_NONE = 0,      // Kein Trigger
    MACRO_TRIGGER_CROSS = 1,     // Cross
    MACRO_TRIGGER_CIRCLE = 2,    // Circle
    MACRO_TRIGGER_SQUARE = 3,    // Square
    MACRO_TRIGGER_TRIANGLE = 4,  // Triangle
    MACRO_TRIGGER_L1 = 5,        // L1
    MACRO_TRIGGER_R1 = 6,        // R1
    MACRO_TRIGGER_L2 = 7,        // L2
    MACRO_TRIGGER_R2 = 8,        // R2
    MACRO_TRIGGER_L3 = 9,        // L3
    MACRO_TRIGGER_R3 = 10,       // R3
    MACRO_TRIGGER_SHARE = 11,    // Share
    MACRO_TRIGGER_OPTIONS = 12,  // Options
    MACRO_TRIGGER_DPAD_UP = 13,  // D-Pad Up
    MACRO_TRIGGER_DPAD_DOWN = 14,// D-Pad Down
    MACRO_TRIGGER_DPAD_LEFT = 15,// D-Pad Left
    MACRO_TRIGGER_DPAD_RIGHT = 16,// D-Pad Right
};

// Makro-Struktur: 2-Button-Kombination → Target-Button
struct ButtonMacro {
    uint8_t trigger_button_1;  // Erster Button (MacroTriggerButton)
    uint8_t trigger_button_2;  // Zweiter Button (MacroTriggerButton)
    uint8_t target_button;     // Ziel-Button (MacroTriggerButton)
    uint8_t enabled;           // 0 = AUS, 1 = AN
};

// Controller Runtime Config - Alle Einstellungen die über API konfigurierbar sind
// ⚠️ WICHTIG: Wenn du neue Settings hinzufügst, siehe Checkliste in controller_config.cpp!
struct ControllerRuntimeConfig {
    uint32_t magic;
    uint16_t config_version;  // Version der Config-Struktur (für automatisches Update!)

    // ===== CHANGE DETECTION & THRESHOLDS =====
    uint8_t stick_change_threshold;              // 0-10 Units
    uint8_t enable_adaptive_center_threshold;    // 0 = AUS, 1 = AN
    uint8_t center_threshold;                    // 0-10 Units (wenn adaptive an)
    uint8_t center_threshold_range_min;          // 126-130
    uint8_t center_threshold_range_max;          // 126-130
    uint8_t enable_change_detection;             // 0 = AUS, 1 = AN
    uint8_t enable_report_counter;               // 0 = AUS, 1 = AN

    // ===== FILTER OPTIONS =====
    uint8_t enable_ema_filter;                   // 0 = AUS, 1 = AN
    float ema_alpha;                             // 0.0-1.0
    uint8_t enable_hysteresis;                   // 0 = AUS, 1 = AN
    uint8_t hysteresis_threshold;                // 0-10 Units
    uint8_t enable_snapback_filter;              // 0 = AUS, 1 = AN
    uint8_t snapback_threshold;                  // 0-20 Units (Geschwindigkeit zum Center)
    uint8_t snapback_window;                     // 1-10 Frames (Zeitfenster für Overshoot-Erkennung)

    // ===== OCTAGONAL GATE (8-Wege-Snapping) =====
    uint8_t enable_octagonal_gate;               // 0 = AUS, 1 = AN (nur linker Stick)
    uint8_t octagonal_gate_strength;             // 0-100% (wie stark zum nächsten 45° Winkel "snappen")

    // ===== BUTTON DEBOUNCING =====
    uint8_t enable_button_debounce;              // 0 = AUS, 1 = AN
    uint8_t debounce_samples;                    // 1-10 Samples

    // ===== ASYMMETRIC RELEASE FILTER =====
    uint8_t enable_asymmetric_release_filter;    // 0 = AUS, 1 = AN
    uint8_t release_guard_us;                     // 15-20 µs empfohlen

    // ===== ADC HARDWARE =====
    uint8_t adc_resolution;                      // 12 = 12-bit, 10 = 10-bit, 8 = 8-bit
    uint8_t adc_averaging;                       // 0 = 1x, 2 = 4x, 3 = 8x, 4 = 16x, 5 = 32x
    uint8_t adc_clock_mhz;                       // 150 = 150 MHz (Standard)
    uint8_t adc_sample_time;                     // 0 = SHORT, 1 = LONG

    // ===== STICK PROCESSING (DEADZONES & CLAMPS) =====
    uint8_t inline_deadzone_radius;              // 0-20 Units (±Units um Center → 128) ✅ AKTIV!
    uint8_t radial_deadzone_radius;              // 0-127 Units (Radial Deadzone, 0 = AUS) ✅ AKTIV!
    uint8_t outer_clamp_enabled;                 // 0 = AUS, 1 = AN

    // ===== PERFORMANCE =====
    uint8_t enable_led_feedback;                 // 0 = AUS, 1 = AN

    // ===== POLLING RATE =====
    uint8_t polling_rate;                        // 0 = 1kHz, 1 = 2kHz, 2 = 4kHz, 3 = 8kHz

    // ===== PROFILE SYSTEM =====
    uint8_t active_profile;                      // 0-6: Aktuell aktives Profil (gespeichert in EEPROM)
    uint8_t profile_switch_button1;             // Button 1 für Runtime-Profil-Wechsel (DS4_BTN2_* Bit-Maske, Standard: DS4_BTN2_SHARE)
    uint8_t profile_switch_button2;             // Button 2 für Runtime-Profil-Wechsel (DS4_BTN2_* Bit-Maske, Standard: DS4_BTN2_OPTIONS)

    // ===== BACK BUTTON REMAPPING (Legacy - wird durch button_remapping ersetzt) =====
    uint8_t back_left_target;                    // BackButtonTarget für linken Back-Button (Legacy)
    uint8_t back_right_target;                   // BackButtonTarget für rechten Back-Button (Legacy)

    // ===== BUTTON MAKROS =====
    ButtonMacro macro_slots[4];                  // Bis zu 4 Makros (z.B. L3+R3=Circle)

    // ===== VOLLSTÄNDIGES BUTTON REMAPPING =====
    // Remapping-Tabelle für ALLE Buttons (18 Buttons total)
    // Jeder Eintrag definiert, auf welchen Button der physische Button gemappt wird
    // 0 = PASSTHROUGH (keine Änderung), 1 = DISABLED, 2-17 = ButtonMapping
    uint8_t button_remapping[18];  // Index: [CROSS, CIRCLE, SQUARE, TRIANGLE, L1, R1, L2, R2, L3, R3, SHARE, OPTIONS, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, BACK_LEFT, BACK_RIGHT]
};

#define CONTROLLER_RUNTIME_CONFIG_MAGIC 0x52554E54u  // "RUNT"

// ===== LEGACY-CODE ENTFERNT =====
// LegacyControllerConfig wurde entfernt - wird nicht mehr verwendet.
// Aktuelle Konfiguration läuft über ControllerRuntimeConfig.

// EEPROM Adressen (neu berechnet ohne LegacyControllerConfig)
// LegacyControllerConfig wurde entfernt - Adressen wurden neu berechnet
// Legacy-Bereich (0-63) bleibt frei für Rückwärtskompatibilität
#define EEPROM_CONFIG_ADDR          0   // Legacy-Adresse (nicht mehr verwendet, aber für Kompatibilität behalten)
#define EEPROM_MANUAL_CAL_ADDR      64  // Direkt nach Legacy-Bereich (64 Bytes Reserve)
#define EEPROM_AUTO_CAL_ADDR        (EEPROM_MANUAL_CAL_ADDR + sizeof(ManualCalibration))
#define EEPROM_PROFILE_ADDR         (EEPROM_AUTO_CAL_ADDR + sizeof(AutoCalibration))
#define EEPROM_RUNTIME_CONFIG_ADDR  (EEPROM_PROFILE_ADDR + sizeof(StickProfileConfig))

#endif // CONFIG_H

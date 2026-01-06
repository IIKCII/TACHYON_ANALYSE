#ifndef CONTROLLER_CONFIG_H
#define CONTROLLER_CONFIG_H

#include <Arduino.h>

// ===== COMPILE-TIME KONFIGURATION =====
// ⚠️ ZENTRALE KONFIGURATION: Ändere die Werte in controller_config.cpp (Zeile 66-67), NICHT hier!
// Die Werte hier sind für compile-time optimization (#if Direktiven) und müssen VOR config.h stehen!
// Diese Werte werden automatisch aus controller_config.cpp übernommen - keine doppelte Pflege nötig!
// Fallback-Werte (nur wenn controller_config.cpp noch nicht eingelesen wurde):
#ifndef CONFIG_ENABLE_CHANGE_DETECTION
#define CONFIG_ENABLE_CHANGE_DETECTION 1  // 0 = AUS (compile-time), 1 = AN (wird aus controller_config.cpp übernommen)
#endif

#ifndef CONFIG_ENABLE_PERF_MONITORING
#define CONFIG_ENABLE_PERF_MONITORING 0  // 0 = AUS (compile-time), 1 = AN (wird aus controller_config.cpp übernommen)
#endif

#include "config.h"

// ===== CONTROLLER RUNTIME CONFIG API =====
// Alle Einstellungen die über API/WebUSB konfigurierbar sind
//
// ⚠️ WICHTIG: Neue Settings/Funktionen müssen auch in controller_config.cpp hinzugefügt werden!
// Siehe Checkliste in controller_config.cpp (Zeile ~12)
//

class ControllerConfig {
public:
    // ===== CHANGE DETECTION & THRESHOLDS =====
    void set_stick_change_threshold(uint8_t threshold, bool persist = true);
    uint8_t get_stick_change_threshold() const;
    
    void set_adaptive_center_threshold_enabled(bool enabled, bool persist = true);
    bool is_adaptive_center_threshold_enabled() const;
    
    void set_center_threshold(uint8_t threshold, bool persist = true);
    uint8_t get_center_threshold() const;
    
    void set_center_threshold_range(uint8_t min, uint8_t max, bool persist = true);
    uint8_t get_center_threshold_range_min() const;
    uint8_t get_center_threshold_range_max() const;
    
    void set_change_detection_enabled(bool enabled, bool persist = true);
    bool is_change_detection_enabled() const;
    
    void set_report_counter_enabled(bool enabled, bool persist = true);
    bool is_report_counter_enabled() const;
    
    // ===== FILTER OPTIONS =====
    void set_ema_filter_enabled(bool enabled, bool persist = true);
    bool is_ema_filter_enabled() const;
    
    void set_ema_alpha(float alpha, bool persist = true);
    float get_ema_alpha() const;
    
    void set_hysteresis_enabled(bool enabled, bool persist = true);
    bool is_hysteresis_enabled() const;
    
    void set_hysteresis_threshold(uint8_t threshold, bool persist = true);
    uint8_t get_hysteresis_threshold() const;
    
    void set_snapback_filter_enabled(bool enabled, bool persist = true);
    bool is_snapback_filter_enabled() const;
    
    void set_snapback_threshold(uint8_t threshold, bool persist = true);
    uint8_t get_snapback_threshold() const;
    
    void set_snapback_window(uint8_t window, bool persist = true);
    uint8_t get_snapback_window() const;
    
    // ===== BUTTON DEBOUNCING =====
    void set_button_debounce_enabled(bool enabled, bool persist = true);
    bool is_button_debounce_enabled() const;
    
    void set_debounce_samples(uint8_t samples, bool persist = true);
    uint8_t get_debounce_samples() const;
    
    // ===== ASYMMETRIC RELEASE FILTER =====
    void set_asymmetric_release_filter_enabled(bool enabled, bool persist = true);
    bool is_asymmetric_release_filter_enabled() const;
    
    void set_release_guard_us(uint8_t us, bool persist = true);
    uint8_t get_release_guard_us() const;
    
    // ===== ADC HARDWARE =====
    void set_adc_resolution(uint8_t resolution, bool persist = true);
    uint8_t get_adc_resolution() const;
    
    void set_adc_averaging(uint8_t averaging, bool persist = true);
    uint8_t get_adc_averaging() const;
    
    void set_adc_clock_mhz(uint8_t mhz, bool persist = true);
    uint8_t get_adc_clock_mhz() const;
    
    void set_adc_sample_time(uint8_t sample_time, bool persist = true);
    uint8_t get_adc_sample_time() const;
    
    // ===== STICK PROCESSING (DEADZONES & CLAMPS) =====
    void set_inline_deadzone_radius(uint8_t radius, bool persist = true);
    uint8_t get_inline_deadzone_radius() const;
    
    void set_radial_deadzone_radius(uint8_t radius, bool persist = true);
    uint8_t get_radial_deadzone_radius() const;
    
    void set_outer_clamp_enabled(bool enabled, bool persist = true);
    bool is_outer_clamp_enabled() const;
    
    // ===== PERFORMANCE =====
    void set_led_feedback_enabled(bool enabled, bool persist = true);
    bool is_led_feedback_enabled() const;
    
    // ===== POLLING RATE =====
    void set_polling_rate(uint8_t rate, bool persist = true);  // 0 = 1kHz, 1 = 2kHz, 2 = 4kHz, 3 = 8kHz
    uint8_t get_polling_rate() const;

    // ===== PROFILE SYSTEM =====
    void set_active_profile(uint8_t profile, bool persist = true);  // 0-6: Profil wechseln
    uint8_t get_active_profile() const;
    void load_profile(uint8_t profile);  // Lädt ein Profil (ohne EEPROM zu speichern)
    
    // Runtime-Profil-Wechsel Button-Kombination (verwendet MacroTriggerButton Enum, wie Makros!)
    void set_profile_switch_button1(uint8_t button_enum, bool persist = true);  // MacroTriggerButton Enum
    uint8_t get_profile_switch_button1() const;  // Gibt MacroTriggerButton Enum zurück
    void set_profile_switch_button2(uint8_t button_enum, bool persist = true);  // MacroTriggerButton Enum
    uint8_t get_profile_switch_button2() const;  // Gibt MacroTriggerButton Enum zurück
    
    // Runtime-Profil-Wechsel Handler (wird von updateController() aufgerufen)
    // Konfigurierbare Button-Kombination für 1 Sekunde halten → Profil wechseln!
    // Gibt true zurück wenn ein Profil-Wechsel stattgefunden hat (für Cache-Invalidierung)
    bool check_runtime_profile_switch(uint8_t buttons1, uint8_t buttons3);  // buttons1 = D-Pad, buttons3 = PS/Touchpad, buttons2 wird intern geholt

    // ===== BACK BUTTON REMAPPING (Legacy - verwende stattdessen set_button_remapping) =====
    void set_back_left_target(uint8_t target, bool persist = true);  // BackButtonTarget enum (Legacy)
    uint8_t get_back_left_target() const;

    void set_back_right_target(uint8_t target, bool persist = true);  // BackButtonTarget enum (Legacy)
    uint8_t get_back_right_target() const;

    // ===== VOLLSTÄNDIGES BUTTON REMAPPING =====
    void set_button_remapping(uint8_t button_index, uint8_t target, bool persist = true);  // ButtonRemapIndex, ButtonMapping
    uint8_t get_button_remapping(uint8_t button_index) const;
    void reset_button_remapping(bool persist = true);  // Setzt alle Buttons auf PASSTHROUGH

    // ===== BUTTON MAKROS =====
    void set_macro_slot(uint8_t slot, uint8_t trigger1, uint8_t trigger2, uint8_t target, bool enabled, bool persist = true);
    void get_macro_slot(uint8_t slot, uint8_t* trigger1, uint8_t* trigger2, uint8_t* target, bool* enabled) const;
    void enable_macro_slot(uint8_t slot, bool enabled, bool persist = true);
    bool is_macro_slot_enabled(uint8_t slot) const;

    // ===== INITIALISIERUNG & SPEICHERUNG =====
    void init();
    void load_from_eeprom();
    void save_to_eeprom();
    void reset_to_defaults();

    // ===== INTERNE FUNKTIONEN (für Teensy_DS4_8000Hz.ino) =====
    // Diese Funktionen werden intern verwendet, nicht über API!
    uint8_t get_stick_change_threshold_internal() const { return config.stick_change_threshold; }
    bool get_adaptive_center_threshold_enabled_internal() const { return config.enable_adaptive_center_threshold != 0; }
    uint8_t get_center_threshold_internal() const { return config.center_threshold; }
    uint8_t get_center_threshold_range_min_internal() const { return config.center_threshold_range_min; }
    uint8_t get_center_threshold_range_max_internal() const { return config.center_threshold_range_max; }
    bool get_change_detection_enabled_internal() const { return config.enable_change_detection != 0; }
    bool get_report_counter_enabled_internal() const { return config.enable_report_counter != 0; }
    bool get_ema_filter_enabled_internal() const { return config.enable_ema_filter != 0; }
    float get_ema_alpha_internal() const { return config.ema_alpha; }
    bool get_hysteresis_enabled_internal() const { return config.enable_hysteresis != 0; }
    uint8_t get_hysteresis_threshold_internal() const { return config.hysteresis_threshold; }
    bool get_snapback_filter_enabled_internal() const { return config.enable_snapback_filter != 0; }
    uint8_t get_snapback_threshold_internal() const { return config.snapback_threshold; }
    uint8_t get_snapback_window_internal() const { return config.snapback_window; }
    bool get_button_debounce_enabled_internal() const { return config.enable_button_debounce != 0; }
    uint8_t get_debounce_samples_internal() const { return config.debounce_samples; }
    bool get_asymmetric_release_filter_enabled_internal() const { return config.enable_asymmetric_release_filter != 0; }
    uint8_t get_release_guard_us_internal() const { return config.release_guard_us; }
    uint8_t get_adc_resolution_internal() const { return config.adc_resolution; }
    uint8_t get_adc_averaging_internal() const { return config.adc_averaging; }
    uint8_t get_adc_clock_mhz_internal() const { return config.adc_clock_mhz; }
    uint8_t get_adc_sample_time_internal() const { return config.adc_sample_time; }
    uint8_t get_inline_deadzone_radius_internal() const { return config.inline_deadzone_radius; }
    uint8_t get_radial_deadzone_radius_internal() const { return config.radial_deadzone_radius; }
    bool get_outer_clamp_enabled_internal() const { return config.outer_clamp_enabled != 0; }
    bool get_led_feedback_enabled_internal() const { return config.enable_led_feedback != 0; }
    uint8_t get_polling_rate_internal() const { return config.polling_rate; }
    uint8_t get_active_profile_internal() const { return config.active_profile; }
    uint8_t get_profile_switch_button1_internal() const { return config.profile_switch_button1; }
    uint8_t get_profile_switch_button2_internal() const { return config.profile_switch_button2; }
    uint8_t get_back_left_target_internal() const { return config.back_left_target; }
    uint8_t get_back_right_target_internal() const { return config.back_right_target; }
    const ButtonMacro* get_macro_slots_internal() const { return config.macro_slots; }
    const uint8_t* get_button_remapping_internal() const { return config.button_remapping; }
    bool get_octagonal_gate_enabled_internal() const { return config.enable_octagonal_gate != 0; }
    uint8_t get_octagonal_gate_strength_internal() const { return config.octagonal_gate_strength; }

private:
    ControllerRuntimeConfig config;
    void set_defaults();
    void merge_with_defaults_smart();  // Intelligentes Merge: Neue Defaults automatisch, User-Werte bleiben
};

extern ControllerConfig g_controller_config;

#endif // CONTROLLER_CONFIG_H


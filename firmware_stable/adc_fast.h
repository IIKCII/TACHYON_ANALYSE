#ifndef ADC_FAST_H
#define ADC_FAST_H

#include <Arduino.h>
#include "config.h"

// ADC_ETC für schnelles Multi-Channel-Sampling
// Teensy 4.1 hat ADC1 und ADC2, beide können parallel laufen

// Pin-zu-ADC Mapping für unsere Sticks:
// PIN 18 (A4)  = ADC1_CH8 / ADC2_CH8   → L_STICK_X
// PIN 41 (A17) = ADC1_CH1 / ADC2_CH1   → L_STICK_Y
// PIN 19 (A5)  = ADC1_CH13 / ADC2_CH13 → R_STICK_X
// PIN 40 (A16) = ADC1_CH0 / ADC2_CH0   → R_STICK_Y

class ADC_FAST {
public:
    void begin();
    void begin_with_manual_calibration();  // Option 3: Manuelle Kalibrierung
    void begin_with_auto_saved();          // Auto-Kalibrierung aus EEPROM laden (oder speichern)
    void init_profile_state();             // Stick-Profil (z. B. Outer Clamp) laden
    void reload_profile_state();
    void trigger_first_stick_pair();  // Phase 1: Pre-Trigger ADC (L_X + L_Y) - LEGACY
    void trigger_all_sticks_now();    // ULTRA-LOW LATENCY: Alle 4 Sticks SOFORT parallel triggern!
    void read_all_sticks(uint16_t *lx, uint16_t *ly, uint16_t *rx, uint16_t *ry);
    void print_8dir_debug();  // Debug-Funktion
    void clear_manual_state();
    void clear_auto_state();
    void set_outer_clamp_enabled(bool enabled, bool persist = true);
    bool is_outer_clamp_enabled() const;
    
    // Inline Deadzone API (direkt in Units, keine Prozent-Konvertierung - funktionierte besser!) ✅ AKTIV!
    void set_inline_deadzone_radius(uint8_t radius, bool persist = true);
    uint8_t get_inline_deadzone_radius() const;
    
    // Radial Deadzone API (direkt in Units: 0-127) ✅ AKTIV!
    void set_radial_deadzone_radius(uint8_t radius, bool persist = true);
    uint8_t get_radial_deadzone_radius() const;

private:
    void configure_adc_etc();
    void run_manual_calibration();  // Manuelle 8-Dir Kalibrierung
    void led_blink_blocking(int count, int on_time_ms, int off_time_ms);  // Blockierendes LED-Blinken
};

extern ADC_FAST FastADC;

// Zugriff auf Kalibrierungs-Daten für Debug
extern StickCalibration8Dir cal_left_8dir_extern;
extern StickCalibration8Dir cal_right_8dir_extern;

#endif // ADC_FAST_H

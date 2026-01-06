#ifndef EEPROM_CONFIG_H
#define EEPROM_CONFIG_H

#include <EEPROM.h>
#include "config.h"

// ===== LEGACY-CODE ENTFERNT =====
// Alle Legacy-Funktionen (loadConfig, saveConfig, createDefaultConfig, initConfig)
// und Legacy-Strukturen (DEFAULT_LEFT_STICK, DEFAULT_RIGHT_STICK) wurden entfernt.
// Aktuelle Kalibrierung läuft über AutoCalibration und ManualCalibration.

// === MANUELLE KALIBRIERUNG (OPTION 3) ===

// Manuelle Kalibrierung laden
inline bool loadManualCalibration(ManualCalibration &cal) {
    EEPROM.get(EEPROM_MANUAL_CAL_ADDR, cal);

    // Validierung: Magic muss stimmen UND is_valid muss true sein
    if (cal.magic != 0xCAFEBABE || !cal.is_valid) {
        return false;
    }

    return true;
}

// Manuelle Kalibrierung speichern
inline void saveManualCalibration(const ManualCalibration &cal) {
    EEPROM.put(EEPROM_MANUAL_CAL_ADDR, cal);
}

// Manuelle Kalibrierung löschen (Reset-Funktion)
inline void clearManualCalibration() {
    ManualCalibration cal;
    memset(&cal, 0, sizeof(cal));  // Alles auf 0 setzen
    cal.is_valid = false;
    cal.magic = 0x00000000;  // Ungültig machen

    saveManualCalibration(cal);
}

inline bool loadAutoCalibration(AutoCalibration &cal) {
    EEPROM.get(EEPROM_AUTO_CAL_ADDR, cal);
    if (cal.magic != AUTO_CAL_MAGIC) {
        memset(&cal, 0, sizeof(cal));
        return false;
    }
    return true;
}

inline void saveAutoCalibration(const AutoCalibration &cal) {
    EEPROM.put(EEPROM_AUTO_CAL_ADDR, cal);
}

inline void clearAutoCalibration() {
    AutoCalibration cal;
    memset(&cal, 0, sizeof(cal));
    cal.magic = 0;
    saveAutoCalibration(cal);
}

inline bool loadStickProfileConfig(StickProfileConfig &cfg) {
    EEPROM.get(EEPROM_PROFILE_ADDR, cfg);
    if (cfg.magic != STICK_PROFILE_MAGIC) {
        memset(&cfg, 0, sizeof(cfg));
        return false;
    }
    return true;
}

inline void saveStickProfileConfig(const StickProfileConfig &cfg) {
    EEPROM.put(EEPROM_PROFILE_ADDR, cfg);
}

inline void clearStickProfileConfig() {
    StickProfileConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.center_clamp_percent_tenths = 35;  // 3.5% Center Clamp
    cfg.radial_deadzone_percent_tenths = 0;  // 0.0% Radial Deadzone
    saveStickProfileConfig(cfg);
}
#endif // EEPROM_CONFIG_H

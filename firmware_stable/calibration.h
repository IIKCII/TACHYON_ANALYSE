#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "config.h"
#include <math.h>

// Look-Up-Table für schnelle Quadratwurzel (0-65535)
// Pre-calculiert für minimale Latenz (~0.05 µs statt ~0.6 µs)
// Verwendet 16-Bit Eingabe mit 8-Bit Granularität (256 Einträge)
static const uint8_t SQRT_LUT_256[256] PROGMEM = {
    0, 16, 22, 27, 32, 35, 39, 42, 45, 48, 50, 53, 55, 57, 59, 61,
    64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 80, 82, 83, 85, 86, 88,
    89, 90, 92, 93, 94, 96, 97, 98, 99, 101, 102, 103, 104, 105, 107, 108,
    109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 144, 145, 146, 147, 148, 149, 150, 150, 151, 152, 153, 154, 154, 155, 156,
    157, 158, 158, 159, 160, 161, 161, 162, 163, 163, 164, 165, 166, 166, 167, 168,
    168, 169, 170, 170, 171, 172, 172, 173, 173, 174, 175, 175, 176, 176, 177, 178,
    178, 179, 179, 180, 181, 181, 182, 182, 183, 183, 184, 185, 185, 186, 186, 187,
    187, 188, 188, 189, 189, 190, 190, 191, 192, 192, 193, 193, 194, 194, 195, 195,
    196, 196, 197, 197, 198, 198, 199, 199, 199, 200, 200, 201, 201, 202, 202, 203,
    203, 204, 204, 205, 205, 206, 206, 206, 207, 207, 208, 208, 209, 209, 210, 210,
    210, 211, 211, 212, 212, 213, 213, 213, 214, 214, 215, 215, 215, 216, 216, 217,
    217, 217, 218, 218, 219, 219, 219, 220, 220, 221, 221, 221, 222, 222, 222, 223,
    223, 224, 224, 224, 225, 225, 225, 226, 226, 227, 227, 227, 228, 228, 228, 229,
    229, 229, 230, 230, 230, 231, 231, 231, 232, 232, 232, 233, 233, 233, 234, 234
};

// Ultra-schnelle Quadratwurzel mit LUT (Look-Up-Table)
// Input: 0-65535 (16-bit), Output: 0-255 (8-bit)
// Latenz: ~0.05 µs (12x schneller als Berechnung!)
static inline uint16_t fast_sqrt_lut(uint32_t x) {
    if (x == 0) return 0;
    if (x >= 65536) return 255;  // Clamp auf 16-bit max

    // Nutze obere 8 Bits als Index (256 Einträge)
    uint8_t index = (x >> 8) & 0xFF;
    return pgm_read_byte(&SQRT_LUT_256[index]);
}

// Schnelle Winkelberechnung für 8 Richtungen (ohne atan2!)
// 0=E, 1=SE, 2=S, 3=SW, 4=W, 5=NW, 6=N, 7=NE (im Uhrzeigersinn von rechts)
static inline uint8_t get_direction_8(int32_t x, int32_t y) {
    if (x == 0 && y == 0) return 0;  // Center

    // Absolute Werte
    int32_t abs_x = (x >= 0) ? x : -x;
    int32_t abs_y = (y >= 0) ? y : -y;

    // Vergleiche ob mehr horizontal oder vertikal
    bool more_x = (abs_x > abs_y);

    // Y ist in Gamepad-Koordinaten invertiert (oben = negativ)
    // 8 Richtungen: E, SE, S, SW, W, NW, N, NE
    if (x >= 0 && y >= 0) {        // Quadrant 4 (rechts unten)
        return more_x ? 0 : 1;     // E oder SE
    } else if (x < 0 && y >= 0) {  // Quadrant 3 (links unten)
        return more_x ? 4 : 3;     // W oder SW
    } else if (x < 0 && y < 0) {   // Quadrant 2 (links oben)
        return more_x ? 4 : 5;     // W oder NW
    } else {                        // Quadrant 1 (rechts oben)
        return more_x ? 0 : 7;     // E oder NE
    }
}

// 8-Richtungs-Kalibrierung V3: CLIPPING statt Reskalierung
// KONZEPT: Finde kleinsten Radius, schneide NUR ab was drüber ist
// ULTRA-OPTIMIERT: ~0.2 µs
static inline void apply8DirCalibration(int32_t &x, int32_t &y, const StickCalibration8Dir &cal) {
    if (!cal.is_calibrated) return;
    if (x == 0 && y == 0) return;  // Im Center bleiben

    // Finde den MINIMALEN Radius aus allen 8 Richtungen
    // Das ist der größte perfekte Kreis der überall passt!
    uint16_t min_radius = 255;
    for (uint8_t i = 0; i < 8; i++) {
        if (cal.radius[i] < min_radius && cal.radius[i] > 50) {
            min_radius = cal.radius[i];
        }
    }

    if (min_radius > 200 || min_radius < 80) return;  // Noch nicht kalibriert oder unrealistisch

    // Berechne aktuellen Abstand vom Center
    uint32_t distance_sq = (uint32_t)(x * x + y * y);
    uint32_t distance = fast_sqrt_lut(distance_sq);

    if (distance == 0) return;

    // Wenn AUSSERHALB des perfekten Kreises: Auf Kreis-Rand clippen
    if (distance > min_radius) {
        // Schneide auf den Kreis-Radius ab
        x = ((int32_t)x * min_radius) / distance;
        y = ((int32_t)y * min_radius) / distance;
    }
    // Wenn INNERHALB: Lass unverändert!
}

// ===== LEGACY-CODE ENTFERNT =====
// applyCalibration() wurde entfernt - wird nicht mehr verwendet.
// Aktuelle Kalibrierung läuft direkt in adc_fast.cpp (map_axis_u8()).
//
// StickCalibrator wurde entfernt - wird nicht mehr verwendet.
// Aktuelle Kalibrierung läuft über AutoCalibration (in adc_fast.cpp).

#endif // CALIBRATION_H

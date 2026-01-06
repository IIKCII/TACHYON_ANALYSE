#include "controller_config.h"
#include "eeprom_config.h"
#include "config.h"      // Für PIN_LED, digitalWrite und MacroTriggerButton
#include "ds4_descriptor.h"  // Für DS4_BTN2_* Masken
#include <EEPROM.h>
#include <string.h>
#include <Arduino.h>  // Für digitalWrite

// ============================================================================================                       
//               
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
//               ████████╗ █████╗  ██████╗██╗  ██╗██╗   ██╗ ██████╗ ███╗   ██╗
//               ╚══██╔══╝██╔══██╗██╔════╝██║  ██║╚██╗ ██╔╝██╔═══██╗████╗  ██║
//                  ██║   ███████║██║     ███████║ ╚████╔╝ ██║   ██║██╔██╗ ██║
//                  ██║   ██╔══██║██║     ██╔══██║  ╚██╔╝  ██║   ██║██║╚██╗██║
//                  ██║   ██║  ██║╚██████╗██║  ██║   ██║   ╚██████╔╝██║ ╚████║
//                  ╚═╝   ╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═══╝
// 
// 
// 
//
// 
// 
// 
// 
//                             ⚡ ULTRA-LOW LATENCY CONTROLLER ⚡  
//                                             
//                               Engineered for the Frame ahead  
//                                                                                         
//                      ⚡ Faster than Light - Precise as a Tachyon ⚡  
// 
// 
// 
// 
// 
//
// 
// 
// 
// 
// 
// 
// 
// II-KC-II                                                                               2025
// ============================================================================================
//
// 
// ===== ZENTRALE KONFIGURATION - ALLE EINSTELLUNGEN HIER ÄNDERN! =====
//
// ===== CHANGE DETECTION & THRESHOLDS =====
#define CONFIG_STICK_CHANGE_THRESHOLD           0   // 0-10 Units (0 = sehr sensitiv, wie im Backup!)
#define CONFIG_ENABLE_ADAPTIVE_CENTER_THRESHOLD 0   // 0 = AUS, 1 = AN
#define CONFIG_CENTER_THRESHOLD                 4   // 0-10 Units (wenn adaptive an)
#define CONFIG_CENTER_THRESHOLD_RANGE_MIN       126 // Untere Grenze (126-130)
#define CONFIG_CENTER_THRESHOLD_RANGE_MAX       130 // Obere Grenze (126-130)

// ===== COMPILE-TIME OPTIMIERUNGEN (für maximale Performance!) =====
// ⚠️ ZENTRALE DEFINITIONEN: ÄNDERE DIESE WERTE HIER!
// Diese Werte werden für compile-time optimization (#if Direktiven) verwendet.
// PERFORMANCE: Deaktiviert = Code komplett entfernt (0 Zyklen Overhead!)
// ⚠️ WICHTIG: Diese Werte werden automatisch in controller_config.h übernommen (keine doppelte Pflege nötig!)
// Note: Redefinition is expected - controller_config.h provides fallback value
#undef CONFIG_ENABLE_CHANGE_DETECTION
#define CONFIG_ENABLE_CHANGE_DETECTION          1   // 0 = AUS (compile-time), 1 = AN (wie im Backup - für ~5000Hz!)
#undef CONFIG_ENABLE_PERF_MONITORING
#define CONFIG_ENABLE_PERF_MONITORING           0   // 0 = AUS (compile-time), 1 = AN (optional für Debug)
#define CONFIG_ENABLE_REPORT_COUNTER            0   // 0 = AUS, 1 = AN (nur für BF6)
#define CONFIG_ENABLE_BUTTON_FILTER_DEBUG       0   // 0 = AUS, 1 = AN (Debug-Logging für Button-Filter - nur für Tests!)

// ===== FILTER OPTIONS =====
#undef CONFIG_ENABLE_EMA_FILTER
#define CONFIG_ENABLE_EMA_FILTER                0   // 0 = AUS, 1 = AN (AKTIVIERT: Reduziert Sprünge beim Aiming!)
#define CONFIG_EMA_ALPHA                        0.8f// 0.0-1.0 (0.9 = minimaler Lag, reduziert Sprünge außerhalb Center!)
#define CONFIG_ENABLE_HYSTERESIS                0   // 0 = AUS, 1 = AN
#define CONFIG_HYSTERESIS_THRESHOLD             2   // 0-10 Units
#define CONFIG_ENABLE_SNAPBACK_FILTER           1   // 0 = AUS, 1 = AN (Verhindert Overshoot beim schnellen Loslassen!)
#define CONFIG_SNAPBACK_THRESHOLD               20  // 0-20 Units (Geschwindigkeit zum Center für Snapback-Erkennung)
#define CONFIG_SNAPBACK_WINDOW                  4   // 1-10 Frames (Zeitfenster für Overshoot-Erkennung nach Snapback)

// ===== OCTAGONAL GATE (8-Wege-Snapping) =====
#undef CONFIG_ENABLE_OCTAGONAL_GATE
#define CONFIG_ENABLE_OCTAGONAL_GATE            0   // 0 = AUS, 1 = AN (nur linker Stick)
#define CONFIG_OCTAGONAL_GATE_STRENGTH          30  // 0-100% (30% = sanftes Snapping, gut für Naraka)

// ===== BUTTON DEBOUNCING =====
#define CONFIG_ENABLE_BUTTON_DEBOUNCE           0   // 0 = AUS, 1 = AN
#define CONFIG_DEBOUNCE_SAMPLES                 4   // 1-10 Samples

// ===== ASYMMETRIC RELEASE FILTER =====
#define CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER  1   // 0 = AUS, 1 = AN
#define CONFIG_RELEASE_GUARD_US                  25  // 15-20 µs empfohlen

// ===== ADC HARDWARE =====
#define CONFIG_ADC_RESOLUTION                   12  // 12 = 12-bit, 10 = 10-bit, 8 = 8-bit
#define CONFIG_ADC_AVERAGING                    2   // 0 = 1x, 2 = 4x, 3 = 8x, 4 = 16x, 5 = 32x
#define CONFIG_ADC_CLOCK_MHZ                    150 // 150 = 150 MHz (Standard)
#define CONFIG_ADC_SAMPLE_TIME                  0   // 0 = SHORT, 1 = LONG

// ===== STICK PROCESSING (DEADZONES & CLAMPS) =====
#define CONFIG_INLINE_DEADZONE_RADIUS           3   // 0-20 Units (±3 um Center → 125-131 → 128) ✅ AKTIV! (3 Units = guter Kompromiss zwischen Rate und Rauschen)
#define CONFIG_RADIAL_DEADZONE_RADIUS           0   // 0-127 Units (Radial Deadzone, 0 = AUS) ✅ AKTIV!
#define CONFIG_OUTER_CLAMP_ENABLED              0   // 0 = AUS, 1 = AN (begrenzt auf Unit-Circle)

// ===== PERFORMANCE =====
#define CONFIG_ENABLE_LED_FEEDBACK              0   // 0 = AUS, 1 = AN

// ===== POLLING RATE =====
#define CONFIG_POLLING_RATE                     3   // 0 = 1kHz, 1 = 2kHz, 2 = 4kHz, 3 = 8kHz (Standard!)

// ===== RUNTIME PROFILE SWITCH =====
// Button-Kombination für Runtime-Profil-Wechsel (für 1 Sekunde halten!)
// Standard: PS-Button + D-Pad Up
// 
// Verfügbare Buttons (MacroTriggerButton Enum):
//   Für buttons1 (D-Pad): MACRO_TRIGGER_DPAD_UP, MACRO_TRIGGER_DPAD_DOWN, MACRO_TRIGGER_DPAD_LEFT, MACRO_TRIGGER_DPAD_RIGHT
//   Für buttons3 (System): MACRO_TRIGGER_SHARE, MACRO_TRIGGER_OPTIONS
//   Für buttons2 (Andere): MACRO_TRIGGER_L1, MACRO_TRIGGER_R1, MACRO_TRIGGER_L2, MACRO_TRIGGER_R2, MACRO_TRIGGER_L3, MACRO_TRIGGER_R3
//
// HINWEIS: PS und Touchpad sind nicht über MacroTriggerButton verfügbar, daher:
//   0xFE = PS-Button (Sonderwert für buttons3)
//   0xFD = Touchpad (Sonderwert für buttons3)
#define CONFIG_PROFILE_SWITCH_BUTTON1           0xFE   // PS-Button (0xFE = Sonderwert)
#define CONFIG_PROFILE_SWITCH_BUTTON2           MACRO_TRIGGER_DPAD_UP  // D-Pad Up

// ============================================================================================
//
//              ⚡ MKB (MOUSE/KEYBOARD) KONFIGURATION - ZENTRALE EINSTELLUNG ⚡
//
// ============================================================================================
//
// PROFIL 0: HYBRID-MODUS (DS4 + PS-Button → Taste '0')
// ============================================================================================
//
//                    ⚡ CONTROLLER PROFILE SYSTEM - ZENTRALE KONFIGURATION ⚡
//
// ============================================================================================
//
// HIER KONFIGURIERST DU ALLES FÜR DEIN PROFIL:
//   ✅ Button-Remapping (alle 18 Buttons!)
//   ✅ Button-Makros (bis zu 4 Kombinationen)
//   ✅ PS-Button-Sequenz (an/aus)
//
// ============================================================================================
//
// ===== SCHRITT 1: PROFIL AUSWÄHLEN (ändere nur diese Zahl!) =====
#define ACTIVE_PROFILE     0
//
// Verfügbare Profile (0-6):
//   0 - Standard (keine Änderungen)
//   1 - FPS Games (CoD/Apex/Warzone - optimiert für Shooter!)
//   2 - Racing Games (Gran Turismo/Forza)
//   3 - Souls-like (Elden Ring/Dark Souls)
//   4 - Fighting Games (Street Fighter/Tekken)
//   5 - Tactical FPS (Rainbow Six/CS2/Valorant)
//   6 - Sports Games (FIFA/NBA 2K)
//
// ============================================================================================

// ============================================================================================
//
// ===== SCHRITT 2: PROFIL KONFIGURIEREN (scrolle zu deinem Profil unten) =====
//
// Jedes Profil hat:
//   1. Button-Remapping (alle 18 Buttons!)
//   2. Button-Makros (bis zu 4 Kombinationen)
//   3. PS-Button-Sequenz (an/aus)
//
// Scrolle runter zu deinem Profil (z.B. "PROFIL 1: FPS") und passe es an!
//
// ============================================================================================

// ===== BUTTON MAKROS (profilabhängig!) =====
// Jedes Profil hat seine eigenen Makros (bis zu 4 pro Profil)
// Format: { trigger_button_1, trigger_button_2, target_button, enabled }
//
// Verfügbare Buttons: MACRO_TRIGGER_NONE, MACRO_TRIGGER_CROSS, MACRO_TRIGGER_CIRCLE,
//                     MACRO_TRIGGER_SQUARE, MACRO_TRIGGER_TRIANGLE, MACRO_TRIGGER_L1,
//                     MACRO_TRIGGER_R1, MACRO_TRIGGER_L2, MACRO_TRIGGER_R2, MACRO_TRIGGER_L3,
//                     MACRO_TRIGGER_R3, MACRO_TRIGGER_SHARE, MACRO_TRIGGER_OPTIONS,
//                     MACRO_TRIGGER_DPAD_UP, MACRO_TRIGGER_DPAD_DOWN, MACRO_TRIGGER_DPAD_LEFT,
//                     MACRO_TRIGGER_DPAD_RIGHT


// ============================================================================================
// ===== PROFIL 0: STANDARD =====
// ============================================================================================

// ===== BUTTON REMAPPING =====
// Jeder Button kann individuell gemappt werden (BTN_MAP_PASSTHROUGH = keine Änderung)
#define PROFILE_0_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_0_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_0_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_0_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_0_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_0_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_0_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_0_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_0_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_0_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_0_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_0_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_0_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_0_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_0_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_0_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_0_REMAP_BACK_LEFT       BTN_MAP_CROSS        // Back-Left → Cross
#define PROFILE_0_REMAP_BACK_RIGHT      BTN_MAP_DPAD_DOWN    // Back-Right → D-Pad Down

// ===== PS-BUTTON SEQUENZ-MAKRO =====
#define PROFILE_0_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// Button-Kombinations-Makros
#define PROFILE_0_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Circle
#define PROFILE_0_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_0_MACRO_1_TARGET        MACRO_TRIGGER_CIRCLE
#define PROFILE_0_MACRO_1_ENABLED       0

#define PROFILE_0_MACRO_2_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_0_MACRO_2_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_2_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_2_ENABLED       0

#define PROFILE_0_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_0_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_3_ENABLED       0

#define PROFILE_0_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_0_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_0_MACRO_4_ENABLED       0

// ============================================================================================
// ===== PROFIL 1: FPS GAMES (Call of Duty, Apex, Warzone) =====
// ============================================================================================
//
// Optimiertes Layout für FPS-Spiele:
//   • Back-Right → Circle (Crouch/Slide ohne Daumen vom Stick!)
//   • Circle → Cross (Jump auf Circle, gewohnt von CoD)
//   • Cross → Square (Reload auf Cross, schneller!)
//   • Back-Left → D-Pad Down (Waffenwechsel/Ping)
//
// ===== BUTTON REMAPPING =====
#define PROFILE_1_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross → Square (Reload auf X!)
#define PROFILE_1_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle → Cross (Jump auf O!)
#define PROFILE_1_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_1_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_1_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_1_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_1_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2 (Aim)
#define PROFILE_1_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2 (Shoot)
#define PROFILE_1_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3 (Sprint)
#define PROFILE_1_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3 (Melee)
#define PROFILE_1_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_1_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_1_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_1_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_1_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_1_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_1_REMAP_BACK_LEFT       BTN_MAP_R2           // Back-Left → D-Pad Down (Ping!)
#define PROFILE_1_REMAP_BACK_RIGHT      BTN_MAP_DPAD_DOWN    // Back-Right → Circle (Slide!)

// ===== PS-BUTTON SEQUENZ-MAKRO =====
#define PROFILE_1_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (nicht nötig für FPS)

// Button-Kombinations-Makros
#define PROFILE_1_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3          // L3 + R3 = Circle (Crouch)
#define PROFILE_1_MACRO_1_TRIGGER_2     MACRO_TRIGGER_DPAD_DOWN
#define PROFILE_1_MACRO_1_TARGET        MACRO_TRIGGER_CIRCLE
#define PROFILE_1_MACRO_1_ENABLED       0

#define PROFILE_1_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L1          // L1 + R1 = Triangle (Reload/Interact)
#define PROFILE_1_MACRO_2_TRIGGER_2     MACRO_TRIGGER_R1
#define PROFILE_1_MACRO_2_TARGET        MACRO_TRIGGER_SQUARE
#define PROFILE_1_MACRO_2_ENABLED       0                         // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_1_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE        // Deaktiviert
#define PROFILE_1_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_1_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_1_MACRO_3_ENABLED       0

#define PROFILE_1_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE       // Deaktiviert
#define PROFILE_1_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_1_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_1_MACRO_4_ENABLED       0

// ===== PROFIL 2: Racing =====
// Back-Buttons wurden oben definiert

// PS-Button Sequenz-Makro für Profil 2
#define PROFILE_2_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// ===== VOLLSTÄNDIGES BUTTON REMAPPING FÜR PROFIL 2 (Racing) =====
// Standard: Alle Buttons auf PASSTHROUGH (keine Änderung außer Back-Buttons)
#define PROFILE_2_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_2_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_2_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_2_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_2_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_2_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_2_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_2_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_2_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_2_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_2_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_2_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_2_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_2_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_2_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_2_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_2_REMAP_BACK_LEFT       PROFILE_2_LEFT   // Back-Left aus Profil-Config
#define PROFILE_2_REMAP_BACK_RIGHT      PROFILE_2_RIGHT  // Back-Right aus Profil-Config

// Button-Kombinations-Makros
#define PROFILE_2_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L1        // L1 + R1 = Triangle (Change View)
#define PROFILE_2_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R1
#define PROFILE_2_MACRO_1_TARGET        MACRO_TRIGGER_TRIANGLE
#define PROFILE_2_MACRO_1_ENABLED       0

#define PROFILE_2_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Circle (Reset)
#define PROFILE_2_MACRO_2_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_2_MACRO_2_TARGET        MACRO_TRIGGER_CIRCLE
#define PROFILE_2_MACRO_2_ENABLED       0                       // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_2_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_2_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_2_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_2_MACRO_3_ENABLED       0

#define PROFILE_2_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_2_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_2_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_2_MACRO_4_ENABLED       0

// ===== PROFIL 3: Souls-like =====
// Back-Buttons wurden oben definiert

// PS-Button Sequenz-Makro für Profil 3
#define PROFILE_3_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// ===== VOLLSTÄNDIGES BUTTON REMAPPING FÜR PROFIL 3 (Souls-like) =====
// Standard: Alle Buttons auf PASSTHROUGH (keine Änderung außer Back-Buttons)
#define PROFILE_3_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_3_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_3_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_3_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_3_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_3_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_3_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_3_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_3_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_3_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_3_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_3_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_3_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_3_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_3_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_3_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_3_REMAP_BACK_LEFT       PROFILE_3_LEFT   // Back-Left aus Profil-Config
#define PROFILE_3_REMAP_BACK_RIGHT      PROFILE_3_RIGHT  // Back-Right aus Profil-Config

// Button-Kombinations-Makros
#define PROFILE_3_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Triangle (Use Item)
#define PROFILE_3_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_3_MACRO_1_TARGET        MACRO_TRIGGER_TRIANGLE
#define PROFILE_3_MACRO_1_ENABLED       0

#define PROFILE_3_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L1        // L1 + Circle = Square (Two-Hand)
#define PROFILE_3_MACRO_2_TRIGGER_2     MACRO_TRIGGER_CIRCLE
#define PROFILE_3_MACRO_2_TARGET        MACRO_TRIGGER_SQUARE
#define PROFILE_3_MACRO_2_ENABLED       0                       // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_3_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_3_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_3_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_3_MACRO_3_ENABLED       0

#define PROFILE_3_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_3_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_3_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_3_MACRO_4_ENABLED       0

// ===== PROFIL 4: Fighting =====
// Back-Buttons wurden oben definiert

// PS-Button Sequenz-Makro für Profil 4
#define PROFILE_4_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// ===== VOLLSTÄNDIGES BUTTON REMAPPING FÜR PROFIL 4 (Fighting) =====
// Standard: Alle Buttons auf PASSTHROUGH (keine Änderung außer Back-Buttons)
#define PROFILE_4_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_4_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_4_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_4_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_4_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_4_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_4_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_4_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_4_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_4_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_4_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_4_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_4_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_4_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_4_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_4_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_4_REMAP_BACK_LEFT       PROFILE_4_LEFT   // Back-Left aus Profil-Config
#define PROFILE_4_REMAP_BACK_RIGHT      PROFILE_4_RIGHT  // Back-Right aus Profil-Config

// Button-Kombinations-Makros
#define PROFILE_4_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Cross (Special)
#define PROFILE_4_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_4_MACRO_1_TARGET        MACRO_TRIGGER_CROSS
#define PROFILE_4_MACRO_1_ENABLED       0

#define PROFILE_4_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L1        // L1 + R1 = Triangle (Throw)
#define PROFILE_4_MACRO_2_TRIGGER_2     MACRO_TRIGGER_R1
#define PROFILE_4_MACRO_2_TARGET        MACRO_TRIGGER_TRIANGLE
#define PROFILE_4_MACRO_2_ENABLED       0                       // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_4_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_4_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_4_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_4_MACRO_3_ENABLED       0

#define PROFILE_4_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_4_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_4_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_4_MACRO_4_ENABLED       0

// ===== PROFIL 5: Tactical FPS =====
// Back-Buttons wurden oben definiert

// PS-Button Sequenz-Makro für Profil 5
#define PROFILE_5_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// ===== VOLLSTÄNDIGES BUTTON REMAPPING FÜR PROFIL 5 (Tactical FPS) =====
// Standard: Alle Buttons auf PASSTHROUGH (keine Änderung außer Back-Buttons)
#define PROFILE_5_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_5_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_5_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_5_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_5_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_5_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_5_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_5_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_5_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_5_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_5_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_5_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_5_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_5_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_5_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_5_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_5_REMAP_BACK_LEFT       PROFILE_5_LEFT   // Back-Left aus Profil-Config
#define PROFILE_5_REMAP_BACK_RIGHT      PROFILE_5_RIGHT  // Back-Right aus Profil-Config

// Button-Kombinations-Makros
#define PROFILE_5_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Triangle (Gadget)
#define PROFILE_5_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_5_MACRO_1_TARGET        MACRO_TRIGGER_TRIANGLE
#define PROFILE_5_MACRO_1_ENABLED       0

#define PROFILE_5_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L1        // L1 + R1 = Square (Ping/Mark)
#define PROFILE_5_MACRO_2_TRIGGER_2     MACRO_TRIGGER_R1
#define PROFILE_5_MACRO_2_TARGET        MACRO_TRIGGER_SQUARE
#define PROFILE_5_MACRO_2_ENABLED       0                       // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_5_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_5_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_5_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_5_MACRO_3_ENABLED       0

#define PROFILE_5_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_5_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_5_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_5_MACRO_4_ENABLED       0

// ===== PROFIL 6: Sports =====
// Back-Buttons wurden oben definiert

// PS-Button Sequenz-Makro für Profil 6
#define PROFILE_6_PS_SEQUENCE_ENABLED   0   // 0 = AUS, 1 = AN (PS → R3 Hold + Triangle Tap)

// ===== VOLLSTÄNDIGES BUTTON REMAPPING FÜR PROFIL 6 (Sports) =====
// Standard: Alle Buttons auf PASSTHROUGH (keine Änderung außer Back-Buttons)
#define PROFILE_6_REMAP_CROSS           BTN_MAP_PASSTHROUGH  // Cross bleibt Cross
#define PROFILE_6_REMAP_CIRCLE          BTN_MAP_PASSTHROUGH  // Circle bleibt Circle
#define PROFILE_6_REMAP_SQUARE          BTN_MAP_PASSTHROUGH  // Square bleibt Square
#define PROFILE_6_REMAP_TRIANGLE        BTN_MAP_PASSTHROUGH  // Triangle bleibt Triangle
#define PROFILE_6_REMAP_L1              BTN_MAP_PASSTHROUGH  // L1 bleibt L1
#define PROFILE_6_REMAP_R1              BTN_MAP_PASSTHROUGH  // R1 bleibt R1
#define PROFILE_6_REMAP_L2              BTN_MAP_PASSTHROUGH  // L2 bleibt L2
#define PROFILE_6_REMAP_R2              BTN_MAP_PASSTHROUGH  // R2 bleibt R2
#define PROFILE_6_REMAP_L3              BTN_MAP_PASSTHROUGH  // L3 bleibt L3
#define PROFILE_6_REMAP_R3              BTN_MAP_PASSTHROUGH  // R3 bleibt R3
#define PROFILE_6_REMAP_SHARE           BTN_MAP_PASSTHROUGH  // Share bleibt Share
#define PROFILE_6_REMAP_OPTIONS         BTN_MAP_PASSTHROUGH  // Options bleibt Options
#define PROFILE_6_REMAP_DPAD_UP         BTN_MAP_PASSTHROUGH  // D-Pad Up bleibt D-Pad Up
#define PROFILE_6_REMAP_DPAD_DOWN       BTN_MAP_PASSTHROUGH  // D-Pad Down bleibt D-Pad Down
#define PROFILE_6_REMAP_DPAD_LEFT       BTN_MAP_PASSTHROUGH  // D-Pad Left bleibt D-Pad Left
#define PROFILE_6_REMAP_DPAD_RIGHT      BTN_MAP_PASSTHROUGH  // D-Pad Right bleibt D-Pad Right
#define PROFILE_6_REMAP_BACK_LEFT       PROFILE_6_LEFT   // Back-Left aus Profil-Config
#define PROFILE_6_REMAP_BACK_RIGHT      PROFILE_6_RIGHT  // Back-Right aus Profil-Config

// Button-Kombinations-Makros
#define PROFILE_6_MACRO_1_TRIGGER_1     MACRO_TRIGGER_L3        // L3 + R3 = Triangle (Special Play)
#define PROFILE_6_MACRO_1_TRIGGER_2     MACRO_TRIGGER_R3
#define PROFILE_6_MACRO_1_TARGET        MACRO_TRIGGER_TRIANGLE
#define PROFILE_6_MACRO_1_ENABLED       0

#define PROFILE_6_MACRO_2_TRIGGER_1     MACRO_TRIGGER_L1        // L1 + R1 = Circle (Celebration)
#define PROFILE_6_MACRO_2_TRIGGER_2     MACRO_TRIGGER_R1
#define PROFILE_6_MACRO_2_TARGET        MACRO_TRIGGER_CIRCLE
#define PROFILE_6_MACRO_2_ENABLED       0                       // Deaktiviert (aktiviere bei Bedarf)

#define PROFILE_6_MACRO_3_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_6_MACRO_3_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_6_MACRO_3_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_6_MACRO_3_ENABLED       0

#define PROFILE_6_MACRO_4_TRIGGER_1     MACRO_TRIGGER_NONE      // Deaktiviert
#define PROFILE_6_MACRO_4_TRIGGER_2     MACRO_TRIGGER_NONE
#define PROFILE_6_MACRO_4_TARGET        MACRO_TRIGGER_NONE
#define PROFILE_6_MACRO_4_ENABLED       0

// ===== AUTOMATISCHE PS-SEQUENZ-AUSWAHL (nicht ändern!) =====
#if ACTIVE_PROFILE == 0
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_0_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 1
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_1_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 2
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_2_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 3
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_3_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 4
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_4_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 5
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_5_PS_SEQUENCE_ENABLED
#elif ACTIVE_PROFILE == 6
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_6_PS_SEQUENCE_ENABLED
#else
    #define CONFIG_PS_SEQUENCE_ENABLED      PROFILE_0_PS_SEQUENCE_ENABLED
#endif

// ===== AUTOMATISCHE MAKRO-AUSWAHL (nicht ändern!) =====
#if ACTIVE_PROFILE == 0
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_0_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_0_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_0_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_0_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_0_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_0_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_0_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_0_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_0_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_0_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_0_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_0_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_0_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_0_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_0_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_0_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_1_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_1_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_1_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_1_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_1_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_1_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_1_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_1_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_1_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_1_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_1_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_1_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_1_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_1_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_1_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_1_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 2
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_2_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_2_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_2_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_2_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_2_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_2_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_2_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_2_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_2_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_2_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_2_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_2_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_2_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_2_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_2_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_2_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 3
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_3_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_3_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_3_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_3_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_3_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_3_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_3_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_3_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_3_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_3_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_3_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_3_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_3_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_3_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_3_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_3_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 4
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_4_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_4_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_4_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_4_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_4_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_4_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_4_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_4_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_4_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_4_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_4_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_4_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_4_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_4_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_4_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_4_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 5
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_5_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_5_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_5_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_5_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_5_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_5_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_5_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_5_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_5_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_5_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_5_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_5_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_5_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_5_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_5_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_5_MACRO_4_ENABLED
#elif ACTIVE_PROFILE == 6
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_6_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_6_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_6_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_6_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_6_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_6_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_6_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_6_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_6_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_6_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_6_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_6_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_6_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_6_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_6_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_6_MACRO_4_ENABLED
#else
    // Fallback zu Profil 0
    #define CONFIG_MACRO_SLOT_1_TRIGGER_1   PROFILE_0_MACRO_1_TRIGGER_1
    #define CONFIG_MACRO_SLOT_1_TRIGGER_2   PROFILE_0_MACRO_1_TRIGGER_2
    #define CONFIG_MACRO_SLOT_1_TARGET      PROFILE_0_MACRO_1_TARGET
    #define CONFIG_MACRO_SLOT_1_ENABLED     PROFILE_0_MACRO_1_ENABLED
    #define CONFIG_MACRO_SLOT_2_TRIGGER_1   PROFILE_0_MACRO_2_TRIGGER_1
    #define CONFIG_MACRO_SLOT_2_TRIGGER_2   PROFILE_0_MACRO_2_TRIGGER_2
    #define CONFIG_MACRO_SLOT_2_TARGET      PROFILE_0_MACRO_2_TARGET
    #define CONFIG_MACRO_SLOT_2_ENABLED     PROFILE_0_MACRO_2_ENABLED
    #define CONFIG_MACRO_SLOT_3_TRIGGER_1   PROFILE_0_MACRO_3_TRIGGER_1
    #define CONFIG_MACRO_SLOT_3_TRIGGER_2   PROFILE_0_MACRO_3_TRIGGER_2
    #define CONFIG_MACRO_SLOT_3_TARGET      PROFILE_0_MACRO_3_TARGET
    #define CONFIG_MACRO_SLOT_3_ENABLED     PROFILE_0_MACRO_3_ENABLED
    #define CONFIG_MACRO_SLOT_4_TRIGGER_1   PROFILE_0_MACRO_4_TRIGGER_1
    #define CONFIG_MACRO_SLOT_4_TRIGGER_2   PROFILE_0_MACRO_4_TRIGGER_2
    #define CONFIG_MACRO_SLOT_4_TARGET      PROFILE_0_MACRO_4_TARGET
    #define CONFIG_MACRO_SLOT_4_ENABLED     PROFILE_0_MACRO_4_ENABLED
#endif

// ===== AUTOMATISCHE BUTTON-REMAPPING-AUSWAHL (basierend auf ACTIVE_PROFILE) =====
#if ACTIVE_PROFILE == 0
    #define CONFIG_REMAP_CROSS           PROFILE_0_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_0_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_0_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_0_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_0_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_0_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_0_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_0_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_0_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_0_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_0_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_0_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_0_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_0_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_0_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_0_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_0_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_0_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 1
    #define CONFIG_REMAP_CROSS           PROFILE_1_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_1_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_1_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_1_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_1_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_1_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_1_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_1_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_1_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_1_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_1_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_1_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_1_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_1_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_1_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_1_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_1_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_1_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 2
    #define CONFIG_REMAP_CROSS           PROFILE_2_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_2_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_2_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_2_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_2_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_2_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_2_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_2_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_2_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_2_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_2_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_2_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_2_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_2_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_2_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_2_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_2_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_2_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 3
    #define CONFIG_REMAP_CROSS           PROFILE_3_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_3_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_3_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_3_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_3_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_3_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_3_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_3_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_3_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_3_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_3_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_3_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_3_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_3_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_3_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_3_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_3_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_3_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 4
    #define CONFIG_REMAP_CROSS           PROFILE_4_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_4_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_4_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_4_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_4_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_4_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_4_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_4_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_4_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_4_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_4_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_4_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_4_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_4_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_4_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_4_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_4_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_4_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 5
    #define CONFIG_REMAP_CROSS           PROFILE_5_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_5_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_5_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_5_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_5_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_5_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_5_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_5_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_5_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_5_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_5_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_5_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_5_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_5_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_5_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_5_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_5_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_5_REMAP_BACK_RIGHT
#elif ACTIVE_PROFILE == 6
    #define CONFIG_REMAP_CROSS           PROFILE_6_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_6_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_6_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_6_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_6_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_6_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_6_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_6_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_6_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_6_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_6_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_6_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_6_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_6_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_6_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_6_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_6_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_6_REMAP_BACK_RIGHT
#else
    // Fallback zu Profil 0
    #define CONFIG_REMAP_CROSS           PROFILE_0_REMAP_CROSS
    #define CONFIG_REMAP_CIRCLE          PROFILE_0_REMAP_CIRCLE
    #define CONFIG_REMAP_SQUARE          PROFILE_0_REMAP_SQUARE
    #define CONFIG_REMAP_TRIANGLE        PROFILE_0_REMAP_TRIANGLE
    #define CONFIG_REMAP_L1              PROFILE_0_REMAP_L1
    #define CONFIG_REMAP_R1              PROFILE_0_REMAP_R1
    #define CONFIG_REMAP_L2              PROFILE_0_REMAP_L2
    #define CONFIG_REMAP_R2              PROFILE_0_REMAP_R2
    #define CONFIG_REMAP_L3              PROFILE_0_REMAP_L3
    #define CONFIG_REMAP_R3              PROFILE_0_REMAP_R3
    #define CONFIG_REMAP_SHARE           PROFILE_0_REMAP_SHARE
    #define CONFIG_REMAP_OPTIONS         PROFILE_0_REMAP_OPTIONS
    #define CONFIG_REMAP_DPAD_UP         PROFILE_0_REMAP_DPAD_UP
    #define CONFIG_REMAP_DPAD_DOWN       PROFILE_0_REMAP_DPAD_DOWN
    #define CONFIG_REMAP_DPAD_LEFT       PROFILE_0_REMAP_DPAD_LEFT
    #define CONFIG_REMAP_DPAD_RIGHT      PROFILE_0_REMAP_DPAD_RIGHT
    #define CONFIG_REMAP_BACK_LEFT       PROFILE_0_REMAP_BACK_LEFT
    #define CONFIG_REMAP_BACK_RIGHT      PROFILE_0_REMAP_BACK_RIGHT
#endif

// ===== CONFIG VERSION (Legacy - nicht mehr benötigt, bleibt für Kompatibilität) =====
#define CONFIG_VERSION                          0   // Wird automatisch gesetzt (nicht mehr manuell ändern!)
//
// ============================================================================================
// ⚠️ WICHTIG: Wenn du neue Settings/Funktionen hinzufügst, MÜSSEN diese hier eingetragen werden!
// 
// 📋 CHECKLISTE FÜR NEUE SETTINGS (strukturiert bleiben!):
// 1. ✅ #define CONFIG_XXX in diesem Block hinzufügen (hier oben, mit Kommentar!)
// 2. ✅ In ControllerRuntimeConfig (config.h, Zeile ~112) Feld hinzufügen
// 3. ✅ In set_defaults() (Zeile ~92) den Wert zuweisen: config.xxx = CONFIG_XXX;
// 4. ✅ API-Funktionen in controller_config.h (Zeile ~10) und controller_config.cpp (ab Zeile ~250) hinzufügen:
//      - set_xxx(value, persist = true)
//      - get_xxx() const
//      - get_xxx_internal() const (für interne Nutzung in .ino/.cpp)
// 5. ✅ Falls im Code verwendet: g_controller_config.get_xxx_internal() aufrufen
// 6. ✅ Optional: In load_from_eeprom() prüfen (aktuell werden Defaults immer übernommen)
//
// So bleibt alles strukturiert und an einem zentralen Ort! 🎯
// ============================================================================================

// Globale Instanz
ControllerConfig g_controller_config;

// ===== HELPER-MAKROS FÜR REPETITIVE GETTER/SETTER (Code-Reduktion!) =====
// OPTIMIERUNG: Reduziert Code-Duplikation für ähnliche Getter/Setter-Patterns

// Makro für einfache uint8_t Getter/Setter mit Clamp (min, max)
#define IMPL_UINT8_GETTER_SETTER(name, field, min_val, max_val, default_val) \
    void ControllerConfig::set_##name(uint8_t value, bool persist) { \
        if (value < min_val) value = min_val; \
        if (value > max_val) value = max_val; \
        config.field = value; \
        if (persist) save_to_eeprom(); \
    } \
    uint8_t ControllerConfig::get_##name() const { \
        return config.field; \
    }

// Makro für bool Getter/Setter
#define IMPL_BOOL_GETTER_SETTER(name, field) \
    void ControllerConfig::set_##name##_enabled(bool enabled, bool persist) { \
        config.field = enabled ? 1 : 0; \
        if (persist) save_to_eeprom(); \
    } \
    bool ControllerConfig::is_##name##_enabled() const { \
        return config.field != 0; \
    }

// Makro für float Getter/Setter mit Range (min, max)
#define IMPL_FLOAT_GETTER_SETTER(name, field, min_val, max_val) \
    void ControllerConfig::set_##name(float value, bool persist) { \
        if (value < min_val) value = min_val; \
        if (value > max_val) value = max_val; \
        config.field = value; \
        if (persist) save_to_eeprom(); \
    } \
    float ControllerConfig::get_##name() const { \
        return config.field; \
    }

// ===== INITIALISIERUNG =====

void ControllerConfig::init() {
    load_from_eeprom();
}

void ControllerConfig::load_from_eeprom() {
    EEPROM.get(EEPROM_RUNTIME_CONFIG_ADDR, config);
    
    // Validierung
    if (config.magic != CONTROLLER_RUNTIME_CONFIG_MAGIC) {
        // Keine gültige Config → Defaults setzen
        set_defaults();
        save_to_eeprom();
    } else {
        // Config ist gültig → Neue Defaults komplett übernehmen!
        // EINFACH: Defaults in controller_config.cpp ändern → werden beim Boot übernommen!
        // WICHTIG: Das überschreibt auch API-Änderungen - einfach nochmal per API setzen falls nötig
        set_defaults();
        save_to_eeprom();
    }
}

void ControllerConfig::save_to_eeprom() {
    config.magic = CONTROLLER_RUNTIME_CONFIG_MAGIC;
    EEPROM.put(EEPROM_RUNTIME_CONFIG_ADDR, config);
}

void ControllerConfig::reset_to_defaults() {
    set_defaults();
    save_to_eeprom();
}

void ControllerConfig::set_defaults() {
    memset(&config, 0, sizeof(config));
    
    // ===== CHANGE DETECTION & THRESHOLDS =====
    config.stick_change_threshold = CONFIG_STICK_CHANGE_THRESHOLD;
    config.enable_adaptive_center_threshold = CONFIG_ENABLE_ADAPTIVE_CENTER_THRESHOLD;
    config.center_threshold = CONFIG_CENTER_THRESHOLD;
    config.center_threshold_range_min = CONFIG_CENTER_THRESHOLD_RANGE_MIN;
    config.center_threshold_range_max = CONFIG_CENTER_THRESHOLD_RANGE_MAX;
    config.enable_change_detection = CONFIG_ENABLE_CHANGE_DETECTION;
    config.enable_report_counter = CONFIG_ENABLE_REPORT_COUNTER;
    
    // ===== FILTER OPTIONS =====
    config.enable_ema_filter = CONFIG_ENABLE_EMA_FILTER;
    config.ema_alpha = CONFIG_EMA_ALPHA;
    config.enable_hysteresis = CONFIG_ENABLE_HYSTERESIS;
    config.hysteresis_threshold = CONFIG_HYSTERESIS_THRESHOLD;
    config.enable_snapback_filter = CONFIG_ENABLE_SNAPBACK_FILTER;
    config.snapback_threshold = CONFIG_SNAPBACK_THRESHOLD;
    config.snapback_window = CONFIG_SNAPBACK_WINDOW;

    // ===== OCTAGONAL GATE =====
    config.enable_octagonal_gate = CONFIG_ENABLE_OCTAGONAL_GATE;
    config.octagonal_gate_strength = CONFIG_OCTAGONAL_GATE_STRENGTH;

    // ===== BUTTON DEBOUNCING =====
    config.enable_button_debounce = CONFIG_ENABLE_BUTTON_DEBOUNCE;
    config.debounce_samples = CONFIG_DEBOUNCE_SAMPLES;

    // ===== ASYMMETRIC RELEASE FILTER =====
    config.enable_asymmetric_release_filter = CONFIG_ENABLE_ASYMMETRIC_RELEASE_FILTER;
    config.release_guard_us = CONFIG_RELEASE_GUARD_US;
    
    // ===== ADC HARDWARE =====
    config.adc_resolution = CONFIG_ADC_RESOLUTION;
    config.adc_averaging = CONFIG_ADC_AVERAGING;
    config.adc_clock_mhz = CONFIG_ADC_CLOCK_MHZ;
    config.adc_sample_time = CONFIG_ADC_SAMPLE_TIME;
    
    // ===== STICK PROCESSING (DEADZONES & CLAMPS) =====
    config.inline_deadzone_radius = CONFIG_INLINE_DEADZONE_RADIUS;
    config.radial_deadzone_radius = CONFIG_RADIAL_DEADZONE_RADIUS;
    config.outer_clamp_enabled = CONFIG_OUTER_CLAMP_ENABLED;
    
    // ===== PERFORMANCE =====
    config.enable_led_feedback = CONFIG_ENABLE_LED_FEEDBACK;
    
    // ===== POLLING RATE =====
    config.polling_rate = CONFIG_POLLING_RATE;

    // ===== PROFILE SYSTEM =====
    config.active_profile = ACTIVE_PROFILE;
    config.profile_switch_button1 = CONFIG_PROFILE_SWITCH_BUTTON1;
    config.profile_switch_button2 = CONFIG_PROFILE_SWITCH_BUTTON2;

    // ===== VOLLSTÄNDIGES BUTTON REMAPPING =====
    // Initialisiere Button-Remapping-Tabelle mit Profilwerten (CONFIG_REMAP_*)
    config.button_remapping[REMAP_IDX_CROSS] = CONFIG_REMAP_CROSS;
    config.button_remapping[REMAP_IDX_CIRCLE] = CONFIG_REMAP_CIRCLE;
    config.button_remapping[REMAP_IDX_SQUARE] = CONFIG_REMAP_SQUARE;
    config.button_remapping[REMAP_IDX_TRIANGLE] = CONFIG_REMAP_TRIANGLE;
    config.button_remapping[REMAP_IDX_L1] = CONFIG_REMAP_L1;
    config.button_remapping[REMAP_IDX_R1] = CONFIG_REMAP_R1;
    config.button_remapping[REMAP_IDX_L2] = CONFIG_REMAP_L2;
    config.button_remapping[REMAP_IDX_R2] = CONFIG_REMAP_R2;
    config.button_remapping[REMAP_IDX_L3] = CONFIG_REMAP_L3;
    config.button_remapping[REMAP_IDX_R3] = CONFIG_REMAP_R3;
    config.button_remapping[REMAP_IDX_SHARE] = CONFIG_REMAP_SHARE;
    config.button_remapping[REMAP_IDX_OPTIONS] = CONFIG_REMAP_OPTIONS;
    config.button_remapping[REMAP_IDX_DPAD_UP] = CONFIG_REMAP_DPAD_UP;
    config.button_remapping[REMAP_IDX_DPAD_DOWN] = CONFIG_REMAP_DPAD_DOWN;
    config.button_remapping[REMAP_IDX_DPAD_LEFT] = CONFIG_REMAP_DPAD_LEFT;
    config.button_remapping[REMAP_IDX_DPAD_RIGHT] = CONFIG_REMAP_DPAD_RIGHT;
    config.button_remapping[REMAP_IDX_BACK_LEFT] = CONFIG_REMAP_BACK_LEFT;
    config.button_remapping[REMAP_IDX_BACK_RIGHT] = CONFIG_REMAP_BACK_RIGHT;

    // ===== LEGACY BACK BUTTON REMAPPING (für Rückwärtskompatibilität) =====
    // Synchronisiere Back-Button-Werte mit Remapping-Tabelle
    config.back_left_target = CONFIG_REMAP_BACK_LEFT;
    config.back_right_target = CONFIG_REMAP_BACK_RIGHT;

    // ===== BUTTON MAKROS =====
    config.macro_slots[0].trigger_button_1 = CONFIG_MACRO_SLOT_1_TRIGGER_1;
    config.macro_slots[0].trigger_button_2 = CONFIG_MACRO_SLOT_1_TRIGGER_2;
    config.macro_slots[0].target_button = CONFIG_MACRO_SLOT_1_TARGET;
    config.macro_slots[0].enabled = CONFIG_MACRO_SLOT_1_ENABLED;

    config.macro_slots[1].trigger_button_1 = CONFIG_MACRO_SLOT_2_TRIGGER_1;
    config.macro_slots[1].trigger_button_2 = CONFIG_MACRO_SLOT_2_TRIGGER_2;
    config.macro_slots[1].target_button = CONFIG_MACRO_SLOT_2_TARGET;
    config.macro_slots[1].enabled = CONFIG_MACRO_SLOT_2_ENABLED;

    config.macro_slots[2].trigger_button_1 = CONFIG_MACRO_SLOT_3_TRIGGER_1;
    config.macro_slots[2].trigger_button_2 = CONFIG_MACRO_SLOT_3_TRIGGER_2;
    config.macro_slots[2].target_button = CONFIG_MACRO_SLOT_3_TARGET;
    config.macro_slots[2].enabled = CONFIG_MACRO_SLOT_3_ENABLED;

    config.macro_slots[3].trigger_button_1 = CONFIG_MACRO_SLOT_4_TRIGGER_1;
    config.macro_slots[3].trigger_button_2 = CONFIG_MACRO_SLOT_4_TRIGGER_2;
    config.macro_slots[3].target_button = CONFIG_MACRO_SLOT_4_TARGET;
    config.macro_slots[3].enabled = CONFIG_MACRO_SLOT_4_ENABLED;

    // ===== CONFIG VERSION =====
    config.config_version = CONFIG_VERSION;

    config.magic = CONTROLLER_RUNTIME_CONFIG_MAGIC;
}

// Einfaches System: Neue Defaults werden IMMER übernommen (wenn sich geändert haben)
// WICHTIG: Das bedeutet, API-Änderungen gehen verloren wenn Defaults sich ändern!
// Für API-Änderungen: Nach dem Boot einfach nochmal per API setzen.
void ControllerConfig::merge_with_defaults_smart() {
    // Einfach neue Defaults setzen - überschreibt alles
    // Das ist die einfachste Lösung: Defaults im Code ändern → werden übernommen
    // WICHTIG: API-Änderungen gehen verloren! Nach dem Boot einfach nochmal per API setzen.
    set_defaults();
    
    // Version aktualisieren
    config.config_version = CONFIG_VERSION;
    
    // Speichern
    save_to_eeprom();
}

// ===== CHANGE DETECTION & THRESHOLDS =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_UINT8_GETTER_SETTER(stick_change_threshold, stick_change_threshold, 0, 10, 0)

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(adaptive_center_threshold, enable_adaptive_center_threshold)
IMPL_UINT8_GETTER_SETTER(center_threshold, center_threshold, 0, 10, 3)

void ControllerConfig::set_center_threshold_range(uint8_t min, uint8_t max, bool persist) {
    if (min > 130) min = 126;
    if (max < 126) max = 130;
    if (min > max) { uint8_t temp = min; min = max; max = temp; }
    config.center_threshold_range_min = min;
    config.center_threshold_range_max = max;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_center_threshold_range_min() const {
    return config.center_threshold_range_min;
}

uint8_t ControllerConfig::get_center_threshold_range_max() const {
    return config.center_threshold_range_max;
}

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(change_detection, enable_change_detection)
IMPL_BOOL_GETTER_SETTER(report_counter, enable_report_counter)

// ===== FILTER OPTIONS =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(ema_filter, enable_ema_filter)
IMPL_FLOAT_GETTER_SETTER(ema_alpha, ema_alpha, 0.0f, 1.0f)
IMPL_BOOL_GETTER_SETTER(hysteresis, enable_hysteresis)
IMPL_UINT8_GETTER_SETTER(hysteresis_threshold, hysteresis_threshold, 0, 10, 2)
IMPL_BOOL_GETTER_SETTER(snapback_filter, enable_snapback_filter)
IMPL_UINT8_GETTER_SETTER(snapback_threshold, snapback_threshold, 0, 20, 10)
IMPL_UINT8_GETTER_SETTER(snapback_window, snapback_window, 1, 10, 3)

// ===== BUTTON DEBOUNCING =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(button_debounce, enable_button_debounce)
IMPL_UINT8_GETTER_SETTER(debounce_samples, debounce_samples, 1, 10, 4)

// ===== ASYMMETRIC RELEASE FILTER =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(asymmetric_release_filter, enable_asymmetric_release_filter)
IMPL_UINT8_GETTER_SETTER(release_guard_us, release_guard_us, 1, 50, 15)

// ===== STICK PROCESSING (DEADZONES & CLAMPS) =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_UINT8_GETTER_SETTER(inline_deadzone_radius, inline_deadzone_radius, 0, 20, 3)
IMPL_UINT8_GETTER_SETTER(radial_deadzone_radius, radial_deadzone_radius, 0, 127, 0)
IMPL_BOOL_GETTER_SETTER(outer_clamp, outer_clamp_enabled)

// ===== PERFORMANCE =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_BOOL_GETTER_SETTER(led_feedback, enable_led_feedback)

// ===== POLLING RATE =====

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_UINT8_GETTER_SETTER(polling_rate, polling_rate, 0, 3, 3)

// ===== ADC HARDWARE =====

// OPTIMIERUNG: Spezielle Validierung für adc_resolution (nur 8, 10, 12 erlaubt)
void ControllerConfig::set_adc_resolution(uint8_t resolution, bool persist) {
    if (resolution != 8 && resolution != 10 && resolution != 12) resolution = 12;  // Default: 12-bit
    config.adc_resolution = resolution;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_adc_resolution() const {
    return config.adc_resolution;
}

// OPTIMIERUNG: Spezielle Validierung für adc_averaging (0, 2, 3, 4, 5 erlaubt)
void ControllerConfig::set_adc_averaging(uint8_t averaging, bool persist) {
    // 0 = 1x, 2 = 4x, 3 = 8x, 4 = 16x, 5 = 32x
    if (averaging > 5 || (averaging > 0 && averaging < 2)) averaging = 3;  // Default: 8x
    config.adc_averaging = averaging;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_adc_averaging() const {
    return config.adc_averaging;
}

// OPTIMIERUNG: Makro für repetitive Getter/Setter
IMPL_UINT8_GETTER_SETTER(adc_clock_mhz, adc_clock_mhz, 50, 150, 150)
IMPL_UINT8_GETTER_SETTER(adc_sample_time, adc_sample_time, 0, 1, 0)

// ===== BACK BUTTON REMAPPING =====

// Back Left Target (0-16, siehe BackButtonTarget enum)
void ControllerConfig::set_back_left_target(uint8_t target, bool persist) {
    if (target > BACK_BTN_DPAD_RIGHT) target = BACK_BTN_L1;  // Default: L1
    config.back_left_target = target;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_back_left_target() const {
    return config.back_left_target;
}

// Back Right Target (0-16, siehe BackButtonTarget enum)
void ControllerConfig::set_back_right_target(uint8_t target, bool persist) {
    if (target > BACK_BTN_DPAD_RIGHT) target = BACK_BTN_R1;  // Default: R1
    config.back_right_target = target;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_back_right_target() const {
    return config.back_right_target;
}

// ===== BUTTON MAKROS =====

// Setze einen kompletten Makro-Slot (Trigger1, Trigger2, Target, Enabled)
void ControllerConfig::set_macro_slot(uint8_t slot, uint8_t trigger1, uint8_t trigger2, uint8_t target, bool enabled, bool persist) {
    if (slot >= 4) return;  // Ungültiger Slot

    // Validierung: Trigger1 und Trigger2 dürfen nicht gleich sein
    if (trigger1 == trigger2 && trigger1 != MACRO_TRIGGER_NONE) return;

    // Validierung: Trigger-Buttons im gültigen Bereich
    if (trigger1 > MACRO_TRIGGER_DPAD_RIGHT) trigger1 = MACRO_TRIGGER_NONE;
    if (trigger2 > MACRO_TRIGGER_DPAD_RIGHT) trigger2 = MACRO_TRIGGER_NONE;
    if (target > MACRO_TRIGGER_DPAD_RIGHT) target = MACRO_TRIGGER_NONE;

    config.macro_slots[slot].trigger_button_1 = trigger1;
    config.macro_slots[slot].trigger_button_2 = trigger2;
    config.macro_slots[slot].target_button = target;
    config.macro_slots[slot].enabled = enabled ? 1 : 0;

    if (persist) save_to_eeprom();
}

// Lese einen Makro-Slot aus
void ControllerConfig::get_macro_slot(uint8_t slot, uint8_t* trigger1, uint8_t* trigger2, uint8_t* target, bool* enabled) const {
    if (slot >= 4) return;  // Ungültiger Slot

    if (trigger1) *trigger1 = config.macro_slots[slot].trigger_button_1;
    if (trigger2) *trigger2 = config.macro_slots[slot].trigger_button_2;
    if (target) *target = config.macro_slots[slot].target_button;
    if (enabled) *enabled = config.macro_slots[slot].enabled != 0;
}

// Aktiviere/Deaktiviere einen Makro-Slot
void ControllerConfig::enable_macro_slot(uint8_t slot, bool enabled, bool persist) {
    if (slot >= 4) return;  // Ungültiger Slot

    config.macro_slots[slot].enabled = enabled ? 1 : 0;

    if (persist) save_to_eeprom();
}

// Prüfe ob ein Makro-Slot aktiviert ist
bool ControllerConfig::is_macro_slot_enabled(uint8_t slot) const {
    if (slot >= 4) return false;  // Ungültiger Slot

    return config.macro_slots[slot].enabled != 0;
}

// ===== VOLLSTÄNDIGES BUTTON REMAPPING =====

// Setze Button-Remapping für einen einzelnen Button
void ControllerConfig::set_button_remapping(uint8_t button_index, uint8_t target, bool persist) {
    if (button_index >= 18) return;  // Ungültiger Index

    // Validierung: Target muss im gültigen Bereich sein (0-17)
    if (target > BTN_MAP_DPAD_RIGHT) target = BTN_MAP_PASSTHROUGH;

    config.button_remapping[button_index] = target;

    // Synchronisiere Legacy-Back-Button-Werte (falls Back-Buttons geändert werden)
    if (button_index == REMAP_IDX_BACK_LEFT) {
        config.back_left_target = target;
    } else if (button_index == REMAP_IDX_BACK_RIGHT) {
        config.back_right_target = target;
    }

    if (persist) save_to_eeprom();
}

// Lese Button-Remapping für einen einzelnen Button
uint8_t ControllerConfig::get_button_remapping(uint8_t button_index) const {
    if (button_index >= 18) return BTN_MAP_PASSTHROUGH;  // Ungültiger Index

    return config.button_remapping[button_index];
}

// Setze alle Buttons auf PASSTHROUGH (keine Remapping)
void ControllerConfig::reset_button_remapping(bool persist) {
    for (uint8_t i = 0; i < 18; i++) {
        config.button_remapping[i] = BTN_MAP_PASSTHROUGH;
    }

    // Synchronisiere Legacy-Werte
    config.back_left_target = BTN_MAP_PASSTHROUGH;
    config.back_right_target = BTN_MAP_PASSTHROUGH;

    if (persist) save_to_eeprom();
}

// ===== PROFILE SYSTEM =====

// Setze aktives Profil und lade dessen Konfiguration
void ControllerConfig::set_active_profile(uint8_t profile, bool persist) {
    if (profile > 6) profile = 0;  // Nur Profile 0-6 erlaubt

    config.active_profile = profile;
    load_profile(profile);  // Profil-Konfiguration laden

    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_active_profile() const {
    return config.active_profile;
}

void ControllerConfig::set_profile_switch_button1(uint8_t button_mask, bool persist) {
    config.profile_switch_button1 = button_mask;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_profile_switch_button1() const {
    return config.profile_switch_button1;
}

void ControllerConfig::set_profile_switch_button2(uint8_t button_mask, bool persist) {
    config.profile_switch_button2 = button_mask;
    if (persist) save_to_eeprom();
}

uint8_t ControllerConfig::get_profile_switch_button2() const {
    return config.profile_switch_button2;
}

// Konvertiert MacroTriggerButton Enum zu DS4_BTN2 Bitmaske (nur für buttons2 Buttons!)
static inline uint8_t macro_trigger_to_btn2_mask(uint8_t trigger_button) {
    switch (trigger_button) {
        case MACRO_TRIGGER_L1:       return DS4_BTN2_L1;
        case MACRO_TRIGGER_R1:       return DS4_BTN2_R1;
        case MACRO_TRIGGER_L2:       return DS4_BTN2_L2;
        case MACRO_TRIGGER_R2:       return DS4_BTN2_R2;
        case MACRO_TRIGGER_L3:       return DS4_BTN2_L3;
        case MACRO_TRIGGER_R3:       return DS4_BTN2_R3;
        case MACRO_TRIGGER_SHARE:    return DS4_BTN2_SHARE;
        case MACRO_TRIGGER_OPTIONS:  return DS4_BTN2_OPTIONS;
        default:                     return 0;  // Ungültig für buttons2
    }
}

// Konvertiert Button-Config zu buttons1/buttons2/buttons3 Check
static inline bool check_button_pressed(uint8_t button_config, uint8_t buttons1, uint8_t buttons2, uint8_t buttons3) {
    // Sonderwerte für buttons3 (PS und Touchpad sind nicht in MacroTriggerButton)
    if (button_config == 0xFE) return (buttons3 & DS4_BTN3_PS) != 0;  // PS-Button
    if (button_config == 0xFD) return (buttons3 & DS4_BTN3_TOUCHPAD) != 0;  // Touchpad
    
    // Buttons2 (L1, R1, L2, R2, L3, R3, Share, Options)
    if (button_config >= MACRO_TRIGGER_L1 && button_config <= MACRO_TRIGGER_OPTIONS) {
        uint8_t mask = macro_trigger_to_btn2_mask(button_config);
        return (buttons2 & mask) != 0;
    }
    
    // Buttons1 (D-Pad)
    switch (button_config) {
        case MACRO_TRIGGER_DPAD_UP:    return (buttons1 & 0x0F) == DS4_BTN1_DPAD_UP;
        case MACRO_TRIGGER_DPAD_DOWN:  return (buttons1 & 0x0F) == DS4_BTN1_DPAD_DOWN;
        case MACRO_TRIGGER_DPAD_LEFT:  return (buttons1 & 0x0F) == DS4_BTN1_DPAD_LEFT;
        case MACRO_TRIGGER_DPAD_RIGHT: return (buttons1 & 0x0F) == DS4_BTN1_DPAD_RIGHT;
        default: return false;
    }
}

// Runtime-Profil-Wechsel Handler (wird von updateController() aufgerufen)
// Konfigurierbare Button-Kombination für 2 Sekunden halten → Profil wechseln!
// Gibt true zurück wenn ein Profil-Wechsel stattgefunden hat (für Cache-Invalidierung)
bool ControllerConfig::check_runtime_profile_switch(uint8_t buttons1, uint8_t buttons3) {
    static uint16_t led_counter = 0;
    static uint8_t blinks_remaining = 0;
    static bool led_state = false;
    static uint16_t hold_timer = 0;  // Timer für 1 Sekunde halten
    static uint8_t target_profile = 0xFF;  // Ziel-Profil (0xFF = kein Wechsel)
    bool profile_switched = false;

    // *** NEUE LOGIC: Prüfe BEIDE Tastenkombinationen ***
    // PS + D-Pad Up → Profil 1
    bool ps_pressed = (buttons3 & DS4_BTN3_PS) != 0;
    uint8_t dpad = buttons1 & 0x0F;
    bool dpad_up_pressed = (dpad == DS4_BTN1_DPAD_UP || dpad == DS4_BTN1_DPAD_UPRIGHT || dpad == DS4_BTN1_DPAD_UPLEFT);
    bool dpad_down_pressed = (dpad == DS4_BTN1_DPAD_DOWN || dpad == DS4_BTN1_DPAD_DOWNRIGHT || dpad == DS4_BTN1_DPAD_DOWNLEFT);

    bool combo_profile1 = ps_pressed && dpad_up_pressed;    // PS + D-Pad Up → Profil 1
    bool combo_profile2 = ps_pressed && dpad_down_pressed;  // PS + D-Pad Down → Profil 2

    bool any_combo_pressed = combo_profile1 || combo_profile2;

    // Hold-Timer: Buttons müssen 1 Sekunde gehalten werden
    const uint16_t HOLD_FRAMES = 8000;  // 8000 Frames * 125µs = 1000ms = 1 Sekunde

    if (any_combo_pressed) {
        // Eine Kombination gedrückt → Ziel-Profil setzen (nur beim ersten Frame)
        if (hold_timer == 0) {
            if (combo_profile1) {
                target_profile = 1;  // Profil 1
            } else if (combo_profile2) {
                target_profile = 2;  // Profil 2
            }
        }

        // Timer hochzählen
        if (hold_timer < HOLD_FRAMES) {
            hold_timer++;
            if (hold_timer >= HOLD_FRAMES && target_profile != 0xFF) {
                // 1 Sekunde erreicht → Profil wechseln!
                set_active_profile(target_profile, true);  // true = in EEPROM speichern
                profile_switched = true;  // Signal für Cache-Invalidierung

                // LED-Feedback starten: Anzahl Blinks = Profil-Nummer + 1 (non-blocking mit Counter!)
                if (config.enable_led_feedback != 0) {
                    blinks_remaining = target_profile + 1;  // 1 Blink = Profil 0, 2 Blinks = Profil 1, 3 Blinks = Profil 2
                    led_counter = 800;  // 800 Frames * 125µs = 100ms
                    led_state = true;  // LED an
                    digitalWrite(PIN_LED, HIGH);
                }

                target_profile = 0xFF;  // Reset
            }
        }
    } else {
        // Buttons nicht mehr beide gedrückt → Timer zurücksetzen
        hold_timer = 0;
        target_profile = 0xFF;
    }
    
    // LED-Feedback für Profil-Wechsel (non-blocking State-Machine!)
    if (led_counter > 0) {
        led_counter--;
        if (led_counter == 0) {
            if (led_state) {
                // LED war an → jetzt aus
                digitalWrite(PIN_LED, LOW);
                blinks_remaining--;
                if (blinks_remaining > 0) {
                    // Nächstes Blink: Pause (LED aus bleiben)
                    led_counter = 800;  // 100ms Pause
                }
                led_state = false;
            } else {
                // Pause vorbei → LED wieder an für nächstes Blink
                digitalWrite(PIN_LED, HIGH);
                led_counter = 800;  // 100ms LED an
                led_state = true;
            }
        }
    }
    
    return profile_switched;
}

// Lädt ein Profil (Button-Remapping, Makros, etc.)
// Diese Funktion muss für jedes Profil die entsprechenden Werte setzen
void ControllerConfig::load_profile(uint8_t profile) {
    // Profil-spezifische Konfiguration laden
    switch (profile) {
        case 0:  // PROFIL 0: STANDARD
            config.button_remapping[REMAP_IDX_CROSS] = PROFILE_0_REMAP_CROSS;
            config.button_remapping[REMAP_IDX_CIRCLE] = PROFILE_0_REMAP_CIRCLE;
            config.button_remapping[REMAP_IDX_SQUARE] = PROFILE_0_REMAP_SQUARE;
            config.button_remapping[REMAP_IDX_TRIANGLE] = PROFILE_0_REMAP_TRIANGLE;
            config.button_remapping[REMAP_IDX_L1] = PROFILE_0_REMAP_L1;
            config.button_remapping[REMAP_IDX_R1] = PROFILE_0_REMAP_R1;
            config.button_remapping[REMAP_IDX_L2] = PROFILE_0_REMAP_L2;
            config.button_remapping[REMAP_IDX_R2] = PROFILE_0_REMAP_R2;
            config.button_remapping[REMAP_IDX_L3] = PROFILE_0_REMAP_L3;
            config.button_remapping[REMAP_IDX_R3] = PROFILE_0_REMAP_R3;
            config.button_remapping[REMAP_IDX_SHARE] = PROFILE_0_REMAP_SHARE;
            config.button_remapping[REMAP_IDX_OPTIONS] = PROFILE_0_REMAP_OPTIONS;
            config.button_remapping[REMAP_IDX_DPAD_UP] = PROFILE_0_REMAP_DPAD_UP;
            config.button_remapping[REMAP_IDX_DPAD_DOWN] = PROFILE_0_REMAP_DPAD_DOWN;
            config.button_remapping[REMAP_IDX_DPAD_LEFT] = PROFILE_0_REMAP_DPAD_LEFT;
            config.button_remapping[REMAP_IDX_DPAD_RIGHT] = PROFILE_0_REMAP_DPAD_RIGHT;
            config.button_remapping[REMAP_IDX_BACK_LEFT] = PROFILE_0_REMAP_BACK_LEFT;
            config.button_remapping[REMAP_IDX_BACK_RIGHT] = PROFILE_0_REMAP_BACK_RIGHT;

            config.macro_slots[0].trigger_button_1 = PROFILE_0_MACRO_1_TRIGGER_1;
            config.macro_slots[0].trigger_button_2 = PROFILE_0_MACRO_1_TRIGGER_2;
            config.macro_slots[0].target_button = PROFILE_0_MACRO_1_TARGET;
            config.macro_slots[0].enabled = PROFILE_0_MACRO_1_ENABLED;

            config.macro_slots[1].trigger_button_1 = PROFILE_0_MACRO_2_TRIGGER_1;
            config.macro_slots[1].trigger_button_2 = PROFILE_0_MACRO_2_TRIGGER_2;
            config.macro_slots[1].target_button = PROFILE_0_MACRO_2_TARGET;
            config.macro_slots[1].enabled = PROFILE_0_MACRO_2_ENABLED;

            config.macro_slots[2].trigger_button_1 = PROFILE_0_MACRO_3_TRIGGER_1;
            config.macro_slots[2].trigger_button_2 = PROFILE_0_MACRO_3_TRIGGER_2;
            config.macro_slots[2].target_button = PROFILE_0_MACRO_3_TARGET;
            config.macro_slots[2].enabled = PROFILE_0_MACRO_3_ENABLED;

            config.macro_slots[3].trigger_button_1 = PROFILE_0_MACRO_4_TRIGGER_1;
            config.macro_slots[3].trigger_button_2 = PROFILE_0_MACRO_4_TRIGGER_2;
            config.macro_slots[3].target_button = PROFILE_0_MACRO_4_TARGET;
            config.macro_slots[3].enabled = PROFILE_0_MACRO_4_ENABLED;
            break;

        case 1:  // PROFIL 1: FPS
            config.button_remapping[REMAP_IDX_CROSS] = PROFILE_1_REMAP_CROSS;
            config.button_remapping[REMAP_IDX_CIRCLE] = PROFILE_1_REMAP_CIRCLE;
            config.button_remapping[REMAP_IDX_SQUARE] = PROFILE_1_REMAP_SQUARE;
            config.button_remapping[REMAP_IDX_TRIANGLE] = PROFILE_1_REMAP_TRIANGLE;
            config.button_remapping[REMAP_IDX_L1] = PROFILE_1_REMAP_L1;
            config.button_remapping[REMAP_IDX_R1] = PROFILE_1_REMAP_R1;
            config.button_remapping[REMAP_IDX_L2] = PROFILE_1_REMAP_L2;
            config.button_remapping[REMAP_IDX_R2] = PROFILE_1_REMAP_R2;
            config.button_remapping[REMAP_IDX_L3] = PROFILE_1_REMAP_L3;
            config.button_remapping[REMAP_IDX_R3] = PROFILE_1_REMAP_R3;
            config.button_remapping[REMAP_IDX_SHARE] = PROFILE_1_REMAP_SHARE;
            config.button_remapping[REMAP_IDX_OPTIONS] = PROFILE_1_REMAP_OPTIONS;
            config.button_remapping[REMAP_IDX_DPAD_UP] = PROFILE_1_REMAP_DPAD_UP;
            config.button_remapping[REMAP_IDX_DPAD_DOWN] = PROFILE_1_REMAP_DPAD_DOWN;
            config.button_remapping[REMAP_IDX_DPAD_LEFT] = PROFILE_1_REMAP_DPAD_LEFT;
            config.button_remapping[REMAP_IDX_DPAD_RIGHT] = PROFILE_1_REMAP_DPAD_RIGHT;
            config.button_remapping[REMAP_IDX_BACK_LEFT] = PROFILE_1_REMAP_BACK_LEFT;
            config.button_remapping[REMAP_IDX_BACK_RIGHT] = PROFILE_1_REMAP_BACK_RIGHT;

            config.macro_slots[0].trigger_button_1 = PROFILE_1_MACRO_1_TRIGGER_1;
            config.macro_slots[0].trigger_button_2 = PROFILE_1_MACRO_1_TRIGGER_2;
            config.macro_slots[0].target_button = PROFILE_1_MACRO_1_TARGET;
            config.macro_slots[0].enabled = PROFILE_1_MACRO_1_ENABLED;

            config.macro_slots[1].trigger_button_1 = PROFILE_1_MACRO_2_TRIGGER_1;
            config.macro_slots[1].trigger_button_2 = PROFILE_1_MACRO_2_TRIGGER_2;
            config.macro_slots[1].target_button = PROFILE_1_MACRO_2_TARGET;
            config.macro_slots[1].enabled = PROFILE_1_MACRO_2_ENABLED;

            config.macro_slots[2].trigger_button_1 = PROFILE_1_MACRO_3_TRIGGER_1;
            config.macro_slots[2].trigger_button_2 = PROFILE_1_MACRO_3_TRIGGER_2;
            config.macro_slots[2].target_button = PROFILE_1_MACRO_3_TARGET;
            config.macro_slots[2].enabled = PROFILE_1_MACRO_3_ENABLED;

            config.macro_slots[3].trigger_button_1 = PROFILE_1_MACRO_4_TRIGGER_1;
            config.macro_slots[3].trigger_button_2 = PROFILE_1_MACRO_4_TRIGGER_2;
            config.macro_slots[3].target_button = PROFILE_1_MACRO_4_TARGET;
            config.macro_slots[3].enabled = PROFILE_1_MACRO_4_ENABLED;
            break;

        default:
            // Fallback: Profil 0 laden
            load_profile(0);
            break;
    }

    // Legacy Back-Button-Werte synchronisieren
    config.back_left_target = config.button_remapping[REMAP_IDX_BACK_LEFT];
    config.back_right_target = config.button_remapping[REMAP_IDX_BACK_RIGHT];
}

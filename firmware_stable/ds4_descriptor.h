#ifndef DS4_DESCRIPTOR_H
#define DS4_DESCRIPTOR_H

#include <Arduino.h>

// ===== DS4 USB DESKRIPTOR (GP2040-CE Referenz) =====
// Vendor ID: 0x054C (Sony Corporation)
// Product ID: 0x09CC (Wireless Controller - DualShock 4 v2)
// Referenz: GP2040-CE PS4Driver.cpp

// DS4 Input Report Format (64 bytes total)
struct DS4Report {
    uint8_t report_id;          // 0x01
    uint8_t left_stick_x;       // 0-255 (128 = center)
    uint8_t left_stick_y;       // 0-255 (128 = center)
    uint8_t right_stick_x;      // 0-255 (128 = center)
    uint8_t right_stick_y;      // 0-255 (128 = center)

    // Buttons
    uint8_t buttons1;           // D-Pad (bits 0-3), Square, Cross, Circle, Triangle (bits 4-7)
    uint8_t buttons2;           // L1, R1, L2, R2, Share, Options, L3, R3
    uint8_t buttons3;           // PS, Touchpad

    uint8_t left_trigger;       // L2 analog (0-255)
    uint8_t right_trigger;      // R2 analog (0-255)

    uint16_t timestamp;         // Controller internal timer
    uint8_t battery;            // Battery level

    int16_t gyro_x;             // Gyro X-Achse
    int16_t gyro_y;             // Gyro Y-Achse
    int16_t gyro_z;             // Gyro Z-Achse

    int16_t accel_x;            // Accelerometer X
    int16_t accel_y;            // Accelerometer Y
    int16_t accel_z;            // Accelerometer Z

    uint8_t reserved[5];        // Reserved bytes

    uint8_t ext_data[24];       // Extended data (Touchpad, etc.)
    uint32_t crc32;             // CRC32 checksum
    uint8_t padding[6];         // Padding auf 64 Bytes (RawHID braucht genau 64!)
} __attribute__((packed));

// ===== DS4 FEATURE REPORTS (GP2040-CE Referenz) =====
// Feature Report 0x02: Controller Calibration (36 bytes)
// Referenz: GP2040-CE PS4Driver.cpp output_0x02
struct DS4FeatureReport_0x02 {
    uint8_t report_id;          // 0x02
    uint8_t data[35];           // Calibration data
} __attribute__((packed));

// Feature Report 0x03: Controller Definition (36 bytes)
// Referenz: GP2040-CE PS4Driver.cpp controllerConfig
struct DS4FeatureReport_0x03 {
    uint8_t report_id;          // 0x03
    uint16_t hid_usage;          // 0x2721
    uint8_t mystery0;            // 0x04
    uint8_t features;            // Feature flags
    uint8_t controller_type;    // Controller type (0x00 = PS4_CONTROLLER)
    uint8_t touchpad_param[2];   // Touchpad parameters
    uint8_t imu_config[12];      // IMU configuration
    uint16_t magic_id;           // 0x0D0D
    uint8_t mystery1[3];        // Unknown
    uint8_t wheel_param[3];      // Wheel parameters (if applicable)
    uint8_t mystery2[9];        // Unknown
} __attribute__((packed));

// Feature Report 0x12: MAC Address (16 bytes)
// Referenz: GP2040-CE PS4Driver.cpp output_0x12
struct DS4FeatureReport_0x12 {
    uint8_t report_id;          // 0x12
    uint8_t device_mac[6];       // Device MAC address
    uint8_t bt_device_class[3];  // Bluetooth device class
    uint8_t host_mac[6];         // Host MAC address
} __attribute__((packed));

// Feature Report 0xA3: Firmware Version (48 bytes)
// Referenz: GP2040-CE PS4Driver.cpp output_0xa3
// KRITISCH für Battlefield 6 / CoD Authentifizierung!
struct DS4FeatureReport_0xA3 {
    uint8_t report_id;          // 0xA3
    uint8_t date[11];            // Date string: "Jun  9 2017\0"
    uint8_t time[8];             // Time string: "12:36:41\0"
    uint8_t reserved[28];       // Reserved bytes
} __attribute__((packed));

// ===== DS4 OUTPUT REPORT (GP2040-CE Referenz) =====
// Output Report 0x05: LED and Rumble (31 bytes)
// Referenz: GP2040-CE PS4Driver.cpp ps4Features
struct DS4OutputReport_0x05 {
    uint8_t report_id;          // 0x05
    uint8_t rumble_right;       // Right motor (0-255)
    uint8_t rumble_left;        // Left motor (0-255)
    uint8_t led_red;            // LED Red (0-255)
    uint8_t led_green;          // LED Green (0-255)
    uint8_t led_blue;           // LED Blue (0-255)
    uint8_t led_blink_on;       // LED blink on time (centiseconds)
    uint8_t led_blink_off;      // LED blink off time (centiseconds)
    uint8_t reserved[23];       // Reserved bytes
} __attribute__((packed));

// Button-Masken für buttons1
#define DS4_BTN1_DPAD_UP        0x00
#define DS4_BTN1_DPAD_UPRIGHT   0x01
#define DS4_BTN1_DPAD_RIGHT     0x02
#define DS4_BTN1_DPAD_DOWNRIGHT 0x03
#define DS4_BTN1_DPAD_DOWN      0x04
#define DS4_BTN1_DPAD_DOWNLEFT  0x05
#define DS4_BTN1_DPAD_LEFT      0x06
#define DS4_BTN1_DPAD_UPLEFT    0x07
#define DS4_BTN1_DPAD_NONE      0x08
#define DS4_BTN1_SQUARE         0x10
#define DS4_BTN1_CROSS          0x20
#define DS4_BTN1_CIRCLE         0x40
#define DS4_BTN1_TRIANGLE       0x80

// Button-Masken für buttons2
#define DS4_BTN2_L1             0x01
#define DS4_BTN2_R1             0x02
#define DS4_BTN2_L2             0x04
#define DS4_BTN2_R2             0x08
#define DS4_BTN2_SHARE          0x10
#define DS4_BTN2_OPTIONS        0x20
#define DS4_BTN2_L3             0x40
#define DS4_BTN2_R3             0x80

// Button-Masken für buttons3
#define DS4_BTN3_PS             0x01
#define DS4_BTN3_TOUCHPAD       0x02

// ===== DS4 FEATURE REPORT IDs (GP2040-CE Referenz) =====
#define DS4_FEATURE_REPORT_CALIBRATION     0x02  // Controller calibration
#define DS4_FEATURE_REPORT_DEFINITION      0x03  // Controller definition
#define DS4_FEATURE_REPORT_MAC_ADDRESS     0x12  // MAC address
#define DS4_FEATURE_REPORT_FIRMWARE        0xA3  // Firmware version (KRITISCH für BF6!)
#define DS4_FEATURE_REPORT_SIGNATURE_NONCE 0xF1  // Signature nonce (PS4 auth)
#define DS4_FEATURE_REPORT_SIGNING_STATE   0xF2  // Signing state
#define DS4_FEATURE_REPORT_RESET_AUTH      0xF3  // Reset authentication

// ===== DS4 OUTPUT REPORT IDs =====
#define DS4_OUTPUT_REPORT_LED_RUMBLE       0x05  // LED and rumble control

// ===== DS4 OUTPUT REPORT GLOBAL VARIABLES =====
// Diese Variablen werden vom USB Output Report Handler gesetzt
// und können im Hauptcode verwendet werden (z.B. für LED-Steuerung)
extern volatile uint8_t ds4_led_red;      // LED Red (0-255)
extern volatile uint8_t ds4_led_green;    // LED Green (0-255)
extern volatile uint8_t ds4_led_blue;     // LED Blue (0-255)
extern volatile uint8_t ds4_led_blink_on;  // LED blink on time (centiseconds)
extern volatile uint8_t ds4_led_blink_off; // LED blink off time (centiseconds)
extern volatile uint8_t ds4_rumble_right;  // Right motor (0-255)
extern volatile uint8_t ds4_rumble_left;   // Left motor (0-255)
extern volatile bool ds4_output_report_received; // Flag: Neuer Output Report empfangen

// ===== DS4 CONTROLLER TYPES (GP2040-CE Referenz) =====
#define DS4_CONTROLLER_TYPE_GAMEPAD        0x00  // Standard gamepad
#define DS4_CONTROLLER_TYPE_WHEEL          0x01  // Racing wheel
#define DS4_CONTROLLER_TYPE_ARCADE_STICK   0x07  // Arcade stick (PS5 compatibility)

// HID Report Descriptor für DS4
// Referenz: GP2040-CE PS4Driver.cpp get_hid_descriptor_report_cb
// HINWEIS: Dies muss in usb_desc.c des Teensy-Cores integriert werden
// Der vollständige Descriptor mit Feature Reports ist in ds4_descriptor_vigem.h
// Dieser vereinfachte Descriptor wird aktuell verwendet
const uint8_t ds4_hid_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)

    // Analog Sticks
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Buttons (16 Buttons)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (Button 1)
    0x29, 0x10,        //   Usage Maximum (Button 16)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    // Triggers
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x33,        //   Usage (Rx)
    0x09, 0x34,        //   Usage (Ry)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data,Var,Abs)

    0xC0               // End Collection
};

// ===== DS4 FEATURE REPORT HELPER FUNCTIONS (GP2040-CE Referenz) =====
// Diese Funktionen können verwendet werden, um Feature Reports zu generieren

// Feature Report 0x02: Controller Calibration
// Referenz: GP2040-CE PS4Driver.cpp output_0x02 (36 bytes OHNE Report ID)
inline void ds4_get_feature_report_0x02(uint8_t* buffer, uint16_t buffer_size) {
    if (buffer_size < 37) return;  // 36 bytes data + 1 byte Report ID
    
    // GP2040-CE Standard Calibration Data (36 bytes OHNE Report ID)
    static const uint8_t calibration_data[36] = {
        0xFE, 0xFF, 0x0E, 0x00, 0x04, 0x00, 0xD4, 0x22,
        0x2A, 0xDD, 0xBB, 0x22, 0x5E, 0xDD, 0x81, 0x22,
        0x84, 0xDD, 0x1C, 0x02, 0x1C, 0x02, 0x85, 0x1F,
        0xB0, 0xE0, 0xC6, 0x20, 0xB5, 0xE0, 0xB1, 0x20,
        0x83, 0xDF, 0x0C, 0x00
    };
    buffer[0] = 0x02;  // Report ID
    memcpy(&buffer[1], calibration_data, 36);
}

// Feature Report 0x03: Controller Definition
// Referenz: GP2040-CE PS4Driver.cpp controllerConfig
inline void ds4_get_feature_report_0x03(uint8_t* buffer, uint16_t buffer_size, uint8_t controller_type = DS4_CONTROLLER_TYPE_GAMEPAD) {
    if (buffer_size < 36) return;
    
    // GP2040-CE Standard Controller Definition
    buffer[0] = 0x03;           // Report ID
    buffer[1] = 0x21;           // HID Usage LSB
    buffer[2] = 0x27;           // HID Usage MSB (0x2721)
    buffer[3] = 0x04;           // Mystery0
    buffer[4] = 0xEF;           // Features (enableController, enableMotion, enableLED, enableRumble, etc.)
    buffer[5] = controller_type; // Controller Type
    buffer[6] = 0x2C;           // Touchpad Param 0
    buffer[7] = 0x56;           // Touchpad Param 1
    // IMU Config (bytes 8-19)
    buffer[8] = 0x08; buffer[9] = 0x00;   // Gyro Range
    buffer[10] = 0x3D; buffer[11] = 0x00; // Gyro Res Per Deg Denom
    buffer[12] = 0xE8; buffer[13] = 0x03; // Gyro Res Per Deg Numer
    buffer[14] = 0x04; buffer[15] = 0x00;  // Accel Range
    buffer[16] = 0xFF; buffer[17] = 0x7F;  // Accel Res Per G
    buffer[18] = 0x0D; buffer[19] = 0x0D;  // Magic ID
    // Rest: zeros
    memset(&buffer[20], 0, 16);
}

// Feature Report 0x12: MAC Address
// Referenz: GP2040-CE PS4Driver.cpp output_0x12
inline void ds4_get_feature_report_0x12(uint8_t* buffer, uint16_t buffer_size) {
    if (buffer_size < 16) return;
    
    buffer[0] = 0x12;           // Report ID
    // Device MAC Address (6 bytes) - kann konfiguriert werden
    memset(&buffer[1], 0x00, 6);
    // BT Device Class (3 bytes)
    buffer[7] = 0x08;
    buffer[8] = 0x25;
    buffer[9] = 0x00;
    // Host MAC Address (6 bytes) - kann konfiguriert werden
    memset(&buffer[10], 0x00, 6);
}

// Feature Report 0xA3: Firmware Version
// Referenz: GP2040-CE PS4Driver.cpp output_0xa3
// KRITISCH für Battlefield 6 / CoD Authentifizierung!
inline void ds4_get_feature_report_0xa3(uint8_t* buffer, uint16_t buffer_size) {
    if (buffer_size < 48) return;
    
    buffer[0] = 0xA3;           // Report ID
    // Date: "Jun  9 2017\0"
    memcpy(&buffer[1], "Jun  9 2017", 11);
    buffer[11] = 0x00;
    // Time: "12:36:41\0"
    memcpy(&buffer[12], "12:36:41", 8);
    buffer[20] = 0x00;
    // Reserved bytes
    buffer[21] = 0x00; buffer[22] = 0x01; buffer[23] = 0x08;
    buffer[24] = 0xB4; buffer[25] = 0x01; buffer[26] = 0x00;
    buffer[27] = 0x00; buffer[28] = 0x00; buffer[29] = 0x07;
    buffer[30] = 0xA0; buffer[31] = 0x10; buffer[32] = 0x20;
    buffer[33] = 0x00; buffer[34] = 0xA0; buffer[35] = 0x02;
    buffer[36] = 0x00;
    // Rest: zeros
    memset(&buffer[37], 0, 11);
}

#endif // DS4_DESCRIPTOR_H

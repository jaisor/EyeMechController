#pragma once

#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>
#include <StreamUtils.h>

#define WIFI        // 2.4Ghz wifi access point
#define LED         // Individually addressible LED strip
#define JOYSTICK    // Joystick with two axes + switch (active LOW)
//#define OLED        // OLED display
//#define BUTTONS     // Buttons

//#define KEYPAD      // Buttons
//#define RING_LIGHT

//#define DEBUG_MOCK_HP
//#define DISABLE_LOGGING
#ifndef DISABLE_LOGGING
  #define LOG_LEVEL LOG_LEVEL_VERBOSE
#endif

#define WEB_LOGGING // When enabled log is available at http://<device_ip>/log
#ifdef WEB_LOGGING
  #define WEB_LOG_LEVEL LOG_LEVEL_VERBOSE
  #define WEB_LOG_MAX_SIZE 8192  // Cap log buffer to 8KB to prevent heap exhaustion
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
  #define SERIAL_MONITOR_BAUD 460800
  //#define DISABLE_LOGGING // Xiao's setup with USB requires serial to be initialized on the IDE else it blocks
#else
  #define SERIAL_MONITOR_BAUD 115200
#endif

#define EEPROM_FACTORY_RESET 0           // Byte to be used for factory reset device fails to start or is rebooted within 1 sec 3 consequitive times
#define EEPROM_CONFIGURATION_START 1     // First EEPROM byte to be used for storing the configuration

#define FACTORY_RESET_CLEAR_TIMER_MS 2000   // Clear factory reset counter when elapsed, considered smooth boot

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #define INTERNAL_LED_PIN GPIO_NUM_8
  #define DEVICE_NAME "ESP32C3LED"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define INTERNAL_LED_PIN LED_BUILTIN
  #define DEVICE_NAME "ESP32S3LED"
#elif defined(SEEED_XIAO_M0)
  #define INTERNAL_LED_PIN     13
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #define INTERNAL_LED_PIN LED_BUILTIN
  #define DEVICE_NAME "ESP32LED"
#elif defined(ESP8266)
  #define INTERNAL_LED_PIN LED_BUILTIN
  #define DEVICE_NAME "ESP8266LED"
#else
  #define INTERNAL_LED_PIN LED_BUILTIN
  #define DEVICE_NAME "ESPXXLED"
#endif

#ifdef WIFI
    #define WIFI_SSID DEVICE_NAME
    #define WIFI_PASS "password123"

    // If unable to connect, it will create a soft accesspoint
    #define WIFI_FALLBACK_SSID DEVICE_NAME // device chip id will be suffixed
    #define WIFI_FALLBACK_PASS "password123"

    #define NTP_SERVER "pool.ntp.org"
    #define NTP_GMT_OFFSET_SEC 0  // GMT (default on factory reset)
    #define NTP_DAYLIGHT_OFFSET_SEC 0  // Deprecated, kept for compatibility

    // Web server
    #define WEB_SERVER_PORT 80
#endif

#ifdef LED
    #define LED_CHANGE_MODE_SEC   0
    #if defined(CONFIG_IDF_TARGET_ESP32C3)
      #define LED_PIN GPIO_NUM_10
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
      #define LED_PIN GPIO_NUM_2
    #elif defined(SEEED_XIAO_M0)
      #define LED_PIN 10
    #elif defined(CONFIG_IDF_TARGET_ESP32)
      #define LED_PIN GPIO_NUM_12
    #elif defined(ESP8266)
        #define LED_PIN D3
    #else
      #define LED_PIN 1
    #endif
    // 267 for RingLight, 480 for PingPong table light
    #ifdef RING_LIGHT 
        #define LED_STRIP_SIZE 267
        #define OUTTER_RING_SIZE 141
    #else
        #define LED_STRIP_SIZE 128
        #define OUTTER_RING_SIZE 240
    #endif
    #define LED_BRIGHTNESS 1  // 0-1, 1-max brightness, make sure your LEDs are powered accordingly
    #define LED_COLOR_ORDER GRB
#endif

#ifdef BUTTONS
  #define BUTTON_1_PIN     GPIO_NUM_0
  #define BUTTON_2_PIN     GPIO_NUM_1
#endif

// Joystick: two analog potentiometers (X/Y axes) + one digital switch
// Uses ADC1 pins only (ADC2 conflicts with WiFi on ESP32 variants).
// Switch uses INPUT_PULLUP; reading is active LOW (pressed = true).
//#define JOYSTICK
#ifdef JOYSTICK
  #if defined(CONFIG_IDF_TARGET_ESP32C3)
    // ADC1 CH3/CH4 (GPIO3/4) for axes; SW on GPIO5 (digital-only, no ADC1) to avoid
    // ADC crosstalk — GPIO2/3/4 share the ADC1 mux, so a switch on GPIO2 caused
    // spurious axis readings when pressed. GPIO5 has no ADC1 connection.
    #define JOY_X_PIN   GPIO_NUM_3
    #define JOY_Y_PIN   GPIO_NUM_4
    #define JOY_SW_PIN  GPIO_NUM_5
  #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    // ADC1 CH3/CH6 are free on S3 when I2C is on GPIO5/6 and LED on GPIO2
    #define JOY_X_PIN   GPIO_NUM_4
    #define JOY_Y_PIN   GPIO_NUM_7
    #define JOY_SW_PIN  GPIO_NUM_8
  #elif defined(CONFIG_IDF_TARGET_ESP32)
    // GPIO34/35 are input-only ADC1 pins safe to use alongside WiFi
    #define JOY_X_PIN   GPIO_NUM_34
    #define JOY_Y_PIN   GPIO_NUM_35
    #define JOY_SW_PIN  GPIO_NUM_32
  #elif defined(ESP8266)
    // ESP8266 has a single 10-bit ADC (A0); Y axis cannot be read independently
    #define JOY_X_PIN   A0
    #define JOY_Y_PIN   A0
    #define JOY_SW_PIN  D5
  #else
    #define JOY_X_PIN   34
    #define JOY_Y_PIN   35
    #define JOY_SW_PIN  32
  #endif
  #define JOY_READ_INTERVAL_MS  20   // Poll joystick every 20 ms (~50 Hz)
  // Max raw ADC value: ESP8266 = 10-bit (1023), ESP32 variants = 12-bit (4095)
  #ifdef ESP8266
    #define JOY_ADC_MAX  1023
  #else
    #define JOY_ADC_MAX  4095
  #endif
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
  #ifdef OLED // ESD32C3 has a built-in OLED ie ESP32C4 Dev board
    #define OLED_SCREEN_WIDTH 72 // OLED display width, in pixels
    #define OLED_SCREEN_HEIGHT 40 // OLED display height, in pixel
    #define OLED_I2C_ID  0x3C
  #endif
#endif

// Standard servo pulse width bounds (12-bit ticks at 50 Hz)
// 50 Hz → 20 ms period → 4096 ticks; 1 tick ≈ 4.88 µs
#define SERVO_PULSE_MIN  150   // ~0°   (~730 µs)
#define SERVO_PULSE_MAX  600   // ~180° (~2930 µs)

struct configuration_t {

    #ifdef WIFI
        char wifiSsid[32];
        char wifiPassword[63];

        int8_t wifiPower;

        // ntp
        char ntpServer[128];
        long gmtOffset_sec;
        int daylightOffset_sec;
    #endif

    #ifdef LED
        float ledBrightness;
        uint8_t ledMode;
        uint8_t ledType;
        unsigned long ledDelayMs;
        unsigned long ledCycleModeMs;
        uint16_t ledStripSize;
        float psLedBrightness;
        int8_t psStartHour;
        int8_t psEndHour;
        uint8_t cycleModesCount;
        uint8_t cycleModesList[32];  // Up to 32 modes in cycle list
    #endif

    char name[128];

    uint8_t ledEnabled;

    // Eye mechanism servo ranges in raw PWM ticks (150 = ~0°, 600 = ~180°), one min/max pair per servo channel.
    // Indexed by EYE_RIGHT_LR(0), EYE_RIGHT_UD(1), EYE_RIGHT_LID(2),
    //            EYE_LEFT_LR(3),  EYE_LEFT_UD(4),  EYE_LEFT_LID(5)
    uint16_t eyeServoRangeMin[6];
    uint16_t eyeServoRangeMax[6];
    // Midpoint trim per channel: actual PWM pulse sent when the servo is commanded to centre.
    // Defaults to (min+max)/2. Must stay within [eyeServoRangeMin, eyeServoRangeMax].
    uint16_t eyeServoTrim[6];

    // Servo inversion bitmask: bit N set means channel N PWM is inverted (PULSE_MIN + PULSE_MAX - pulse)
    uint8_t servoInvertedMask;

    #ifdef JOYSTICK
    // 1 = joystick X/Y drives eye gaze and button closes eyelids; 0 = joystick read-only
    uint8_t joystickEyeControl;
    #endif

    char _loaded[7]; // used to check if EEPROM was correctly set    
};

extern configuration_t configuration;
#ifdef WEB_LOGGING
  extern StringPrint logStream;
  void trimLogStream();
#endif

uint8_t EEPROM_initAndCheckFactoryReset();
void EEPROM_clearFactoryReset();

void EEPROM_saveConfig();
void EEPROM_loadConfig();
void EEPROM_wipe();

uint32_t CONFIG_getDeviceId();
unsigned long CONFIG_getUpTime();

void intLEDOn();
void intLEDOff();
void intLEDBlink(uint16_t ms);

#ifdef LED
    float CONFIG_getLedBrightness(bool force = false);
#endif

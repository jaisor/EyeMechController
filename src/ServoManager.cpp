#include "ServoManager.h"

#include <ArduinoLog.h>
#include <Wire.h>
#include "Configuration.h"

CServoManager::CServoManager()
    : pwm(Adafruit_PWMServoDriver(0x40))
{
    memset(pulses, 0, sizeof(pulses));
}

CServoManager::~CServoManager() {}

void CServoManager::setup() {
    Log.infoln("[ServoManager] Initializing PWM servo driver at 0x40");
    pwm.begin();
    pwm.setOscillatorFrequency(27000000); // Calibrated 27 MHz internal oscillator
    pwm.setPWMFreq(50);                   // 50 Hz – standard for analog servos
    delay(10);

    // Drive all 6 channels to minimum pulse on startup
    for (uint8_t ch = 0; ch < SERVO_COUNT; ch++) {
        setPulse(ch, (configuration.eyeServoRangeMax[ch] - configuration.eyeServoRangeMin[ch]) / 2);
    }
    Log.infoln("[ServoManager] Ready – %d servo channels active", SERVO_COUNT);
}

void CServoManager::loop() {
    // Reserved for future motion sequencing
}

void CServoManager::setPulse(uint8_t channel, uint16_t pulse) {
    if (channel >= SERVO_COUNT) {
        Log.warningln("[ServoManager] setPulse: channel %d out of range (max %d)", channel, SERVO_COUNT - 1);
        return;
    }
    uint16_t lo   = configuration.eyeServoRangeMin[channel];
    uint16_t hi   = configuration.eyeServoRangeMax[channel];
    uint16_t trim = configuration.eyeServoTrim[channel];

    // Sanitise trim: must lie in [lo, hi]; fall back to natural midpoint if invalid
    if (trim < lo || trim > hi) trim = lo + (hi - lo) / 2;

    if (pulse < lo) pulse = lo;
    if (pulse > hi) pulse = hi;

    // Two-piece linear mapping so that the midpoint (lo+hi)/2 maps to trim.
    // This centres the servo at the trimmed position without clipping the full range.
    uint16_t mid = lo + (hi - lo) / 2;
    uint16_t out;
    if (hi == lo) {
        out = lo;
    } else if (pulse <= mid) {
        // Map [lo, mid] → [lo, trim]
        out = (mid > lo)
            ? (uint16_t)(lo + (uint32_t)(pulse - lo) * (trim - lo) / (mid - lo))
            : lo;
    } else {
        // Map (mid, hi] → (trim, hi]
        out = (hi > mid)
            ? (uint16_t)(trim + (uint32_t)(pulse - mid) * (hi - trim) / (hi - mid))
            : hi;
    }

    if (configuration.servoInvertedMask & (1u << channel)) {
        out = lo + hi - out;
    }

    pulses[channel] = out;
    pwm.setPWM(channel, 0, out);
    //Log.verboseln("[ServoManager] ch%d → pulse=%d (trim=%d)", channel, out, trim);
}

uint16_t CServoManager::getPulse(uint8_t channel) const {
    if (channel >= SERVO_COUNT) return SERVO_PULSE_MIN;
    return pulses[channel];
}

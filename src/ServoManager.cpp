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
        setPulse(ch, (configuration.eyeServoRangeMin[ch] + configuration.eyeServoRangeMax[ch]) / 2);
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
    uint16_t lo  = configuration.eyeServoRangeMin[channel];
    uint16_t hi  = configuration.eyeServoRangeMax[channel];

    if (pulse < lo) pulse = lo;
    if (pulse > hi) pulse = hi;

    uint16_t out = pulse;

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

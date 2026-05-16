#include "ServoManager.h"

#include <ArduinoLog.h>
#include <Wire.h>

CServoManager::CServoManager()
    : pwm(Adafruit_PWMServoDriver(0x40))
{
    memset(angles, 0, sizeof(angles));
}

CServoManager::~CServoManager() {}

void CServoManager::setup() {
    Log.infoln("[ServoManager] Initializing PWM servo driver at 0x40");
    pwm.begin();
    pwm.setOscillatorFrequency(27000000); // Calibrated 27 MHz internal oscillator
    pwm.setPWMFreq(50);                   // 50 Hz – standard for analog servos
    delay(10);

    // Drive all 6 channels to 0° on startup
    for (uint8_t ch = 0; ch < SERVO_COUNT; ch++) {
        setAngle(ch, 0);
    }
    Log.infoln("[ServoManager] Ready – %d servo channels active", SERVO_COUNT);
}

void CServoManager::loop() {
    // Reserved for future motion sequencing
}

void CServoManager::setAngle(uint8_t channel, uint8_t angle) {
    if (channel >= SERVO_COUNT) {
        Log.warningln("[ServoManager] setAngle: channel %d out of range (max %d)", channel, SERVO_COUNT - 1);
        return;
    }
    if (angle > 180) angle = 180;

    angles[channel] = angle;
    uint16_t pulse = angleToPulse(angle);
    pwm.setPWM(channel, 0, pulse);
    Log.verboseln("[ServoManager] ch%d → %d° (pulse=%d)", channel, angle, pulse);
}

uint8_t CServoManager::getAngle(uint8_t channel) const {
    if (channel >= SERVO_COUNT) return 0;
    return angles[channel];
}

uint16_t CServoManager::angleToPulse(uint8_t angle) const {
    // Linear interpolation from SERVO_PULSE_MIN (0°) to SERVO_PULSE_MAX (180°)
    return (uint16_t)(SERVO_PULSE_MIN + ((uint32_t)(SERVO_PULSE_MAX - SERVO_PULSE_MIN) * angle) / 180);
}

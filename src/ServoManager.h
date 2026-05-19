#pragma once

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVO_COUNT 6

class CServoManager {

public:
    CServoManager();
    ~CServoManager();

    void setup();
    void loop();

    // Set raw PWM pulse (SERVO_PULSE_MIN–SERVO_PULSE_MAX) for a single servo channel (0–5)
    void setPulse(uint8_t channel, uint16_t pulse);

    uint16_t getPulse(uint8_t channel) const;

private:
    Adafruit_PWMServoDriver pwm;
    uint16_t pulses[SERVO_COUNT];
};

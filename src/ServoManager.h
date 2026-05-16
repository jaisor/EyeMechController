#pragma once

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

#define SERVO_COUNT 6

// Standard servo pulse width bounds (microseconds → 12-bit ticks at 50 Hz)
// 50 Hz → 20 ms period → 4096 ticks; 1 tick ≈ 4.88 µs
#define SERVO_PULSE_MIN  150   // ~0°   (~730 µs)
#define SERVO_PULSE_MAX  600   // ~180° (~2930 µs)

class CServoManager {

public:
    CServoManager();
    ~CServoManager();

    void setup();
    void loop();

    // Set angle (0–180°) for a single servo channel (0–5)
    void setAngle(uint8_t channel, uint8_t angle);

    uint8_t getAngle(uint8_t channel) const;

private:
    uint16_t angleToPulse(uint8_t angle) const;

    Adafruit_PWMServoDriver pwm;
    uint8_t angles[SERVO_COUNT];
};

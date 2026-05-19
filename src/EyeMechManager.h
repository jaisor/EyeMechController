#pragma once

#include <Arduino.h>
#include "ServoManager.h"

// Servo channel assignments
// Right eye: channels 0–2
#define EYE_RIGHT_LR    0   // Right eye up / down
#define EYE_RIGHT_UD    1   // Right eye left / right
#define EYE_RIGHT_LID   2   // Right eyelid open / close

// Left eye: channels 3–5
#define EYE_LEFT_LR     3   // Left eye up / down
#define EYE_LEFT_UD     4   // Left eye left / right
#define EYE_LEFT_LID    5   // Left eyelid open / close

// Physical servo range for the eye mechanism (degrees)
#define EYE_SERVO_MIN       0
#define EYE_SERVO_MAX       55

// Eyelid servo range (degrees) – adjust independently of gaze servos
#define EYE_LID_SERVO_MAX   22

// Blink timing (ms): how long lids stay closed before re-opening
#define EYE_BLINK_CLOSE_MS  200

// Smooth movement: interval (ms) between each interpolation step
#define EYE_MOVE_STEP_INTERVAL_MS  10

class CEyeMechManager {

public:
    CEyeMechManager(CServoManager* servoManager);

    void setup();
    void loop();

    // Move both eyes to track a point on a 0–100 plane.
    //   x=0   → full left,  x=100 → full right
    //   y=0   → full down,  y=100 → full up
    void lookAt(float x, float y);

    // Control each eye independently (x/y in [0.0, 100.0]).
    void setRightEye(float x, float y);
    void setLeftEye(float x, float y);

    // Set eyelid openness for both eyes simultaneously.
    // openPercent: 0 = fully closed, 100 = fully open.
    void setEyelids(uint8_t openPercent);

    // Trigger a single non-blocking blink (ignored if a blink is in progress).
    void blink();

    // Set movement speed (1 = fastest / current behaviour, 8 = 1/8th speed).
    void setSpeed(uint8_t speed);

private:
    // Map a value in [0.0, 100.0] to the configured PWM pulse range for the given servo channel.
    uint16_t mapServo(float value, uint8_t channel) const;

    CServoManager* _servo;

    // Retained eyelid openness (0-100) so blink can restore the previous position.
    uint8_t _eyelidOpen;

    // Smooth movement: current and target gaze position in [0.0, 100.0]
    float         _currentX, _currentY;
    float         _targetX,  _targetY;
    uint8_t       _speed;          // 1 = fastest (instant), 8 = slowest (1/8th)
    unsigned long _moveStepMs;     // timestamp of last movement step

    // Non-blocking blink state machine
    enum BlinkState : uint8_t { BLINK_IDLE, BLINK_CLOSING };
    BlinkState      _blinkState;
    unsigned long   _blinkPhaseMs;
};

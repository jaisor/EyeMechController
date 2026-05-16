#include "EyeMechManager.h"

#include <ArduinoLog.h>

CEyeMechManager::CEyeMechManager(CServoManager* servoManager)
    : _servo(servoManager)
    , _eyelidOpen(100)
    , _currentX(50), _currentY(50)
    , _targetX(50),  _targetY(50)
    , _speed(1)
    , _moveStepMs(0)
    , _blinkState(BLINK_IDLE)
    , _blinkPhaseMs(0)
{}

void CEyeMechManager::setup() {
    Log.infoln("[EyeMechManager] Initializing – centering gaze and opening eyelids");
    // Drive servos directly so center position takes effect before the main loop runs
    _currentX = _targetX = 50;
    _currentY = _targetY = 50;
    setRightEye(50, 50);
    setLeftEye(50, 50);
    setEyelids(100);  // Fully open
}

void CEyeMechManager::loop() {
    // --- Blink state machine ---
    if (_blinkState == BLINK_CLOSING) {
        if (millis() - _blinkPhaseMs >= EYE_BLINK_CLOSE_MS) {
            uint8_t angle = mapToServo(_eyelidOpen);
            _servo->setAngle(EYE_RIGHT_LID, angle);
            _servo->setAngle(EYE_LEFT_LID, angle);
            _blinkState = BLINK_IDLE;
            Log.verboseln("[EyeMechManager] Blink complete");
        }
    }

    // --- Smooth movement toward target ---
    if (_currentX == _targetX && _currentY == _targetY) return;

    unsigned long now = millis();
    if (now - _moveStepMs < EYE_MOVE_STEP_INTERVAL_MS) return;
    _moveStepMs = now;

    // step size: speed 1 → 100 (instant jump), speed 8 → 12 (1/8th)
    uint8_t step = (uint8_t)(100 / _speed);
    if (step == 0) step = 1;

    if (_currentX < _targetX) {
        uint8_t d = _targetX - _currentX;
        _currentX += (step < d ? step : d);
    } else if (_currentX > _targetX) {
        uint8_t d = _currentX - _targetX;
        _currentX -= (step < d ? step : d);
    }

    if (_currentY < _targetY) {
        uint8_t d = _targetY - _currentY;
        _currentY += (step < d ? step : d);
    } else if (_currentY > _targetY) {
        uint8_t d = _currentY - _targetY;
        _currentY -= (step < d ? step : d);
    }

    setRightEye(_currentX, _currentY);
    setLeftEye(_currentX, _currentY);
}

// ---------------------------------------------------------------------------
// Gaze control
// ---------------------------------------------------------------------------

void CEyeMechManager::lookAt(uint8_t x, uint8_t y) {
    _targetX = x;
    _targetY = y;
}

void CEyeMechManager::setRightEye(uint8_t x, uint8_t y) {
    _servo->setAngle(EYE_RIGHT_LR, mapToServo(x));
    _servo->setAngle(EYE_RIGHT_UD, mapToServo(y));
}

void CEyeMechManager::setLeftEye(uint8_t x, uint8_t y) {
    _servo->setAngle(EYE_LEFT_LR, mapToServo(x));
    _servo->setAngle(EYE_LEFT_UD, mapToServo(y));
}

// ---------------------------------------------------------------------------
// Eyelid control
// ---------------------------------------------------------------------------

void CEyeMechManager::setEyelids(uint8_t openPercent) {
    if (openPercent > 100) openPercent = 100;
    _eyelidOpen = openPercent;
    uint8_t angle = mapToServo(openPercent);
    _servo->setAngle(EYE_RIGHT_LID, angle);
    _servo->setAngle(EYE_LEFT_LID, angle);
}

void CEyeMechManager::blink() {
    if (_blinkState != BLINK_IDLE) return; // Already blinking – ignore

    // Snap eyelids shut and start the timer
    _servo->setAngle(EYE_RIGHT_LID, EYE_SERVO_MIN);
    _servo->setAngle(EYE_LEFT_LID, EYE_SERVO_MIN);
    _blinkState   = BLINK_CLOSING;
    _blinkPhaseMs = millis();
    Log.verboseln("[EyeMechManager] Blink started");
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void CEyeMechManager::setSpeed(uint8_t speed) {
    if (speed < 1) speed = 1;
    if (speed > 8) speed = 8;
    _speed = speed;
}

uint8_t CEyeMechManager::mapToServo(uint8_t value) const {
    // Linear map: [0, 100] → [EYE_SERVO_MIN, EYE_SERVO_MAX]
    return (uint8_t)(EYE_SERVO_MIN +
        ((uint32_t)(EYE_SERVO_MAX - EYE_SERVO_MIN) * value) / 100);
}

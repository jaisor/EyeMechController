#include "EyeMechManager.h"

#include <ArduinoLog.h>
#include "Configuration.h"

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
            // Lids have been closed long enough – enter the hold pause
            _blinkState   = BLINK_CLOSED;
            _blinkPhaseMs = millis();
        }
    } else if (_blinkState == BLINK_CLOSED) {
        if (millis() - _blinkPhaseMs >= EYE_BLINK_HOLD_MS) {
            // Hold over – reopen
            _servo->setPulse(EYE_RIGHT_LID, mapServo(_eyelidOpen, EYE_RIGHT_LID));
            _servo->setPulse(EYE_LEFT_LID,  mapServo(_eyelidOpen, EYE_LEFT_LID));
            _blinkState = BLINK_IDLE;
            Log.verboseln("[EyeMechManager] Blink complete");
        }
    }

    // --- Smooth movement toward target ---
    if (_currentX == _targetX && _currentY == _targetY) return;

    unsigned long now = millis();
    if (now - _moveStepMs < EYE_MOVE_STEP_INTERVAL_MS) return;
    _moveStepMs = now;

    // step size: speed 1 → 100 (instant jump), speed 8 → 12.5 (1/8th)
    float step = 100.0f / _speed;

    if (_currentX < _targetX) {
        float d = _targetX - _currentX;
        _currentX += (step < d ? step : d);
    } else if (_currentX > _targetX) {
        float d = _currentX - _targetX;
        _currentX -= (step < d ? step : d);
    }

    if (_currentY < _targetY) {
        float d = _targetY - _currentY;
        _currentY += (step < d ? step : d);
    } else if (_currentY > _targetY) {
        float d = _currentY - _targetY;
        _currentY -= (step < d ? step : d);
    }

    setRightEye(_currentX, _currentY);
    setLeftEye(_currentX, _currentY);
}

// ---------------------------------------------------------------------------
// Gaze control
// ---------------------------------------------------------------------------

void CEyeMechManager::lookAt(float x, float y) {
    _targetX = constrain(x, 0.0f, 100.0f);
    _targetY = constrain(y, 0.0f, 100.0f);
}

void CEyeMechManager::setRightEye(float x, float y) {
    _servo->setPulse(EYE_RIGHT_LR, mapServo(x,           EYE_RIGHT_LR));  // x→LR
    _servo->setPulse(EYE_RIGHT_UD, mapServo(100.0f - y,  EYE_RIGHT_UD));  // y→UD (inverted)
}

void CEyeMechManager::setLeftEye(float x, float y) {
    _servo->setPulse(EYE_LEFT_LR, mapServo(x,           EYE_LEFT_LR));   // x→LR
    _servo->setPulse(EYE_LEFT_UD, mapServo(100.0f - y,  EYE_LEFT_UD));   // y→UD (inverted)
}

// ---------------------------------------------------------------------------
// Eyelid control
// ---------------------------------------------------------------------------

void CEyeMechManager::setEyelids(uint8_t openPercent) {
    if (openPercent > 100) openPercent = 100;
    _eyelidOpen = openPercent;
    _servo->setPulse(EYE_RIGHT_LID, mapServo(openPercent, EYE_RIGHT_LID));
    _servo->setPulse(EYE_LEFT_LID, mapServo(openPercent, EYE_LEFT_LID));
}

void CEyeMechManager::blink() {
    if (_blinkState != BLINK_IDLE) return; // Already blinking – ignore

    // Snap eyelids shut and start the timer
    _servo->setPulse(EYE_RIGHT_LID, configuration.eyeServoRangeMin[EYE_RIGHT_LID]);
    _servo->setPulse(EYE_LEFT_LID, configuration.eyeServoRangeMin[EYE_LEFT_LID]);
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

uint16_t CEyeMechManager::mapServo(float value, uint8_t channel) const {
    // Linear map: [0.0, 100.0] → [eyeServoRangeMin[ch], eyeServoRangeMax[ch]] (PWM pulse ticks)
    uint16_t lo = configuration.eyeServoRangeMin[channel];
    uint16_t hi = configuration.eyeServoRangeMax[channel];
    if (hi < lo) hi = lo;
    return (uint16_t)(lo + (float)(hi - lo) * value / 100.0f);
}

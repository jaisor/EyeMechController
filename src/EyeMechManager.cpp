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
    uint16_t lrPulse, udPulse;
    correctedGaze(x, y, lrPulse, udPulse);
    _servo->setPulse(EYE_RIGHT_LR, lrPulse);
    _servo->setPulse(EYE_RIGHT_UD, udPulse);
}

void CEyeMechManager::setLeftEye(float x, float y) {
    uint16_t lrPulse, udPulse;
    correctedGazeLeft(x, y, lrPulse, udPulse);
    _servo->setPulse(EYE_LEFT_LR, lrPulse);
    _servo->setPulse(EYE_LEFT_UD, udPulse);
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

void CEyeMechManager::correctedGaze(float x, float y, uint16_t& lrPulse, uint16_t& udPulse) const {
    // Map x [0,100] → col [0,2], y [100→0] → row [0,2] (row 0 = top = y=100)
    float col_f = x / 50.0f;
    float row_f = (100.0f - y) / 50.0f;
    if (col_f < 0.0f) col_f = 0.0f; else if (col_f > 2.0f) col_f = 2.0f;
    if (row_f < 0.0f) row_f = 0.0f; else if (row_f > 2.0f) row_f = 2.0f;

    uint8_t col0 = (uint8_t)col_f; if (col0 > 1) col0 = 1;
    uint8_t row0 = (uint8_t)row_f; if (row0 > 1) row0 = 1;
    uint8_t col1 = col0 + 1;
    uint8_t row1 = row0 + 1;
    float tx = col_f - (float)col0;
    float ty = row_f - (float)row0;

    for (uint8_t axis = 0; axis < 2; axis++) {
        float v00 = configuration.eyeCorrMatrixR[row0 * 3 + col0][axis];
        float v10 = configuration.eyeCorrMatrixR[row0 * 3 + col1][axis];
        float v01 = configuration.eyeCorrMatrixR[row1 * 3 + col0][axis];
        float v11 = configuration.eyeCorrMatrixR[row1 * 3 + col1][axis];
        float result = (1.0f - tx) * (1.0f - ty) * v00
                     +         tx  * (1.0f - ty) * v10
                     + (1.0f - tx) *          ty  * v01
                     +         tx  *          ty  * v11;
        if (axis == 0) lrPulse = (uint16_t)result;
        else           udPulse = (uint16_t)result;
    }
}

void CEyeMechManager::correctedGazeLeft(float x, float y, uint16_t& lrPulse, uint16_t& udPulse) const {
    float col_f = x / 50.0f;
    float row_f = (100.0f - y) / 50.0f;
    if (col_f < 0.0f) col_f = 0.0f; else if (col_f > 2.0f) col_f = 2.0f;
    if (row_f < 0.0f) row_f = 0.0f; else if (row_f > 2.0f) row_f = 2.0f;

    uint8_t col0 = (uint8_t)col_f; if (col0 > 1) col0 = 1;
    uint8_t row0 = (uint8_t)row_f; if (row0 > 1) row0 = 1;
    uint8_t col1 = col0 + 1;
    uint8_t row1 = row0 + 1;
    float tx = col_f - (float)col0;
    float ty = row_f - (float)row0;

    for (uint8_t axis = 0; axis < 2; axis++) {
        float v00 = configuration.eyeCorrMatrixL[row0 * 3 + col0][axis];
        float v10 = configuration.eyeCorrMatrixL[row0 * 3 + col1][axis];
        float v01 = configuration.eyeCorrMatrixL[row1 * 3 + col0][axis];
        float v11 = configuration.eyeCorrMatrixL[row1 * 3 + col1][axis];
        float result = (1.0f - tx) * (1.0f - ty) * v00
                     +         tx  * (1.0f - ty) * v10
                     + (1.0f - tx) *          ty  * v01
                     +         tx  *          ty  * v11;
        if (axis == 0) lrPulse = (uint16_t)result;
        else           udPulse = (uint16_t)result;
    }
}

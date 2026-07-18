#include "safety/SafetyController.h"

void SafetyController::begin(uint8_t estopPin, bool enabled) {
    estopPin_ = estopPin;
    enabled_ = enabled;
    estopActive_ = false;
    lastFault_ = FaultCode::None;

    if (enabled_) {
        pinMode(estopPin_, INPUT_PULLUP);
    }
}

void SafetyController::update() {
    if (!enabled_) {
        return;
    }

    // Active-low emergency stop with internal pull-up.
    const bool active = digitalRead(estopPin_) == LOW;
    if (active && !estopActive_) {
        estopActive_ = true;
        lastFault_ = FaultCode::EmergencyStop;
    } else if (!active) {
        estopActive_ = false;
    }
}

void SafetyController::raiseFault(FaultCode code) {
    lastFault_ = code;
}

bool SafetyController::acknowledgeFault() {
    if (enabled_ && estopActive_) {
        return false;
    }
    lastFault_ = FaultCode::None;
    return true;
}

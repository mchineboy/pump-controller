#include "safety/SafetyController.h"

void SafetyController::begin(uint8_t estopPin, bool enabled) {
    estopPin_ = estopPin;
    enabled_ = enabled;
    estopActive_ = false;
    wasActive_ = false;
    lastFault_ = FaultCode::None;

    if (enabled_) {
        pinMode(estopPin_, INPUT_PULLUP);
        // Active-low: closed switch to GND asserts ESTOP.
        estopActive_ = digitalRead(estopPin_) == LOW;
        wasActive_ = estopActive_;
        if (estopActive_) {
            lastFault_ = FaultCode::EmergencyStop;
        }
    }
}

void SafetyController::setEnabled(bool enabled) {
    begin(estopPin_, enabled);
}

void SafetyController::update() {
    if (!enabled_) {
        estopActive_ = false;
        wasActive_ = false;
        return;
    }

    // Active-low emergency stop with internal pull-up.
    // Firmware loop path — independent of the web server.
    const bool active = digitalRead(estopPin_) == LOW;
    if (active) {
        estopActive_ = true;
        if (!wasActive_) {
            lastFault_ = FaultCode::EmergencyStop;
        }
    } else {
        estopActive_ = false;
    }
    wasActive_ = active;
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

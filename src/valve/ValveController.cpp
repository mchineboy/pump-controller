#include "valve/ValveController.h"

void ValveController::begin(uint8_t pin, bool available, bool activeHigh) {
    pin_ = pin;
    available_ = available;
    activeHigh_ = activeHigh;
    open_ = false;

    if (!available_) {
        return;
    }

    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, activeHigh_ ? LOW : HIGH);
}

bool ValveController::open() {
    if (!available_) {
        return true;  // No-op success when valve hardware is absent.
    }
    digitalWrite(pin_, activeHigh_ ? HIGH : LOW);
    open_ = true;
    return true;
}

bool ValveController::close() {
    if (!available_) {
        open_ = false;
        return true;
    }
    digitalWrite(pin_, activeHigh_ ? LOW : HIGH);
    open_ = false;
    return true;
}

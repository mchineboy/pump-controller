#include "sensors/ReservoirSensor.h"

void ReservoirSensor::begin(uint8_t pin, bool enabled, bool emptyActiveLow) {
    pin_ = pin;
    enabled_ = enabled;
    emptyActiveLow_ = emptyActiveLow;
    empty_ = false;

    if (!enabled_ || pin_ == 255) {
        return;
    }

    // GPIO 34–39 are input-only and lack internal pull-ups; use external bias.
    pinMode(pin_, INPUT);
    update();
}

void ReservoirSensor::configure(bool enabled, bool emptyActiveLow) {
    begin(pin_, enabled, emptyActiveLow);
}

void ReservoirSensor::update() {
    if (!enabled_ || pin_ == 255) {
        empty_ = false;
        return;
    }

    const int level = digitalRead(pin_);
    empty_ = emptyActiveLow_ ? (level == LOW) : (level == HIGH);
}

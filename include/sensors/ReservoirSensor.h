#pragma once

#include <Arduino.h>

class ReservoirSensor {
public:
    void begin(uint8_t pin, bool enabled, bool emptyActiveLow);
    void configure(bool enabled, bool emptyActiveLow);
    void update();

    bool isEnabled() const { return enabled_; }
    bool isEmpty() const { return enabled_ && empty_; }
    bool emptyActiveLow() const { return emptyActiveLow_; }

private:
    uint8_t pin_ = 255;
    bool enabled_ = false;
    bool emptyActiveLow_ = true;
    bool empty_ = false;
};

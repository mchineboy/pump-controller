#pragma once

#include <Arduino.h>

class ValveController {
public:
    void begin(uint8_t pin, bool available, bool activeHigh = true);

    bool open();
    bool close();
    bool isOpen() const { return open_; }
    bool isAvailable() const { return available_; }

private:
    uint8_t pin_ = 255;
    bool available_ = false;
    bool activeHigh_ = true;
    bool open_ = false;
};

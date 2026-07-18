#pragma once

#include <Arduino.h>

class TemperatureSensor {
public:
    bool begin(uint8_t pin);
    void configure(bool enabled, float warnLowC, float warnHighC);
    void update();

    bool isEnabled() const { return enabled_; }
    bool isReady() const { return ready_; }
    float celsius() const { return celsius_; }
    bool warnLow() const { return warnLow_; }
    bool warnHigh() const { return warnHigh_; }
    float warnLowC() const { return warnLowC_; }
    float warnHighC() const { return warnHighC_; }
    const String& lastError() const { return lastError_; }

private:
    bool readCelsius(float& outC);

    uint8_t pin_ = 255;
    bool enabled_ = false;
    bool ready_ = false;
    float celsius_ = NAN;
    float warnLowC_ = 0.0f;
    float warnHighC_ = 100.0f;
    bool warnLow_ = false;
    bool warnHigh_ = false;
    String lastError_;
    uint32_t lastSampleMs_ = 0;
    uint32_t conversionStartedMs_ = 0;
    bool conversionPending_ = false;
};

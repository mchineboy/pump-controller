#pragma once

#include <Arduino.h>

class FlowSensor {
public:
    bool begin(uint8_t pin);
    void configure(bool enabled, float pulsesPerLiter);
    void update();
    void resetCumulative();

    bool isEnabled() const { return enabled_; }
    bool isReady() const { return ready_; }
    float pulsesPerLiter() const { return pulsesPerLiter_; }
    float flowMlPerMin() const { return flowMlPerMin_; }
    float cumulativeMl() const { return cumulativeMl_; }
    uint32_t pulseCount() const { return pulseCount_; }
    const String& lastError() const { return lastError_; }

    // Called from ISR.
    static void IRAM_ATTR onPulseIsr();

private:
    static FlowSensor* active_;

    uint8_t pin_ = 255;
    bool enabled_ = false;
    bool ready_ = false;
    float pulsesPerLiter_ = 450.0f;
    volatile uint32_t pulseCount_ = 0;
    uint32_t lastPulseCount_ = 0;
    uint32_t lastRateMs_ = 0;
    float flowMlPerMin_ = 0.0f;
    float cumulativeMl_ = 0.0f;
    String lastError_;
};

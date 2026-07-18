#include "sensors/FlowSensor.h"

#include <cmath>

namespace {
constexpr uint32_t kRateWindowMs = 1000;
}

FlowSensor* FlowSensor::active_ = nullptr;

void IRAM_ATTR FlowSensor::onPulseIsr() {
    if (active_ != nullptr) {
        active_->pulseCount_++;
    }
}

bool FlowSensor::begin(uint8_t pin) {
    if (active_ == this) {
        detachInterrupt(digitalPinToInterrupt(pin_));
        active_ = nullptr;
    }
    pin_ = pin;
    ready_ = false;
    lastError_ = "";
    pulseCount_ = 0;
    lastPulseCount_ = 0;
    flowMlPerMin_ = 0.0f;
    cumulativeMl_ = 0.0f;
    if (pin_ == 255) {
        lastError_ = "flow_pin_invalid";
        return false;
    }
    pinMode(pin_, INPUT_PULLUP);
    return true;
}

void FlowSensor::configure(bool enabled, float pulsesPerLiter) {
    enabled_ = enabled;
    pulsesPerLiter_ =
        (pulsesPerLiter > 0.0f && std::isfinite(pulsesPerLiter)) ? pulsesPerLiter
                                                                  : 450.0f;
    ready_ = false;
    lastError_ = "";
    flowMlPerMin_ = 0.0f;

    if (active_ == this) {
        detachInterrupt(digitalPinToInterrupt(pin_));
        active_ = nullptr;
    }

    if (!enabled_) {
        return;
    }
    if (pin_ == 255) {
        lastError_ = "flow_pin_invalid";
        return;
    }

    pulseCount_ = 0;
    lastPulseCount_ = 0;
    lastRateMs_ = millis();
    cumulativeMl_ = 0.0f;
    active_ = this;
    attachInterrupt(digitalPinToInterrupt(pin_), onPulseIsr, FALLING);
    ready_ = true;
}

void FlowSensor::update() {
    if (!enabled_ || !ready_) {
        return;
    }

    const uint32_t now = millis();
    const uint32_t elapsed = now - lastRateMs_;
    if (elapsed < kRateWindowMs) {
        return;
    }

    noInterrupts();
    const uint32_t count = pulseCount_;
    interrupts();

    const uint32_t delta = count - lastPulseCount_;
    lastPulseCount_ = count;
    lastRateMs_ = now;

    const float liters = static_cast<float>(delta) / pulsesPerLiter_;
    const float ml = liters * 1000.0f;
    cumulativeMl_ += ml;
    const float minutes = static_cast<float>(elapsed) / 60000.0f;
    flowMlPerMin_ = minutes > 0.0f ? (ml / minutes) : 0.0f;
}

void FlowSensor::resetCumulative() {
    noInterrupts();
    pulseCount_ = 0;
    interrupts();
    lastPulseCount_ = 0;
    cumulativeMl_ = 0.0f;
    flowMlPerMin_ = 0.0f;
    lastRateMs_ = millis();
}

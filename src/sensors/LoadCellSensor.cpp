#include "sensors/LoadCellSensor.h"

#include <cmath>

namespace {
constexpr uint32_t kSampleIntervalMs = 200;
constexpr uint32_t kReadTimeoutMs = 100;
}  // namespace

bool LoadCellSensor::begin(uint8_t dataPin, uint8_t clockPin) {
    dataPin_ = dataPin;
    clockPin_ = clockPin;
    ready_ = false;
    lastError_ = "";
    raw_ = 0;
    grams_ = 0.0f;

    if (dataPin_ == 255 || clockPin_ == 255) {
        lastError_ = "loadcell_pins_invalid";
        return false;
    }

    pinMode(dataPin_, INPUT);
    pinMode(clockPin_, OUTPUT);
    digitalWrite(clockPin_, LOW);
    return true;
}

void LoadCellSensor::configure(bool enabled, float scale, int32_t offset) {
    enabled_ = enabled;
    scale_ = (scale == 0.0f) ? 1.0f : scale;
    offset_ = offset;
    ready_ = false;
    lastError_ = "";

    if (!enabled_) {
        grams_ = 0.0f;
        raw_ = 0;
        return;
    }

    int32_t sample = 0;
    if (!averageRaw(4, sample)) {
        lastError_ = "loadcell_no_response";
        ready_ = false;
        return;
    }

    ready_ = true;
    raw_ = sample;
    grams_ = static_cast<float>(sample - offset_) / scale_;
}

bool LoadCellSensor::readRaw(int32_t& value) {
    const uint32_t started = millis();
    while (digitalRead(dataPin_) == HIGH) {
        if (millis() - started > kReadTimeoutMs) {
            return false;
        }
        delay(1);
    }

    uint32_t raw = 0;
    for (uint8_t i = 0; i < 24; ++i) {
        digitalWrite(clockPin_, HIGH);
        delayMicroseconds(1);
        raw = (raw << 1) | (digitalRead(dataPin_) ? 1u : 0u);
        digitalWrite(clockPin_, LOW);
        delayMicroseconds(1);
    }

    // Channel A, gain 128 — one extra clock pulse.
    digitalWrite(clockPin_, HIGH);
    delayMicroseconds(1);
    digitalWrite(clockPin_, LOW);
    delayMicroseconds(1);

    if (raw & 0x800000u) {
        raw |= 0xFF000000u;
    }
    value = static_cast<int32_t>(raw);
    return true;
}

bool LoadCellSensor::averageRaw(uint8_t samples, int32_t& average) {
    if (samples == 0) {
        return false;
    }
    int64_t sum = 0;
    for (uint8_t i = 0; i < samples; ++i) {
        int32_t sample = 0;
        if (!readRaw(sample)) {
            return false;
        }
        sum += sample;
    }
    average = static_cast<int32_t>(sum / samples);
    return true;
}

void LoadCellSensor::update() {
    if (!enabled_) {
        return;
    }
    const uint32_t now = millis();
    if (now - lastSampleMs_ < kSampleIntervalMs) {
        return;
    }
    lastSampleMs_ = now;

    int32_t sample = 0;
    if (!readRaw(sample)) {
        ready_ = false;
        lastError_ = "loadcell_no_response";
        return;
    }

    ready_ = true;
    lastError_ = "";
    raw_ = sample;
    grams_ = static_cast<float>(sample - offset_) / scale_;
}

bool LoadCellSensor::tare(uint8_t samples) {
    if (!enabled_) {
        lastError_ = "loadcell_disabled";
        return false;
    }
    int32_t average = 0;
    if (!averageRaw(samples, average)) {
        lastError_ = "loadcell_no_response";
        ready_ = false;
        return false;
    }
    offset_ = average;
    raw_ = average;
    grams_ = 0.0f;
    ready_ = true;
    lastError_ = "";
    return true;
}

bool LoadCellSensor::calibrate(float knownGrams, uint8_t samples) {
    if (!enabled_) {
        lastError_ = "loadcell_disabled";
        return false;
    }
    if (!std::isfinite(knownGrams) || knownGrams <= 0.0f) {
        lastError_ = "loadcell_invalid_mass";
        return false;
    }
    int32_t average = 0;
    if (!averageRaw(samples, average)) {
        lastError_ = "loadcell_no_response";
        ready_ = false;
        return false;
    }
    const float delta = static_cast<float>(average - offset_);
    if (fabsf(delta) < 1.0f) {
        lastError_ = "loadcell_no_delta";
        return false;
    }
    scale_ = delta / knownGrams;
    raw_ = average;
    grams_ = knownGrams;
    ready_ = true;
    lastError_ = "";
    return true;
}

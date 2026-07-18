#pragma once

#include <Arduino.h>

class LoadCellSensor {
public:
    bool begin(uint8_t dataPin, uint8_t clockPin);
    void configure(bool enabled, float scale, int32_t offset);
    void update();

    bool tare(uint8_t samples = 16);
    bool calibrate(float knownGrams, uint8_t samples = 16);

    bool isEnabled() const { return enabled_; }
    bool isReady() const { return ready_; }
    float scale() const { return scale_; }
    int32_t offset() const { return offset_; }
    float grams() const { return grams_; }
    int32_t raw() const { return raw_; }
    const String& lastError() const { return lastError_; }

private:
    bool readRaw(int32_t& value);
    bool averageRaw(uint8_t samples, int32_t& average);

    uint8_t dataPin_ = 255;
    uint8_t clockPin_ = 255;
    bool enabled_ = false;
    bool ready_ = false;
    float scale_ = 1.0f;
    int32_t offset_ = 0;
    int32_t raw_ = 0;
    float grams_ = 0.0f;
    String lastError_;
    uint32_t lastSampleMs_ = 0;
};

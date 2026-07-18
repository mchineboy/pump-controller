#include "sensors/TemperatureSensor.h"

#include <DallasTemperature.h>
#include <OneWire.h>
#include <cmath>
#include <memory>

namespace {
constexpr uint32_t kSampleIntervalMs = 2000;
constexpr uint32_t kConversionWaitMs = 800;
std::unique_ptr<OneWire> gOneWire;
std::unique_ptr<DallasTemperature> gSensors;
}  // namespace

bool TemperatureSensor::begin(uint8_t pin) {
    pin_ = pin;
    ready_ = false;
    lastError_ = "";
    celsius_ = NAN;
    conversionPending_ = false;
    gOneWire.reset();
    gSensors.reset();

    if (pin_ == 255) {
        lastError_ = "temp_pin_invalid";
        return false;
    }

    gOneWire.reset(new OneWire(pin_));
    gSensors.reset(new DallasTemperature(gOneWire.get()));
    gSensors->begin();
    return true;
}

void TemperatureSensor::configure(bool enabled, float warnLowC, float warnHighC) {
    enabled_ = enabled;
    warnLowC_ = warnLowC;
    warnHighC_ = warnHighC;
    warnLow_ = false;
    warnHigh_ = false;
    ready_ = false;
    lastError_ = "";
    celsius_ = NAN;
    conversionPending_ = false;

    if (!enabled_) {
        return;
    }
    if (gSensors == nullptr) {
        lastError_ = "temp_not_initialized";
        return;
    }

    gSensors->begin();
    const uint8_t count = gSensors->getDeviceCount();
    if (count == 0) {
        lastError_ = "temp_no_device";
        ready_ = false;
        return;
    }

    gSensors->setWaitForConversion(false);
    gSensors->setResolution(12);
    ready_ = true;
    lastError_ = "";
}

bool TemperatureSensor::readCelsius(float& outC) {
    if (gSensors == nullptr) {
        return false;
    }
    const float value = gSensors->getTempCByIndex(0);
    if (value == DEVICE_DISCONNECTED_C || !std::isfinite(value)) {
        return false;
    }
    outC = value;
    return true;
}

void TemperatureSensor::update() {
    if (!enabled_ || gSensors == nullptr) {
        return;
    }

    const uint32_t now = millis();

    if (conversionPending_) {
        if (now - conversionStartedMs_ < kConversionWaitMs) {
            return;
        }
        float value = NAN;
        if (!readCelsius(value)) {
            ready_ = false;
            lastError_ = "temp_read_failed";
            conversionPending_ = false;
            return;
        }
        celsius_ = value;
        ready_ = true;
        lastError_ = "";
        warnLow_ = std::isfinite(warnLowC_) && value < warnLowC_;
        warnHigh_ = std::isfinite(warnHighC_) && value > warnHighC_;
        conversionPending_ = false;
        lastSampleMs_ = now;
        return;
    }

    if (now - lastSampleMs_ < kSampleIntervalMs) {
        return;
    }

    gSensors->requestTemperatures();
    conversionStartedMs_ = now;
    conversionPending_ = true;
}

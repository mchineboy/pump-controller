#pragma once

#include <Arduino.h>

struct TmcDriverConfig {
    bool enabled = false;
    uint16_t runCurrentMa = 800;
    uint16_t holdCurrentMa = 400;
    uint16_t microsteps = 16;
    bool stealthChop = true;
};

class TmcDriverController {
public:
    bool begin(uint8_t rxPin, uint8_t txPin);
    bool apply(const TmcDriverConfig& config);
    bool refreshDiagnostics();

    bool isReady() const { return ready_; }
    bool isEnabled() const { return config_.enabled; }
    const TmcDriverConfig& config() const { return config_; }

    bool overtemperature() const { return overtemperature_; }
    bool shortCircuit() const { return shortCircuit_; }
    bool openLoad() const { return openLoad_; }
    uint32_t lastStatus() const { return lastStatus_; }
    const String& lastError() const { return lastError_; }

private:
    bool ready_ = false;
    TmcDriverConfig config_;
    String lastError_;
    uint32_t lastStatus_ = 0;
    bool overtemperature_ = false;
    bool shortCircuit_ = false;
    bool openLoad_ = false;
    uint8_t rxPin_ = 16;
    uint8_t txPin_ = 17;
};

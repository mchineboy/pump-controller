#pragma once

#include <Arduino.h>

struct GlobalSettings {
    String deviceName = "Fluid Dispensing Controller";
    String hostname = "pump-controller";
    String wifiMode = "ap";  // ap | station
    bool emergencyStopEnabled = false;
    bool driverUartEnabled = false;
    uint16_t driverRunCurrentMa = 800;
    uint16_t driverHoldCurrentMa = 400;
    uint16_t driverMicrosteps = 16;
    bool driverStealthChop = true;
    bool loggingEnabled = true;
    bool webAuthEnabled = false;
    bool valveHardwarePresent = false;
};

class SettingsRepository {
public:
    bool begin();
    GlobalSettings get() const { return settings_; }
    bool save(const GlobalSettings& settings);

private:
    GlobalSettings settings_;
};

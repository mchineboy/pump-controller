#pragma once

#include <Arduino.h>

struct GlobalSettings {
    String deviceName = "Precision Pump Controller";
    String hostname = "pump-controller";
    String wifiMode = "ap";  // ap | station
    bool emergencyStopEnabled = false;
    bool driverUartEnabled = false;
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

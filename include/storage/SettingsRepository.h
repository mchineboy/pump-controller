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
    bool reservoirSensorEnabled = false;
    bool reservoirEmptyActiveLow = true;
    // warn | block | fault
    String reservoirEmptyPolicy = "block";
    bool loadCellEnabled = false;
    float loadCellScale = 1.0f;
    int32_t loadCellOffset = 0;
    float fluidDensityGPerMl = 1.0f;
    bool temperatureSensorEnabled = false;
    float temperatureWarnLowC = 5.0f;
    float temperatureWarnHighC = 40.0f;
    bool flowSensorEnabled = false;
    float flowPulsesPerLiter = 450.0f;
    // open_loop | verify_after | stop_on_feedback
    String dispenseFeedbackMode = "open_loop";
    // auto | load_cell | flow
    String dispenseFeedbackSource = "auto";
    float dispenseFeedbackTolerancePercent = 5.0f;
    // warn | fault
    String dispenseFeedbackOnMiss = "warn";
};

class SettingsRepository {
public:
    bool begin();
    GlobalSettings get() const { return settings_; }
    bool save(const GlobalSettings& settings);

private:
    GlobalSettings settings_;
};

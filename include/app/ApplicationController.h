#pragma once

#include "diagnostics/EventLogger.h"
#include "motor/StepperController.h"
#include "motor/TmcDriverController.h"
#include "pump/PumpService.h"
#include "safety/SafetyController.h"
#include "sensors/LoadCellSensor.h"
#include "sensors/ReservoirSensor.h"
#include "storage/ProfileRepository.h"
#include "storage/SettingsRepository.h"
#include "valve/ValveController.h"
#include "web/WebServerController.h"

class ApplicationController {
public:
    void begin();
    void loop();

private:
    void beginSafeOutputs();
    void beginNetwork();
    void applyTmcSettings(const GlobalSettings& settings);
    void applyReservoirSettings(const GlobalSettings& settings);
    void applyLoadCellSettings(const GlobalSettings& settings);

    StepperController stepper_;
    TmcDriverController tmc_;
    ValveController valve_;
    ReservoirSensor reservoir_;
    LoadCellSensor loadCell_;
    ProfileRepository profiles_;
    SettingsRepository settings_;
    SafetyController safety_;
    EventLogger logger_;
    PumpService pump_;
    WebServerController web_;
};

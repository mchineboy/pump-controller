#pragma once

#include "diagnostics/EventLogger.h"
#include "motor/StepperController.h"
#include "motor/TmcDriverController.h"
#include "pump/PumpService.h"
#include "safety/SafetyController.h"
#include "sensors/FlowSensor.h"
#include "sensors/LoadCellSensor.h"
#include "sensors/ReservoirSensor.h"
#include "sensors/TemperatureSensor.h"
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
    void applyTemperatureSettings(const GlobalSettings& settings);
    void applyFlowSettings(const GlobalSettings& settings);
    void applyFeedbackSettings(const GlobalSettings& settings);

    StepperController stepper_;
    StepperController stepper2_;
    StepperController stepper3_;
    TmcDriverController tmc_;
    ValveController valve_;
    ValveController valve2_;
    ValveController valve3_;
    ReservoirSensor reservoir_;
    LoadCellSensor loadCell_;
    TemperatureSensor temperature_;
    FlowSensor flow_;
    ProfileRepository profiles_;
    SettingsRepository settings_;
    SafetyController safety_;
    EventLogger logger_;
    PumpService pump_;
    WebServerController web_;
    bool temperatureWarned_ = false;
};

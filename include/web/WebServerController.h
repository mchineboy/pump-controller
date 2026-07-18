#pragma once

#include <ESPAsyncWebServer.h>

#include "diagnostics/EventLogger.h"
#include "motor/StepperController.h"
#include "motor/TmcDriverController.h"
#include "pump/PumpService.h"
#include "safety/SafetyController.h"
#include "storage/ProfileRepository.h"
#include "storage/SettingsRepository.h"
#include "valve/ValveController.h"

class WebServerController {
public:
    void begin(
        PumpService& pump,
        ProfileRepository& profiles,
        SettingsRepository& settings,
        StepperController& stepper,
        ValveController& valve,
        SafetyController& safety,
        EventLogger& logger,
        TmcDriverController& tmc
    );

    void update();

private:
    void registerStaticRoutes();
    void registerApiRoutes();
    bool requireAuth(AsyncWebServerRequest* request) const;
    void fillProfileJson(const FluidProfile& profile, JsonObject doc) const;
    void fillTmcSettingsJson(const GlobalSettings& settings, JsonObject doc) const;
    bool applyTmcFromBody(JsonObjectConst body, GlobalSettings& settings) const;

    AsyncWebServer server_{80};
    AsyncEventSource events_{"/api/events"};

    PumpService* pump_ = nullptr;
    ProfileRepository* profiles_ = nullptr;
    SettingsRepository* settings_ = nullptr;
    StepperController* stepper_ = nullptr;
    ValveController* valve_ = nullptr;
    SafetyController* safety_ = nullptr;
    EventLogger* logger_ = nullptr;
    TmcDriverController* tmc_ = nullptr;

    SystemState lastBroadcastState_ = SystemState::Booting;
    uint32_t lastProgressBroadcastMs_ = 0;
};

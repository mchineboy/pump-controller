#include "web/WebServerController.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "Config.h"
#include "secrets.h"

namespace {

void sendJson(AsyncWebServerRequest* request, int status, const JsonDocument& doc) {
    String payload;
    serializeJson(doc, payload);
    request->send(status, "application/json", payload);
}

void sendError(AsyncWebServerRequest* request, int status, const char* message) {
    JsonDocument doc;
    doc["error"] = message;
    sendJson(request, status, doc);
}

bool readJsonBody(uint8_t* data, size_t len, JsonDocument& doc) {
    if (data == nullptr || len == 0) {
        return false;
    }
    return deserializeJson(doc, data, len) == DeserializationError::Ok;
}

bool clampMotorSettings(MotorSettings& motor) {
    if (motor.speedStepsPerSecond < Config::kMinSpeedStepsPerSec ||
        motor.speedStepsPerSecond > Config::kMaxSpeedStepsPerSec) {
        return false;
    }
    if (motor.accelerationStepsPerSecondSquared < Config::kMinAccelStepsPerSec2 ||
        motor.accelerationStepsPerSecondSquared > Config::kMaxAccelStepsPerSec2) {
        return false;
    }
    if (motor.decelerationStepsPerSecondSquared < Config::kMinAccelStepsPerSec2 ||
        motor.decelerationStepsPerSecondSquared > Config::kMaxAccelStepsPerSec2) {
        return false;
    }
    return true;
}

bool clampValveSettings(const ValveSettings& valve) {
    if (valve.preOpenMs > Config::kMaxValveTimingMs ||
        valve.postMotorCloseMs > Config::kMaxValveTimingMs) {
        return false;
    }
    if (valve.antiDripReverseSteps < 0 || valve.antiDripReverseSteps > 5000) {
        return false;
    }
    if (valve.antiDripEnabled &&
        (valve.antiDripSpeedStepsPerSecond < Config::kMinSpeedStepsPerSec ||
         valve.antiDripSpeedStepsPerSecond > Config::kMaxSpeedStepsPerSec)) {
        return false;
    }
    return true;
}

}  // namespace

bool WebServerController::requireAuth(AsyncWebServerRequest* request) const {
    if (settings_ == nullptr || !settings_->get().webAuthEnabled) {
        return true;
    }
    if (request->authenticate(Secrets::kWebUsername, Secrets::kWebPassword)) {
        return true;
    }
    request->requestAuthentication("PumpController");
    return false;
}

void WebServerController::fillProfileJson(
    const FluidProfile& profile,
    JsonObject doc
) const {
    doc["id"] = profile.id;
    doc["name"] = profile.name;
    doc["enabled"] = profile.enabled;
    doc["calibrated"] = profile.calibrated;

    JsonObject cal = doc["calibration"].to<JsonObject>();
    cal["method"] = profile.calibration.method;
    cal["requested_duration_ms"] = profile.calibration.requestedDurationMs;
    cal["actual_duration_ms"] = profile.calibration.actualDurationMs;
    cal["step_count"] = profile.calibration.stepCount;
    cal["measured_ml"] = profile.calibration.measuredMl;
    cal["steps_per_ml"] = profile.calibration.stepsPerMl;
    cal["ml_per_second"] = profile.calibration.mlPerSecond;
    cal["sample_count"] = profile.calibration.sampleCount;
    cal["calibrated_at"] = profile.calibration.calibratedAt;

    JsonObject motor = doc["motor"].to<JsonObject>();
    motor["speed_steps_per_second"] = profile.motor.speedStepsPerSecond;
    motor["acceleration_steps_per_second_squared"] =
        profile.motor.accelerationStepsPerSecondSquared;
    motor["deceleration_steps_per_second_squared"] =
        profile.motor.decelerationStepsPerSecondSquared;
    motor["direction_inverted"] = profile.motor.directionInverted;
    motor["microsteps"] = profile.motor.microsteps;

    JsonObject valve = doc["valve"].to<JsonObject>();
    valve["enabled"] = profile.valve.enabled;
    valve["active_high"] = profile.valve.activeHigh;
    valve["pre_open_ms"] = profile.valve.preOpenMs;
    valve["post_motor_close_ms"] = profile.valve.postMotorCloseMs;
    valve["anti_drip_enabled"] = profile.valve.antiDripEnabled;
    valve["anti_drip_reverse_steps"] = profile.valve.antiDripReverseSteps;
    valve["anti_drip_speed_steps_per_second"] =
        profile.valve.antiDripSpeedStepsPerSecond;

    JsonObject limits = doc["limits"].to<JsonObject>();
    limits["minimum_ml"] = profile.limits.minimumMl;
    limits["maximum_ml"] = profile.limits.maximumMl;
}

void WebServerController::begin(
    PumpService& pump,
    ProfileRepository& profiles,
    SettingsRepository& settings,
    StepperController& stepper,
    ValveController& valve,
    SafetyController& safety,
    EventLogger& logger
) {
    pump_ = &pump;
    profiles_ = &profiles;
    settings_ = &settings;
    stepper_ = &stepper;
    valve_ = &valve;
    safety_ = &safety;
    logger_ = &logger;

    registerApiRoutes();
    registerStaticRoutes();

    events_.onConnect([this](AsyncEventSourceClient* client) {
        if (client == nullptr || pump_ == nullptr) {
            return;
        }
        JsonDocument doc;
        const OperationStatus progress = pump_->getProgress();
        doc["state"] = systemStateToString(progress.state);
        doc["operation_id"] = progress.operationId;
        String payload;
        serializeJson(doc, payload);
        client->send(payload.c_str(), "status", millis());
    });

    server_.addHandler(&events_);
    server_.begin();
}

void WebServerController::registerStaticRoutes() {
    server_.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl("no-cache");

    server_.onNotFound([this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        if (request->url().startsWith("/api/")) {
            sendError(request, 404, "not_found");
            return;
        }
        request->send(LittleFS, "/index.html", "text/html");
    });
}

void WebServerController::registerApiRoutes() {
    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        JsonDocument doc;
        const OperationStatus progress = pump_->getProgress();
        doc["state"] = systemStateToString(progress.state);
        if (progress.profileId.isEmpty()) {
            doc["active_profile"] = nullptr;
        } else {
            doc["active_profile"] = progress.profileId;
        }
        doc["motor_running"] = stepper_->isRunning();
        doc["valve_open"] = valve_->isOpen();
        doc["estop_enabled"] = safety_->isEnabled();
        doc["estop_active"] = safety_->isEmergencyStopActive();
        doc["ip"] = WiFi.status() == WL_CONNECTED
            ? WiFi.localIP().toString()
            : WiFi.softAPIP().toString();
        const char* fault = faultCodeToString(pump_->lastFault());
        if (fault == nullptr) {
            doc["fault"] = nullptr;
        } else {
            doc["fault"] = fault;
        }
        sendJson(request, 200, doc);
    });

    server_.on("/api/profiles", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const FluidProfile& profile : profiles_->all()) {
            JsonObject obj = arr.add<JsonObject>();
            obj["id"] = profile.id;
            obj["name"] = profile.name;
            obj["enabled"] = profile.enabled;
            obj["calibrated"] = profile.calibrated;
            obj["steps_per_ml"] = profile.calibration.stepsPerMl;
            obj["ml_per_second"] = profile.calibration.mlPerSecond;
            obj["speed_steps_per_second"] = profile.motor.speedStepsPerSecond;
            obj["acceleration_steps_per_second_squared"] =
                profile.motor.accelerationStepsPerSecondSquared;
            obj["minimum_ml"] = profile.limits.minimumMl;
            obj["maximum_ml"] = profile.limits.maximumMl;
        }
        sendJson(request, 200, doc);
    });

    // Profile detail / update / history / reset via /api/profile?id=
    server_.on("/api/profile", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        if (!request->hasParam("id")) {
            sendError(request, 400, "missing_id");
            return;
        }
        const String id = request->getParam("id")->value();
        if (!profiles_->has(id)) {
            sendError(request, 404, "profile_not_found");
            return;
        }
        JsonDocument doc;
        fillProfileJson(profiles_->get(id), doc.to<JsonObject>());
        sendJson(request, 200, doc);
    });

    server_.on(
        "/api/profile",
        HTTP_PUT,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            if (!request->hasParam("id")) {
                sendError(request, 400, "missing_id");
                return;
            }
            const String id = request->getParam("id")->value();
            if (!profiles_->has(id)) {
                sendError(request, 404, "profile_not_found");
                return;
            }

            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }

            FluidProfile profile = profiles_->get(id);
            if (body["name"].is<const char*>()) {
                profile.name = body["name"].as<const char*>();
            }
            if (body["enabled"].is<bool>()) {
                profile.enabled = body["enabled"].as<bool>();
            }

            JsonObjectConst motor = body["motor"].as<JsonObjectConst>();
            if (!motor.isNull()) {
                const uint32_t previousSpeed = profile.motor.speedStepsPerSecond;
                profile.motor.speedStepsPerSecond =
                    motor["speed_steps_per_second"] |
                    profile.motor.speedStepsPerSecond;
                profile.motor.accelerationStepsPerSecondSquared =
                    motor["acceleration_steps_per_second_squared"] |
                    profile.motor.accelerationStepsPerSecondSquared;
                profile.motor.decelerationStepsPerSecondSquared =
                    motor["deceleration_steps_per_second_squared"] |
                    profile.motor.decelerationStepsPerSecondSquared;
                profile.motor.directionInverted =
                    motor["direction_inverted"] | profile.motor.directionInverted;
                if (!clampMotorSettings(profile.motor)) {
                    sendError(request, 400, "motor_settings_out_of_range");
                    return;
                }
                if (profile.motor.speedStepsPerSecond != previousSpeed &&
                    profile.calibrated) {
                    profile.calibrated = false;
                    profile.calibration.valid = false;
                }
            }

            JsonObjectConst limits = body["limits"].as<JsonObjectConst>();
            if (!limits.isNull()) {
                profile.limits.minimumMl =
                    limits["minimum_ml"] | profile.limits.minimumMl;
                profile.limits.maximumMl =
                    limits["maximum_ml"] | profile.limits.maximumMl;
            }

            JsonObjectConst valve = body["valve"].as<JsonObjectConst>();
            if (!valve.isNull()) {
                const bool previousAntiDrip = profile.valve.antiDripEnabled;
                const int32_t previousReverse = profile.valve.antiDripReverseSteps;
                const uint32_t previousAntiSpeed =
                    profile.valve.antiDripSpeedStepsPerSecond;
                profile.valve.enabled = valve["enabled"] | profile.valve.enabled;
                profile.valve.activeHigh =
                    valve["active_high"] | profile.valve.activeHigh;
                profile.valve.preOpenMs =
                    valve["pre_open_ms"] | profile.valve.preOpenMs;
                profile.valve.postMotorCloseMs =
                    valve["post_motor_close_ms"] | profile.valve.postMotorCloseMs;
                profile.valve.antiDripEnabled =
                    valve["anti_drip_enabled"] | profile.valve.antiDripEnabled;
                profile.valve.antiDripReverseSteps =
                    valve["anti_drip_reverse_steps"] |
                    profile.valve.antiDripReverseSteps;
                profile.valve.antiDripSpeedStepsPerSecond =
                    valve["anti_drip_speed_steps_per_second"] |
                    profile.valve.antiDripSpeedStepsPerSecond;
                if (!clampValveSettings(profile.valve)) {
                    sendError(request, 400, "valve_settings_out_of_range");
                    return;
                }
                const bool antiDripChanged =
                    previousAntiDrip != profile.valve.antiDripEnabled ||
                    previousReverse != profile.valve.antiDripReverseSteps ||
                    previousAntiSpeed != profile.valve.antiDripSpeedStepsPerSecond;
                if (antiDripChanged && profile.calibrated) {
                    profile.calibrated = false;
                    profile.calibration.valid = false;
                }
            }

            if (!profiles_->save(profile)) {
                sendError(request, 500, "storage_failure");
                return;
            }

            if (logger_ != nullptr) {
                JsonDocument fields;
                fields["profile_id"] = id;
                logger_->log("configuration_changed", fields);
            }

            JsonDocument doc;
            fillProfileJson(profile, doc.to<JsonObject>());
            sendJson(request, 200, doc);
        }
    );

    server_.on("/api/profile/reset-calibration", HTTP_POST,
        [this](AsyncWebServerRequest* request) {
            if (!requireAuth(request)) {
                return;
            }
            if (!request->hasParam("id")) {
                sendError(request, 400, "missing_id");
                return;
            }
            const String id = request->getParam("id")->value();
            if (!profiles_->has(id)) {
                sendError(request, 404, "profile_not_found");
                return;
            }
            FluidProfile profile = profiles_->get(id);
            profile.calibrated = false;
            profile.calibration = CalibrationData{};
            if (!profiles_->save(profile)) {
                sendError(request, 500, "storage_failure");
                return;
            }
            JsonDocument doc;
            doc["reset"] = true;
            sendJson(request, 200, doc);
        });

    server_.on("/api/profile/history", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        if (!request->hasParam("id")) {
            sendError(request, 400, "missing_id");
            return;
        }
        const String id = request->getParam("id")->value();
        if (!profiles_->has(id)) {
            sendError(request, 404, "profile_not_found");
            return;
        }
        JsonDocument doc;
        doc["profile_id"] = id;
        JsonArray entries = doc["entries"].to<JsonArray>();
        for (const CalibrationHistoryEntry& entry :
             profiles_->getCalibrationHistory(id)) {
            JsonObject obj = entries.add<JsonObject>();
            obj["timestamp"] = entry.timestamp;
            obj["requested_duration_ms"] = entry.requestedDurationMs;
            obj["actual_duration_ms"] = entry.actualDurationMs;
            obj["step_count"] = entry.stepCount;
            obj["measured_ml"] = entry.measuredMl;
            obj["steps_per_ml"] = entry.stepsPerMl;
        }
        sendJson(request, 200, doc);
    });

    server_.on(
        "/api/calibration/start",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            const char* profileId = body["profile_id"] | "fluid_1";
            const uint32_t durationMs =
                body["duration_ms"] | Config::kDefaultCalibrationDurationMs;
            if (!pump_->startCalibration(profileId, durationMs)) {
                sendError(
                    request,
                    409,
                    faultCodeToString(pump_->lastFault()) != nullptr
                        ? faultCodeToString(pump_->lastFault())
                        : "calibration_rejected"
                );
                return;
            }
            JsonDocument doc;
            doc["accepted"] = true;
            doc["operation_id"] = pump_->activeOperationId();
            sendJson(request, 200, doc);
        }
    );

    server_.on(
        "/api/calibration/measurement",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            const float measuredMl = body["measured_ml"] | -1.0f;
            if (!pump_->submitCalibrationMeasurement(measuredMl)) {
                sendError(request, 400, "invalid_calibration");
                return;
            }
            const CalibrationResult result = pump_->calculateCalibrationResult();
            JsonDocument doc;
            doc["sample_count"] = result.sampleCount;
            doc["steps_per_ml"] = result.stepsPerMl;
            doc["ml_per_second"] = result.mlPerSecond;
            doc["average_ml"] = result.sampleAverageMl;
            doc["std_dev_ml"] = result.sampleStdDevMl;
            doc["coefficient_of_variation"] = result.coefficientOfVariation;
            doc["variation_warning"] =
                result.coefficientOfVariation > Config::kCalibrationCvWarningPercent;
            doc["saved"] = false;
            sendJson(request, 200, doc);
        }
    );

    server_.on(
        "/api/calibration/sample/remove",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            const size_t index = body["index"] | 9999;
            if (!pump_->removeCalibrationSample(index)) {
                sendError(request, 400, "invalid_sample_index");
                return;
            }
            const CalibrationResult result = pump_->calculateCalibrationResult();
            JsonDocument doc;
            doc["sample_count"] = result.sampleCount;
            doc["steps_per_ml"] = result.stepsPerMl;
            doc["average_ml"] = result.sampleAverageMl;
            doc["coefficient_of_variation"] = result.coefficientOfVariation;
            sendJson(request, 200, doc);
        }
    );

    server_.on(
        "/api/calibration/save",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            (void)data;
            (void)len;
            if (!requireAuth(request)) {
                return;
            }
            if (!pump_->saveCalibration()) {
                sendError(request, 400, "save_failed");
                return;
            }
            const String profileId = pump_->getProgress().profileId;
            const FluidProfile profile = profiles_->get(profileId);
            JsonDocument doc;
            doc["saved"] = true;
            doc["profile_id"] = profile.id;
            doc["steps_per_ml"] = profile.calibration.stepsPerMl;
            sendJson(request, 200, doc);
        }
    );

    server_.on("/api/calibration/discard", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        pump_->discardPendingCalibration();
        JsonDocument doc;
        doc["discarded"] = true;
        sendJson(request, 200, doc);
    });

    server_.on("/api/calibration/pending", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        JsonDocument doc;
        JsonArray samples = doc["samples"].to<JsonArray>();
        for (const CalibrationSample& sample : pump_->pendingSamples()) {
            JsonObject obj = samples.add<JsonObject>();
            obj["measured_ml"] = sample.measuredMl;
            obj["step_count"] = sample.stepCount;
            obj["actual_duration_ms"] = sample.actualDurationMs;
        }
        const CalibrationResult result = pump_->calculateCalibrationResult();
        doc["sample_count"] = result.sampleCount;
        doc["steps_per_ml"] = result.stepsPerMl;
        doc["average_ml"] = result.sampleAverageMl;
        doc["coefficient_of_variation"] = result.coefficientOfVariation;
        doc["variation_warning"] =
            result.coefficientOfVariation > Config::kCalibrationCvWarningPercent;
        sendJson(request, 200, doc);
    });

    server_.on(
        "/api/dispense",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            DispenseRequest dispense;
            dispense.profileId = body["profile_id"] | "fluid_1";
            dispense.requestedMl = body["volume_ml"] | 0.0f;
            if (!pump_->startDispense(dispense)) {
                sendError(
                    request,
                    409,
                    faultCodeToString(pump_->lastFault()) != nullptr
                        ? faultCodeToString(pump_->lastFault())
                        : "dispense_rejected"
                );
                return;
            }
            const OperationStatus progress = pump_->getProgress();
            JsonDocument doc;
            doc["accepted"] = true;
            doc["operation_id"] = progress.operationId;
            doc["target_steps"] = progress.targetSteps;
            doc["estimated_duration_ms"] = progress.estimatedRemainingMs;
            sendJson(request, 200, doc);
        }
    );

    server_.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        pump_->requestStop();
        JsonDocument doc;
        doc["stopped"] = true;
        sendJson(request, 200, doc);
    });

    server_.on("/api/fault/ack", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        if (!pump_->acknowledgeFault()) {
            sendError(request, 409, "fault_still_active");
            return;
        }
        JsonDocument doc;
        doc["acknowledged"] = true;
        sendJson(request, 200, doc);
    });

    server_.on("/api/operation", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        const OperationStatus progress = pump_->getProgress();
        JsonDocument doc;
        doc["operation_id"] = progress.operationId;
        doc["state"] = systemStateToString(progress.state);
        doc["profile_id"] = progress.profileId;
        doc["requested_ml"] = progress.requestedMl;
        doc["estimated_delivered_ml"] = progress.estimatedDeliveredMl;
        doc["completed_steps"] = progress.completedSteps;
        doc["target_steps"] = progress.targetSteps;
        doc["progress_percent"] = progress.progressPercent;
        doc["elapsed_ms"] = progress.elapsedMs;
        doc["estimated_remaining_ms"] = progress.estimatedRemainingMs;
        doc["completion_reason"] = progress.completionReason;
        sendJson(request, 200, doc);
    });

    server_.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        const GlobalSettings settings = settings_->get();
        JsonDocument doc;
        doc["device_name"] = settings.deviceName;
        doc["hostname"] = settings.hostname;
        doc["wifi_mode"] = settings.wifiMode;
        doc["emergency_stop_enabled"] = settings.emergencyStopEnabled;
        doc["logging_enabled"] = settings.loggingEnabled;
        doc["web_auth_enabled"] = settings.webAuthEnabled;
        doc["valve_hardware_present"] = settings.valveHardwarePresent;
        sendJson(request, 200, doc);
    });

    server_.on(
        "/api/settings",
        HTTP_PUT,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            GlobalSettings settings = settings_->get();
            settings.deviceName = body["device_name"] | settings.deviceName;
            settings.hostname = body["hostname"] | settings.hostname;
            settings.loggingEnabled = body["logging_enabled"] | settings.loggingEnabled;
            settings.webAuthEnabled = body["web_auth_enabled"] | settings.webAuthEnabled;
            settings.emergencyStopEnabled =
                body["emergency_stop_enabled"] | settings.emergencyStopEnabled;
            settings.valveHardwarePresent =
                body["valve_hardware_present"] | settings.valveHardwarePresent;
            if (!settings_->save(settings)) {
                sendError(request, 500, "storage_failure");
                return;
            }
            logger_->setEnabled(settings.loggingEnabled);
            if (valve_ != nullptr) {
                valve_->begin(
                    PUMP_VALVE_PIN,
                    settings.valveHardwarePresent,
                    true
                );
            }
            if (safety_ != nullptr) {
                safety_->begin(PUMP_ESTOP_PIN, settings.emergencyStopEnabled);
            }
            JsonDocument doc;
            doc["saved"] = true;
            doc["web_auth_enabled"] = settings.webAuthEnabled;
            doc["valve_hardware_present"] = settings.valveHardwarePresent;
            doc["emergency_stop_enabled"] = settings.emergencyStopEnabled;
            sendJson(request, 200, doc);
        }
    );

    server_.on("/api/events/log", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        request->send(200, "application/json", logger_->readRecent(50));
    });

    server_.on("/api/events/log", HTTP_DELETE, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        logger_->clear();
        JsonDocument doc;
        doc["cleared"] = true;
        sendJson(request, 200, doc);
    });

    server_.on("/api/config/export", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!requireAuth(request)) {
            return;
        }
        JsonDocument doc;
        doc["format"] = "pump-controller-config";
        doc["version"] = 1;
        const GlobalSettings settings = settings_->get();
        JsonObject settingsObj = doc["settings"].to<JsonObject>();
        settingsObj["device_name"] = settings.deviceName;
        settingsObj["hostname"] = settings.hostname;
        settingsObj["logging_enabled"] = settings.loggingEnabled;
        settingsObj["web_auth_enabled"] = settings.webAuthEnabled;
        settingsObj["emergency_stop_enabled"] = settings.emergencyStopEnabled;
        settingsObj["valve_hardware_present"] = settings.valveHardwarePresent;
        profiles_->exportToJson(doc);
        sendJson(request, 200, doc);
    });

    server_.on(
        "/api/config/import",
        HTTP_POST,
        [](AsyncWebServerRequest* request) {},
        nullptr,
        [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t, size_t) {
            if (!requireAuth(request)) {
                return;
            }
            JsonDocument body;
            if (!readJsonBody(data, len, body)) {
                sendError(request, 400, "invalid_json");
                return;
            }
            if (!profiles_->importFromJson(body)) {
                sendError(request, 400, "invalid_config");
                return;
            }
            JsonObjectConst settingsObj = body["settings"].as<JsonObjectConst>();
            if (!settingsObj.isNull()) {
                GlobalSettings settings = settings_->get();
                settings.deviceName = settingsObj["device_name"] | settings.deviceName;
                settings.hostname = settingsObj["hostname"] | settings.hostname;
                settings.loggingEnabled =
                    settingsObj["logging_enabled"] | settings.loggingEnabled;
                settings.webAuthEnabled =
                    settingsObj["web_auth_enabled"] | settings.webAuthEnabled;
                settings.emergencyStopEnabled =
                    settingsObj["emergency_stop_enabled"] |
                    settings.emergencyStopEnabled;
                settings.valveHardwarePresent =
                    settingsObj["valve_hardware_present"] |
                    settings.valveHardwarePresent;
                settings_->save(settings);
                logger_->setEnabled(settings.loggingEnabled);
            }
            if (logger_ != nullptr) {
                logger_->log("configuration_imported");
            }
            JsonDocument doc;
            doc["imported"] = true;
            doc["profile_count"] = profiles_->count();
            sendJson(request, 200, doc);
        }
    );
}

void WebServerController::update() {
    if (pump_ == nullptr) {
        return;
    }

    const OperationStatus progress = pump_->getProgress();
    const uint32_t now = millis();
    const bool stateChanged = progress.state != lastBroadcastState_;
    const bool progressDue =
        (progress.state == SystemState::Dispensing ||
         progress.state == SystemState::Calibrating) &&
        (now - lastProgressBroadcastMs_ >= 250);

    if (!stateChanged && !progressDue) {
        return;
    }

    lastBroadcastState_ = progress.state;
    lastProgressBroadcastMs_ = now;

    JsonDocument doc;
    doc["operation_id"] = progress.operationId;
    doc["state"] = systemStateToString(progress.state);
    doc["profile_id"] = progress.profileId;
    doc["requested_ml"] = progress.requestedMl;
    doc["estimated_delivered_ml"] = progress.estimatedDeliveredMl;
    doc["completed_steps"] = progress.completedSteps;
    doc["target_steps"] = progress.targetSteps;
    doc["progress_percent"] = progress.progressPercent;
    doc["elapsed_ms"] = progress.elapsedMs;
    doc["estimated_remaining_ms"] = progress.estimatedRemainingMs;
    doc["completion_reason"] = progress.completionReason;
    String payload;
    serializeJson(doc, payload);
    events_.send(payload.c_str(), "operation", now);
}

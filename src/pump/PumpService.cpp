#include "pump/PumpService.h"

#include <ArduinoJson.h>
#include <cmath>

#include "Config.h"

void PumpService::begin(
    StepperController& stepper,
    ValveController& valve,
    ProfileRepository& profiles,
    SafetyController& safety,
    EventLogger& logger
) {
    stepper_ = &stepper;
    valve_ = &valve;
    profiles_ = &profiles;
    safety_ = &safety;
    logger_ = &logger;
    applySafeOutputs();
    sequencePhase_ = SequencePhase::Idle;
    pendingMotion_ = PendingMotion::None;
    state_ = SystemState::Idle;
}

void PumpService::logEvent(const char* event) {
    if (logger_ == nullptr) {
        return;
    }
    JsonDocument fields;
    fields["operation_id"] = operationId_;
    fields["profile_id"] = activeProfileId_;
    fields["state"] = systemStateToString(state_);
    logger_->log(event, fields);
}

bool PumpService::isBusy() const {
    return state_ != SystemState::Idle &&
           state_ != SystemState::Completed &&
           state_ != SystemState::Fault &&
           state_ != SystemState::AwaitingCalibrationMeasurement;
}

void PumpService::applySafeOutputs() {
    if (stepper_ != nullptr) {
        stepper_->stopImmediate();
    }
    if (valve_ != nullptr) {
        valve_->close();
    }
    sequencePhase_ = SequencePhase::Idle;
    pendingMotion_ = PendingMotion::None;
    valveInUse_ = false;
}

bool PumpService::fail(FaultCode code) {
    lastFault_ = code;
    if (safety_ != nullptr) {
        safety_->raiseFault(code);
    }
    applySafeOutputs();
    state_ = SystemState::Fault;
    completionReason_ = faultCodeToString(code);
    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["fault"] = faultCodeToString(code);
        fields["profile_id"] = activeProfileId_;
        logger_->log("fault", fields);
    }
    return false;
}

void PumpService::enterIdle() {
    applySafeOutputs();
    state_ = SystemState::Idle;
    awaitingMeasurement_ = false;
    completionReason_ = "";
}

String PumpService::nextOperationId(const char* prefix) {
    return String(prefix) + String(millis());
}

bool PumpService::valveShouldRun(const FluidProfile& profile) const {
    return profile.valve.enabled &&
           valve_ != nullptr &&
           valve_->isAvailable();
}

bool PumpService::beginValvePreOpen(const FluidProfile& profile) {
    valveInUse_ = true;
    valvePreOpenMs_ = profile.valve.preOpenMs;
    valvePostCloseMs_ = profile.valve.postMotorCloseMs;
    if (!valve_->open()) {
        return fail(FaultCode::ValveFault);
    }
    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["profile_id"] = activeProfileId_;
        fields["pre_open_ms"] = valvePreOpenMs_;
        logger_->log("valve_opened", fields);
    }
    sequencePhase_ = SequencePhase::ValvePreOpen;
    phaseStartedMs_ = millis();
    if (valvePreOpenMs_ == 0) {
        return startPendingMotor();
    }
    return true;
}

bool PumpService::startPendingMotor() {
    bool started = false;
    if (pendingMotion_ == PendingMotion::Dispense) {
        started = stepper_->startMove(
            targetSteps_,
            pendingSpeed_,
            pendingAccel_,
            pendingDirectionInverted_
        );
    } else if (pendingMotion_ == PendingMotion::Calibration) {
        started = stepper_->startTimedMove(
            calibrationDurationMs_,
            pendingSpeed_,
            pendingAccel_,
            pendingDirectionInverted_
        );
    }

    if (!started) {
        valve_->close();
        return fail(FaultCode::InternalError);
    }

    sequencePhase_ = SequencePhase::MotorRunning;
    phaseStartedMs_ = millis();
    return true;
}

void PumpService::beginValvePostClose() {
    if (!valveInUse_ || valvePostCloseMs_ == 0) {
        completeValvePostClose();
        return;
    }
    sequencePhase_ = SequencePhase::ValvePostClose;
    phaseStartedMs_ = millis();
}

void PumpService::completeValvePostClose() {
    if (valveInUse_ && valve_ != nullptr) {
        valve_->close();
        if (logger_ != nullptr) {
            JsonDocument fields;
            fields["profile_id"] = activeProfileId_;
            fields["post_close_ms"] = valvePostCloseMs_;
            logger_->log("valve_closed", fields);
        }
    }
    valveInUse_ = false;
    sequencePhase_ = SequencePhase::Idle;

    if (pendingMotion_ == PendingMotion::Calibration) {
        stepper_->setEnabled(false);
        awaitingMeasurement_ = true;
        state_ = SystemState::AwaitingCalibrationMeasurement;
        completionReason_ = "awaiting_measurement";
        logEvent("calibration_completed");
    } else {
        stepper_->setEnabled(false);
        state_ = SystemState::Completed;
        completionReason_ = "target_reached";
        if (logger_ != nullptr) {
            JsonDocument fields;
            fields["operation_id"] = operationId_;
            fields["profile_id"] = activeProfileId_;
            fields["requested_ml"] = requestedMl_;
            fields["commanded_steps"] = targetSteps_;
            fields["elapsed_ms"] = millis() - operationStartMs_;
            logger_->log("dispense_completed", fields);
        }
    }
    pendingMotion_ = PendingMotion::None;
}

bool PumpService::startDispense(const DispenseRequest& request) {
    if (state_ != SystemState::Idle && state_ != SystemState::Completed) {
        return fail(FaultCode::OperationAlreadyRunning);
    }
    if (safety_ != nullptr && safety_->isEmergencyStopActive()) {
        return fail(FaultCode::EmergencyStop);
    }
    if (!profiles_->has(request.profileId)) {
        return fail(FaultCode::InternalError);
    }

    FluidProfile profile = profiles_->get(request.profileId);
    if (!profile.enabled) {
        return fail(FaultCode::InternalError);
    }
    if (!profile.calibrated || profile.calibration.stepsPerMl <= 0.0f) {
        return fail(FaultCode::NotCalibrated);
    }
    if (!std::isfinite(request.requestedMl) || request.requestedMl <= 0.0f) {
        return fail(FaultCode::InvalidVolume);
    }
    if (request.requestedMl < profile.limits.minimumMl) {
        return fail(FaultCode::VolumeBelowLimit);
    }
    if (request.requestedMl > profile.limits.maximumMl) {
        return fail(FaultCode::VolumeAboveLimit);
    }

    const int64_t targetSteps = llround(
        static_cast<double>(request.requestedMl) *
        static_cast<double>(profile.calibration.stepsPerMl)
    );
    if (targetSteps <= 0 || targetSteps > Config::kMaxOperationSteps) {
        return fail(FaultCode::InvalidVolume);
    }

    operationId_ = nextOperationId("disp-");
    activeProfileId_ = request.profileId;
    requestedMl_ = request.requestedMl;
    targetSteps_ = targetSteps;
    operationStartMs_ = millis();
    completionReason_ = "";
    pendingMotion_ = PendingMotion::Dispense;
    pendingSpeed_ = profile.motor.speedStepsPerSecond;
    pendingAccel_ = profile.motor.accelerationStepsPerSecondSquared;
    pendingDirectionInverted_ = profile.motor.directionInverted;

    const double expectedSeconds =
        static_cast<double>(targetSteps) /
        static_cast<double>(profile.motor.speedStepsPerSecond);
    const uint32_t valveMarginMs =
        valveShouldRun(profile)
            ? (profile.valve.preOpenMs + profile.valve.postMotorCloseMs)
            : 0;
    maxRuntimeMs_ = static_cast<uint32_t>(
        expectedSeconds * 1000.0 * 1.2 + 2000.0 + valveMarginMs
    );

    state_ = SystemState::Dispensing;
    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["operation_id"] = operationId_;
        fields["profile_id"] = activeProfileId_;
        fields["requested_ml"] = requestedMl_;
        fields["target_steps"] = targetSteps_;
        logger_->log("dispense_started", fields);
    }

    if (valveShouldRun(profile)) {
        return beginValvePreOpen(profile);
    }

    valveInUse_ = false;
    return startPendingMotor();
}

bool PumpService::startCalibration(const String& profileId, uint32_t durationMs) {
    if (state_ != SystemState::Idle &&
        state_ != SystemState::Completed &&
        state_ != SystemState::AwaitingCalibrationMeasurement) {
        return fail(FaultCode::OperationAlreadyRunning);
    }
    if (safety_ != nullptr && safety_->isEmergencyStopActive()) {
        return fail(FaultCode::EmergencyStop);
    }
    if (!profiles_->has(profileId)) {
        return fail(FaultCode::InternalError);
    }
    if (durationMs == 0) {
        return fail(FaultCode::InvalidCalibration);
    }

    FluidProfile profile = profiles_->get(profileId);
    if (!profile.enabled) {
        return fail(FaultCode::InternalError);
    }

    operationId_ = nextOperationId("cal-");
    activeProfileId_ = profileId;
    requestedMl_ = 0.0f;
    targetSteps_ = 0;
    calibrationDurationMs_ = durationMs;
    operationStartMs_ = millis();
    awaitingMeasurement_ = false;
    completionReason_ = "";
    pendingMotion_ = PendingMotion::Calibration;
    pendingSpeed_ = profile.motor.speedStepsPerSecond;
    pendingAccel_ = profile.motor.accelerationStepsPerSecondSquared;
    pendingDirectionInverted_ = profile.motor.directionInverted;

    const uint32_t valveMarginMs =
        valveShouldRun(profile)
            ? (profile.valve.preOpenMs + profile.valve.postMotorCloseMs)
            : 0;
    maxRuntimeMs_ = durationMs + (durationMs / 5) + 2000 + valveMarginMs;

    state_ = SystemState::Calibrating;
    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["operation_id"] = operationId_;
        fields["profile_id"] = activeProfileId_;
        fields["duration_ms"] = durationMs;
        fields["speed"] = profile.motor.speedStepsPerSecond;
        logger_->log("calibration_started", fields);
    }

    if (valveShouldRun(profile)) {
        return beginValvePreOpen(profile);
    }

    valveInUse_ = false;
    return startPendingMotor();
}

void PumpService::requestStop() {
    if (state_ == SystemState::Idle ||
        state_ == SystemState::Fault ||
        state_ == SystemState::AwaitingCalibrationMeasurement) {
        return;
    }
    state_ = SystemState::Stopping;
    stepper_->stopImmediate();
    if (valve_ != nullptr) {
        valve_->close();
    }
    if (valveInUse_ && logger_ != nullptr) {
        JsonDocument fields;
        fields["profile_id"] = activeProfileId_;
        fields["reason"] = "stopped_by_user";
        logger_->log("valve_closed", fields);
    }
    completionReason_ = "stopped_by_user";
    logEvent("dispense_stopped");
    completeStop();
}

bool PumpService::acknowledgeFault() {
    if (state_ != SystemState::Fault) {
        return true;
    }
    if (safety_ != nullptr && !safety_->acknowledgeFault()) {
        return false;
    }
    lastFault_ = FaultCode::None;
    enterIdle();
    return true;
}

void PumpService::finishCalibrationMotion() {
    pendingCalibrationSteps_ = stepper_->stepsCompleted();
    pendingCalibrationActualMs_ = millis() - operationStartMs_;
    beginValvePostClose();
}

void PumpService::finishDispense() {
    beginValvePostClose();
}

void PumpService::completeStop() {
    sequencePhase_ = SequencePhase::Idle;
    pendingMotion_ = PendingMotion::None;
    valveInUse_ = false;
    stepper_->setEnabled(false);
    state_ = SystemState::Idle;
}

void PumpService::update() {
    if (safety_ != nullptr && safety_->isEmergencyStopActive()) {
        if (state_ != SystemState::Fault) {
            applySafeOutputs();
            lastFault_ = FaultCode::EmergencyStop;
            state_ = SystemState::Fault;
            completionReason_ = "emergency_stop";
            logEvent("fault");
        }
        return;
    }

    if (state_ != SystemState::Dispensing && state_ != SystemState::Calibrating) {
        return;
    }

    if ((millis() - operationStartMs_) > maxRuntimeMs_) {
        fail(FaultCode::MotorTimeout);
        return;
    }

    if (sequencePhase_ == SequencePhase::ValvePreOpen) {
        if (millis() - phaseStartedMs_ >= valvePreOpenMs_) {
            if (!startPendingMotor()) {
                return;
            }
        }
        return;
    }

    if (sequencePhase_ == SequencePhase::ValvePostClose) {
        if (millis() - phaseStartedMs_ >= valvePostCloseMs_) {
            completeValvePostClose();
        }
        return;
    }

    if (sequencePhase_ == SequencePhase::MotorRunning) {
        stepper_->update();
        if (!stepper_->isRunning()) {
            if (state_ == SystemState::Calibrating) {
                finishCalibrationMotion();
            } else {
                finishDispense();
            }
        }
    }
}

OperationStatus PumpService::getProgress() const {
    OperationStatus status;
    status.operationId = operationId_;
    status.state = state_;
    status.profileId = activeProfileId_;
    status.requestedMl = requestedMl_;
    status.targetSteps = targetSteps_;
    status.completedSteps = stepper_ != nullptr ? stepper_->stepsCompleted() : 0;
    status.elapsedMs = operationStartMs_ == 0 ? 0 : (millis() - operationStartMs_);
    status.completionReason = completionReason_;
    status.fault = lastFault_;

    if (state_ == SystemState::Calibrating ||
        state_ == SystemState::AwaitingCalibrationMeasurement) {
        status.progressPercent =
            state_ == SystemState::AwaitingCalibrationMeasurement
                ? 100.0f
                : (stepper_ != nullptr ? stepper_->progress() * 100.0f : 0.0f);
        return status;
    }

    FluidProfile profile = profiles_->get(activeProfileId_);
    if (profile.calibrated && profile.calibration.stepsPerMl > 0.0f) {
        status.estimatedDeliveredMl =
            static_cast<float>(status.completedSteps) /
            profile.calibration.stepsPerMl;
    }

    if (targetSteps_ > 0) {
        status.progressPercent =
            100.0f * static_cast<float>(status.completedSteps) /
            static_cast<float>(targetSteps_);
        const int64_t remaining = targetSteps_ - status.completedSteps;
        if (remaining > 0 && profile.motor.speedStepsPerSecond > 0) {
            status.estimatedRemainingMs = static_cast<uint32_t>(
                (remaining * 1000LL) / profile.motor.speedStepsPerSecond
            );
        }
    }

    return status;
}

bool PumpService::submitCalibrationMeasurement(float measuredMl) {
    if (state_ != SystemState::AwaitingCalibrationMeasurement ||
        !awaitingMeasurement_) {
        return false;
    }
    if (!std::isfinite(measuredMl) || measuredMl <= 0.0f) {
        return false;
    }

    CalibrationSample sample;
    sample.requestedDurationMs = calibrationDurationMs_;
    sample.actualDurationMs = pendingCalibrationActualMs_;
    sample.stepCount = pendingCalibrationSteps_;
    sample.measuredMl = measuredMl;
    sample.timestamp = millis();
    samples_.push_back(sample);

    awaitingMeasurement_ = false;
    state_ = SystemState::Idle;
    completionReason_ = "measurement_accepted";

    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["profile_id"] = activeProfileId_;
        fields["measured_ml"] = measuredMl;
        fields["step_count"] = sample.stepCount;
        fields["sample_count"] = samples_.size();
        logger_->log("calibration_measurement_submitted", fields);
    }
    return true;
}

bool PumpService::removeCalibrationSample(size_t index) {
    if (index >= samples_.size()) {
        return false;
    }
    samples_.erase(samples_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

CalibrationResult PumpService::calculateCalibrationResult() const {
    CalibrationResult result;
    if (samples_.empty()) {
        return result;
    }

    double totalStepsPerMl = 0.0;
    double totalMlPerSecond = 0.0;
    double totalMl = 0.0;

    for (const CalibrationSample& sample : samples_) {
        totalStepsPerMl +=
            static_cast<double>(sample.stepCount) / sample.measuredMl;
        totalMlPerSecond +=
            sample.measuredMl / (sample.actualDurationMs / 1000.0);
        totalMl += sample.measuredMl;
    }

    result.sampleCount = static_cast<uint8_t>(samples_.size());
    result.stepsPerMl =
        static_cast<float>(totalStepsPerMl / result.sampleCount);
    result.mlPerSecond =
        static_cast<float>(totalMlPerSecond / result.sampleCount);
    result.sampleAverageMl =
        static_cast<float>(totalMl / result.sampleCount);

    if (result.sampleCount > 1) {
        double varianceSum = 0.0;
        for (const CalibrationSample& sample : samples_) {
            const double delta = sample.measuredMl - result.sampleAverageMl;
            varianceSum += delta * delta;
        }
        result.sampleStdDevMl =
            static_cast<float>(sqrt(varianceSum / (result.sampleCount - 1)));
        if (result.sampleAverageMl > 0.0f) {
            result.coefficientOfVariation =
                (result.sampleStdDevMl / result.sampleAverageMl) * 100.0f;
        }
    }

    result.valid = result.stepsPerMl > 0.0f;
    return result;
}

bool PumpService::saveCalibration() {
    const CalibrationResult result = calculateCalibrationResult();
    if (!result.valid || !profiles_->has(activeProfileId_)) {
        return false;
    }

    FluidProfile profile = profiles_->get(activeProfileId_);
    profile.calibrated = true;
    profile.calibration.valid = true;
    profile.calibration.method = "timed_step_count";
    profile.calibration.stepsPerMl = result.stepsPerMl;
    profile.calibration.mlPerSecond = result.mlPerSecond;
    profile.calibration.measuredMl = result.sampleAverageMl;
    profile.calibration.sampleCount = result.sampleCount;
    profile.calibration.requestedDurationMs = calibrationDurationMs_;

    if (!samples_.empty()) {
        const CalibrationSample& last = samples_.back();
        profile.calibration.actualDurationMs = last.actualDurationMs;
        profile.calibration.stepCount = last.stepCount;
    }

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(millis()));
    profile.calibration.calibratedAt = buffer;

    if (!profiles_->save(profile)) {
        return fail(FaultCode::StorageFailure);
    }

    CalibrationHistoryEntry history;
    history.timestamp = profile.calibration.calibratedAt;
    history.requestedDurationMs = profile.calibration.requestedDurationMs;
    history.actualDurationMs = profile.calibration.actualDurationMs;
    history.stepCount = profile.calibration.stepCount;
    history.measuredMl = profile.calibration.measuredMl;
    history.stepsPerMl = profile.calibration.stepsPerMl;
    profiles_->appendCalibrationHistory(activeProfileId_, history);

    if (logger_ != nullptr) {
        JsonDocument fields;
        fields["profile_id"] = activeProfileId_;
        fields["steps_per_ml"] = result.stepsPerMl;
        fields["sample_count"] = result.sampleCount;
        fields["coefficient_of_variation"] = result.coefficientOfVariation;
        logger_->log("calibration_saved", fields);
    }

    samples_.clear();
    return true;
}

void PumpService::discardPendingCalibration() {
    samples_.clear();
    awaitingMeasurement_ = false;
    if (state_ == SystemState::AwaitingCalibrationMeasurement) {
        enterIdle();
    }
}

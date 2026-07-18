#pragma once

#include <Arduino.h>
#include <vector>

#include "diagnostics/EventLogger.h"
#include "models/FluidProfile.h"
#include "models/OperationStatus.h"
#include "motor/StepperController.h"
#include "motor/TmcDriverController.h"
#include "safety/SafetyController.h"
#include "sensors/ReservoirSensor.h"
#include "storage/ProfileRepository.h"
#include "valve/ValveController.h"

struct CalibrationSample {
    uint32_t requestedDurationMs = 0;
    uint32_t actualDurationMs = 0;
    int64_t stepCount = 0;
    float measuredMl = 0.0f;
    uint64_t timestamp = 0;
};

struct CalibrationResult {
    bool valid = false;
    float stepsPerMl = 0.0f;
    float mlPerSecond = 0.0f;
    float sampleAverageMl = 0.0f;
    float sampleStdDevMl = 0.0f;
    float coefficientOfVariation = 0.0f;
    uint8_t sampleCount = 0;
};

enum class ReservoirEmptyPolicy : uint8_t {
    Warn,
    Block,
    Fault
};

inline ReservoirEmptyPolicy parseReservoirEmptyPolicy(const String& value) {
    if (value == "warn") {
        return ReservoirEmptyPolicy::Warn;
    }
    if (value == "fault") {
        return ReservoirEmptyPolicy::Fault;
    }
    return ReservoirEmptyPolicy::Block;
}

inline const char* reservoirEmptyPolicyToString(ReservoirEmptyPolicy policy) {
    switch (policy) {
        case ReservoirEmptyPolicy::Warn: return "warn";
        case ReservoirEmptyPolicy::Fault: return "fault";
        case ReservoirEmptyPolicy::Block:
        default:
            return "block";
    }
}

class PumpService {
public:
    void begin(
        StepperController& stepper,
        ValveController& valve,
        ProfileRepository& profiles,
        SafetyController& safety,
        EventLogger& logger,
        TmcDriverController& tmc,
        ReservoirSensor& reservoir
    );

    void update();
    void setReservoirEmptyPolicy(ReservoirEmptyPolicy policy);

    bool startDispense(const DispenseRequest& request);
    bool startCalibration(const String& profileId, uint32_t durationMs);
    void requestStop();
    bool acknowledgeFault();
    bool injectMotorDriverFault();

    bool submitCalibrationMeasurement(float measuredMl);
    bool removeCalibrationSample(size_t index);
    bool saveCalibration();
    void discardPendingCalibration();
    CalibrationResult calculateCalibrationResult() const;
    const std::vector<CalibrationSample>& pendingSamples() const {
        return samples_;
    }

    SystemState state() const { return state_; }
    OperationStatus getProgress() const;
    bool isBusy() const;
    FaultCode lastFault() const { return lastFault_; }
    const String& activeOperationId() const { return operationId_; }
    ReservoirEmptyPolicy reservoirEmptyPolicy() const {
        return reservoirPolicy_;
    }
    bool reservoirEmptyWarning() const { return reservoirEmptyWarning_; }

private:
    enum class SequencePhase : uint8_t {
        Idle,
        ValvePreOpen,
        MotorRunning,
        AntiDrip,
        ValvePostClose
    };

    enum class PendingMotion : uint8_t {
        None,
        Dispense,
        Calibration
    };

    bool fail(FaultCode code);
    void enterIdle();
    void finishCalibrationMotion();
    void finishDispense();
    void completeStop();
    void applySafeOutputs();
    String nextOperationId(const char* prefix);
    void logEvent(const char* event);
    bool valveShouldRun(const FluidProfile& profile) const;
    bool beginValvePreOpen(const FluidProfile& profile);
    bool startPendingMotor();
    void beginAntiDripOrPostClose();
    bool startAntiDripMotor();
    void beginValvePostClose();
    void completeValvePostClose();
    void pollDriverDiagnostics();
    bool driverFaultPresent() const;
    void pollReservoir();

    StepperController* stepper_ = nullptr;
    ValveController* valve_ = nullptr;
    ProfileRepository* profiles_ = nullptr;
    SafetyController* safety_ = nullptr;
    EventLogger* logger_ = nullptr;
    TmcDriverController* tmc_ = nullptr;
    ReservoirSensor* reservoir_ = nullptr;

    SystemState state_ = SystemState::Booting;
    FaultCode lastFault_ = FaultCode::None;
    SequencePhase sequencePhase_ = SequencePhase::Idle;
    PendingMotion pendingMotion_ = PendingMotion::None;
    String operationId_;
    String activeProfileId_;
    float requestedMl_ = 0.0f;
    int64_t targetSteps_ = 0;
    uint32_t operationStartMs_ = 0;
    uint32_t calibrationDurationMs_ = 0;
    uint32_t maxRuntimeMs_ = 0;
    bool awaitingMeasurement_ = false;
    int64_t pendingCalibrationSteps_ = 0;
    uint32_t pendingCalibrationActualMs_ = 0;
    String completionReason_;

    bool valveInUse_ = false;
    uint32_t valvePreOpenMs_ = 0;
    uint32_t valvePostCloseMs_ = 0;
    uint32_t phaseStartedMs_ = 0;

    uint32_t pendingSpeed_ = 0;
    uint32_t pendingAccel_ = 0;
    bool pendingDirectionInverted_ = false;

    bool antiDripEnabled_ = false;
    int32_t antiDripReverseSteps_ = 0;
    uint32_t antiDripSpeed_ = 0;

    uint32_t lastDriverPollMs_ = 0;
    bool injectedDriverFault_ = false;

    ReservoirEmptyPolicy reservoirPolicy_ = ReservoirEmptyPolicy::Block;
    bool reservoirWasEmpty_ = false;
    bool reservoirEmptyWarning_ = false;

    std::vector<CalibrationSample> samples_;
};

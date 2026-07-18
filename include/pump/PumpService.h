#pragma once

#include <Arduino.h>
#include <vector>

#include "diagnostics/EventLogger.h"
#include "models/FluidProfile.h"
#include "models/OperationStatus.h"
#include "motor/StepperController.h"
#include "motor/TmcDriverController.h"
#include "safety/SafetyController.h"
#include "sensors/FlowSensor.h"
#include "sensors/LoadCellSensor.h"
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

enum class DispenseFeedbackMode : uint8_t {
    OpenLoop,
    VerifyAfter,
    StopOnFeedback
};

enum class DispenseFeedbackSource : uint8_t {
    Auto,
    LoadCell,
    Flow
};

enum class DispenseFeedbackOnMiss : uint8_t {
    Warn,
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

inline DispenseFeedbackMode parseDispenseFeedbackMode(const String& value) {
    if (value == "verify_after") {
        return DispenseFeedbackMode::VerifyAfter;
    }
    if (value == "stop_on_feedback") {
        return DispenseFeedbackMode::StopOnFeedback;
    }
    return DispenseFeedbackMode::OpenLoop;
}

inline const char* dispenseFeedbackModeToString(DispenseFeedbackMode mode) {
    switch (mode) {
        case DispenseFeedbackMode::VerifyAfter: return "verify_after";
        case DispenseFeedbackMode::StopOnFeedback: return "stop_on_feedback";
        case DispenseFeedbackMode::OpenLoop:
        default:
            return "open_loop";
    }
}

inline DispenseFeedbackSource parseDispenseFeedbackSource(const String& value) {
    if (value == "load_cell") {
        return DispenseFeedbackSource::LoadCell;
    }
    if (value == "flow") {
        return DispenseFeedbackSource::Flow;
    }
    return DispenseFeedbackSource::Auto;
}

inline const char* dispenseFeedbackSourceToString(DispenseFeedbackSource source) {
    switch (source) {
        case DispenseFeedbackSource::LoadCell: return "load_cell";
        case DispenseFeedbackSource::Flow: return "flow";
        case DispenseFeedbackSource::Auto:
        default:
            return "auto";
    }
}

inline DispenseFeedbackOnMiss parseDispenseFeedbackOnMiss(const String& value) {
    if (value == "fault") {
        return DispenseFeedbackOnMiss::Fault;
    }
    return DispenseFeedbackOnMiss::Warn;
}

inline const char* dispenseFeedbackOnMissToString(DispenseFeedbackOnMiss value) {
    switch (value) {
        case DispenseFeedbackOnMiss::Fault: return "fault";
        case DispenseFeedbackOnMiss::Warn:
        default:
            return "warn";
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
        ReservoirSensor& reservoir,
        LoadCellSensor& loadCell,
        FlowSensor& flow
    );

    void update();
    void setReservoirEmptyPolicy(ReservoirEmptyPolicy policy);
    void setFeedbackConfig(
        DispenseFeedbackMode mode,
        DispenseFeedbackSource source,
        float tolerancePercent,
        DispenseFeedbackOnMiss onMiss,
        float densityGPerMl
    );

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
    DispenseFeedbackMode feedbackMode() const { return feedbackMode_; }
    DispenseFeedbackSource feedbackSource() const { return feedbackSource_; }

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
    bool prepareFeedbackForDispense();
    bool readFeedbackMl(float& outMl, const char*& sourceName) const;
    void pollFeedbackDuringDispense();
    void finalizeDispenseVerification();

    StepperController* stepper_ = nullptr;
    ValveController* valve_ = nullptr;
    ProfileRepository* profiles_ = nullptr;
    SafetyController* safety_ = nullptr;
    EventLogger* logger_ = nullptr;
    TmcDriverController* tmc_ = nullptr;
    ReservoirSensor* reservoir_ = nullptr;
    LoadCellSensor* loadCell_ = nullptr;
    FlowSensor* flow_ = nullptr;

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

    DispenseFeedbackMode feedbackMode_ = DispenseFeedbackMode::OpenLoop;
    DispenseFeedbackSource feedbackSource_ = DispenseFeedbackSource::Auto;
    float feedbackTolerancePercent_ = 5.0f;
    DispenseFeedbackOnMiss feedbackOnMiss_ = DispenseFeedbackOnMiss::Warn;
    float densityGPerMl_ = 1.0f;
    bool feedbackActive_ = false;
    float loadCellBaselineGrams_ = 0.0f;
    float feedbackMl_ = 0.0f;
    String feedbackSourceName_;
    bool verificationOk_ = true;
    float verificationErrorPercent_ = 0.0f;
    bool stoppedByFeedback_ = false;

    std::vector<CalibrationSample> samples_;
};

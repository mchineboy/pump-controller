#pragma once

#include <Arduino.h>
#include "models/SystemTypes.h"

struct DispenseRequest {
    String profileId;
    float requestedMl = 0.0f;
};

struct OperationStatus {
    String operationId;
    SystemState state = SystemState::Idle;
    String profileId;
    String pumpId;
    float requestedMl = 0.0f;
    float estimatedDeliveredMl = 0.0f;
    int64_t targetSteps = 0;
    int64_t completedSteps = 0;
    float progressPercent = 0.0f;
    uint32_t elapsedMs = 0;
    uint32_t estimatedRemainingMs = 0;
    String completionReason;
    FaultCode fault = FaultCode::None;
    float feedbackMl = 0.0f;
    bool feedbackAvailable = false;
    String feedbackSource;
    String feedbackMode;
    bool verificationOk = true;
    float verificationErrorPercent = 0.0f;
};

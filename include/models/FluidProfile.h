#pragma once

#include <Arduino.h>

struct CalibrationData {
    bool valid = false;
    String method = "timed_step_count";
    uint32_t requestedDurationMs = 1000;
    uint32_t actualDurationMs = 1000;
    int64_t stepCount = 0;
    float measuredMl = 0.0f;
    float stepsPerMl = 0.0f;
    float mlPerSecond = 0.0f;
    uint8_t sampleCount = 0;
    String calibratedAt;
};

struct MotorSettings {
    uint32_t speedStepsPerSecond = 1600;
    uint32_t accelerationStepsPerSecondSquared = 800;
    uint32_t decelerationStepsPerSecondSquared = 800;
    bool directionInverted = false;
    uint16_t microsteps = 16;
};

struct ValveSettings {
    bool enabled = false;
    bool activeHigh = true;
    uint32_t preOpenMs = 50;
    uint32_t postMotorCloseMs = 75;
    bool antiDripEnabled = false;
    int32_t antiDripReverseSteps = 0;
    uint32_t antiDripSpeedStepsPerSecond = 300;
};

struct DispenseLimits {
    float minimumMl = 1.0f;
    float maximumMl = 1000.0f;
};

struct FluidProfile {
    String id = "fluid_1";
    String name = "Fluid 1";
    bool enabled = true;
    bool calibrated = false;
    CalibrationData calibration;
    MotorSettings motor;
    ValveSettings valve;
    DispenseLimits limits;
};

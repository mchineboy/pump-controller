#pragma once

#include <Arduino.h>

class StepperController {
public:
    void begin(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin);

    bool startMove(
        int64_t targetSteps,
        uint32_t speedStepsPerSecond,
        uint32_t accelerationStepsPerSecondSquared,
        bool directionInverted
    );

    bool startTimedMove(
        uint32_t durationMs,
        uint32_t speedStepsPerSecond,
        uint32_t accelerationStepsPerSecondSquared,
        bool directionInverted
    );

    void stopImmediate();
    void stopControlled();
    void update();

    bool isRunning() const { return running_; }
    int64_t stepsCompleted() const { return stepsCompleted_; }
    int64_t stepsRemaining() const;
    float progress() const;
    void setEnabled(bool enabled);

private:
    void generateStepPulse();
    uint32_t intervalForCurrentSpeed() const;

    uint8_t stepPin_ = 255;
    uint8_t dirPin_ = 255;
    uint8_t enablePin_ = 255;

    bool running_ = false;
    bool timedMode_ = false;
    bool directionInverted_ = false;

    int64_t targetSteps_ = 0;
    int64_t stepsCompleted_ = 0;

    uint32_t speedStepsPerSecond_ = 0;
    uint32_t accelStepsPerSecondSquared_ = 0;
    uint32_t currentSpeedStepsPerSecond_ = 0;

    uint32_t startMs_ = 0;
    uint32_t durationMs_ = 0;
    uint32_t lastStepUs_ = 0;
    uint32_t nextStepIntervalUs_ = 0;
};

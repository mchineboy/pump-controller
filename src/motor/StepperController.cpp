#include "motor/StepperController.h"

void StepperController::begin(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin) {
    stepPin_ = stepPin;
    dirPin_ = dirPin;
    enablePin_ = enablePin;

    pinMode(stepPin_, OUTPUT);
    pinMode(dirPin_, OUTPUT);
    pinMode(enablePin_, OUTPUT);

    digitalWrite(stepPin_, LOW);
    digitalWrite(dirPin_, LOW);
    // ENABLE LOW = enabled on most TMC drivers; keep disabled at boot.
    digitalWrite(enablePin_, HIGH);
}

void StepperController::setEnabled(bool enabled) {
    // Active-low enable for typical TMC2209 wiring.
    digitalWrite(enablePin_, enabled ? LOW : HIGH);
}

bool StepperController::startMove(
    int64_t targetSteps,
    uint32_t speedStepsPerSecond,
    uint32_t accelerationStepsPerSecondSquared,
    bool directionInverted
) {
    if (running_ || targetSteps <= 0 || speedStepsPerSecond == 0) {
        return false;
    }

    timedMode_ = false;
    directionInverted_ = directionInverted;
    targetSteps_ = targetSteps;
    stepsCompleted_ = 0;
    speedStepsPerSecond_ = speedStepsPerSecond;
    accelStepsPerSecondSquared_ = accelerationStepsPerSecondSquared;
    currentSpeedStepsPerSecond_ = 0;
    startMs_ = millis();
    durationMs_ = 0;
    lastStepUs_ = micros();
    nextStepIntervalUs_ = 0;

    digitalWrite(dirPin_, directionInverted_ ? HIGH : LOW);
    setEnabled(true);
    running_ = true;
    return true;
}

bool StepperController::startTimedMove(
    uint32_t durationMs,
    uint32_t speedStepsPerSecond,
    uint32_t accelerationStepsPerSecondSquared,
    bool directionInverted
) {
    if (running_ || durationMs == 0 || speedStepsPerSecond == 0) {
        return false;
    }

    timedMode_ = true;
    directionInverted_ = directionInverted;
    targetSteps_ = 0;
    stepsCompleted_ = 0;
    speedStepsPerSecond_ = speedStepsPerSecond;
    accelStepsPerSecondSquared_ = accelerationStepsPerSecondSquared;
    currentSpeedStepsPerSecond_ = 0;
    startMs_ = millis();
    durationMs_ = durationMs;
    lastStepUs_ = micros();
    nextStepIntervalUs_ = 0;

    digitalWrite(dirPin_, directionInverted_ ? HIGH : LOW);
    setEnabled(true);
    running_ = true;
    return true;
}

void StepperController::stopImmediate() {
    running_ = false;
    digitalWrite(stepPin_, LOW);
    setEnabled(false);
}

void StepperController::stopControlled() {
    // Phase 1: controlled stop equals immediate stop.
    // Later: decelerate using profile deceleration settings.
    stopImmediate();
}

int64_t StepperController::stepsRemaining() const {
    if (timedMode_) {
        return 0;
    }
    if (stepsCompleted_ >= targetSteps_) {
        return 0;
    }
    return targetSteps_ - stepsCompleted_;
}

float StepperController::progress() const {
    if (timedMode_) {
        if (durationMs_ == 0) {
            return 0.0f;
        }
        const uint32_t elapsed = millis() - startMs_;
        if (elapsed >= durationMs_) {
            return 1.0f;
        }
        return static_cast<float>(elapsed) / static_cast<float>(durationMs_);
    }

    if (targetSteps_ <= 0) {
        return 0.0f;
    }
    return static_cast<float>(stepsCompleted_) / static_cast<float>(targetSteps_);
}

uint32_t StepperController::intervalForCurrentSpeed() const {
    if (currentSpeedStepsPerSecond_ == 0) {
        return 1000;
    }
    return 1000000UL / currentSpeedStepsPerSecond_;
}

void StepperController::generateStepPulse() {
    digitalWrite(stepPin_, HIGH);
    delayMicroseconds(2);
    digitalWrite(stepPin_, LOW);
    ++stepsCompleted_;
}

void StepperController::update() {
    if (!running_) {
        return;
    }

    if (timedMode_ && (millis() - startMs_) >= durationMs_) {
        running_ = false;
        digitalWrite(stepPin_, LOW);
        return;
    }

    if (!timedMode_ && stepsCompleted_ >= targetSteps_) {
        running_ = false;
        digitalWrite(stepPin_, LOW);
        return;
    }

    // Simple trapezoid approximation: ramp toward target speed each step window.
    if (currentSpeedStepsPerSecond_ < speedStepsPerSecond_) {
        const uint32_t increment =
            accelStepsPerSecondSquared_ == 0
                ? speedStepsPerSecond_
                : (accelStepsPerSecondSquared_ / 1000 == 0
                       ? 1u
                       : accelStepsPerSecondSquared_ / 1000);
        const uint32_t next = currentSpeedStepsPerSecond_ + increment;
        currentSpeedStepsPerSecond_ =
            next > speedStepsPerSecond_ ? speedStepsPerSecond_ : next;
    }

    const uint32_t now = micros();
    nextStepIntervalUs_ = intervalForCurrentSpeed();
    if ((now - lastStepUs_) >= nextStepIntervalUs_) {
        lastStepUs_ = now;
        generateStepPulse();
    }
}

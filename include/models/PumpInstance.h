#pragma once

#include <Arduino.h>

#include "motor/StepperController.h"
#include "valve/ValveController.h"

constexpr size_t kMaxPumpPaths = 2;
constexpr const char* kDefaultPumpId = "pump_1";
constexpr const char* kPump2Id = "pump_2";

struct PumpPinConfig {
    uint8_t stepPin = 255;
    uint8_t dirPin = 255;
    uint8_t enablePin = 255;
    uint8_t valvePin = 255;
    uint8_t tmcAddress = 0b00;
};

/** Runtime binding of one physical fluid path (STEP/DIR/EN + optional valve). */
struct PumpPath {
    String id = kDefaultPumpId;
    PumpPinConfig pins;
    StepperController* stepper = nullptr;
    ValveController* valve = nullptr;
};

inline bool isValidPumpId(const String& id) {
    return id == kDefaultPumpId || id == kPump2Id;
}

inline String normalizePumpId(const String& id) {
    if (id.isEmpty() || !isValidPumpId(id)) {
        return String(kDefaultPumpId);
    }
    return id;
}

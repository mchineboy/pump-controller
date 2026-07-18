#pragma once

#include "models/SystemTypes.h"

class SafetyController {
public:
    void begin(uint8_t estopPin, bool enabled);
    void update();

    bool isEmergencyStopActive() const { return estopActive_; }
    bool acknowledgeFault();
    FaultCode lastFault() const { return lastFault_; }
    void raiseFault(FaultCode code);

private:
    uint8_t estopPin_ = 255;
    bool enabled_ = false;
    bool estopActive_ = false;
    FaultCode lastFault_ = FaultCode::None;
};

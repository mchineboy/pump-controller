#pragma once

#include <Arduino.h>

// Compile-time pin defaults (overridable via platformio.ini build_flags).
#ifndef PUMP_STEP_PIN
#define PUMP_STEP_PIN 26
#endif
#ifndef PUMP_DIR_PIN
#define PUMP_DIR_PIN 27
#endif
#ifndef PUMP_ENABLE_PIN
#define PUMP_ENABLE_PIN 25
#endif
#ifndef PUMP_VALVE_PIN
#define PUMP_VALVE_PIN 33
#endif
#ifndef PUMP_ESTOP_PIN
#define PUMP_ESTOP_PIN 32
#endif
#ifndef PUMP_TMC_RX_PIN
#define PUMP_TMC_RX_PIN 16
#endif
#ifndef PUMP_TMC_TX_PIN
#define PUMP_TMC_TX_PIN 17
#endif
#ifndef PUMP_RESERVOIR_PIN
#define PUMP_RESERVOIR_PIN 34
#endif
#ifndef PUMP_LOADCELL_DT_PIN
#define PUMP_LOADCELL_DT_PIN 19
#endif
#ifndef PUMP_LOADCELL_SCK_PIN
#define PUMP_LOADCELL_SCK_PIN 18
#endif

namespace Config {
constexpr const char* kDeviceName = "Fluid Dispensing Controller";
constexpr const char* kHostname = "pump-controller";
constexpr const char* kApSsid = "PumpController";
constexpr const char* kApPassword = "pumpsetup";
constexpr uint32_t kDefaultSpeedStepsPerSec = 1600;
constexpr uint32_t kDefaultAccelStepsPerSec2 = 800;
constexpr uint32_t kMinSpeedStepsPerSec = 100;
constexpr uint32_t kMaxSpeedStepsPerSec = 5000;
constexpr uint32_t kMinAccelStepsPerSec2 = 100;
constexpr uint32_t kMaxAccelStepsPerSec2 = 20000;
constexpr uint32_t kDefaultCalibrationDurationMs = 1000;
constexpr int64_t kMaxOperationSteps = 5000000;
constexpr float kDefaultMinDispenseMl = 1.0f;
constexpr float kDefaultMaxDispenseMl = 1000.0f;
constexpr uint32_t kDriverDisableDelayMs = 500;
constexpr float kCalibrationCvWarningPercent = 3.0f;
constexpr int kProfilesSchemaVersion = 2;
constexpr uint32_t kMaxValveTimingMs = 5000;
constexpr uint16_t kDefaultTmcRunCurrentMa = 800;
constexpr uint16_t kDefaultTmcHoldCurrentMa = 400;
constexpr uint16_t kDefaultTmcMicrosteps = 16;
constexpr float kDefaultLoadCellScale = 1.0f;
constexpr float kDefaultFluidDensityGPerMl = 1.0f;
}  // namespace Config

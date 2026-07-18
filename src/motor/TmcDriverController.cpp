#include "motor/TmcDriverController.h"

#include <HardwareSerial.h>
#include <TMCStepper.h>

namespace {
HardwareSerial TmcSerial(2);
constexpr float kSenseResistorOhms = 0.11f;
constexpr uint8_t kDriverAddress = 0b00;
}  // namespace

bool TmcDriverController::begin(uint8_t rxPin, uint8_t txPin) {
    rxPin_ = rxPin;
    txPin_ = txPin;
    ready_ = false;
    lastError_ = "";
    overtemperature_ = false;
    shortCircuit_ = false;
    openLoad_ = false;
    lastStatus_ = 0;
    return true;
}

bool TmcDriverController::apply(const TmcDriverConfig& config) {
    config_ = config;
    ready_ = false;
    lastError_ = "";
    overtemperature_ = false;
    shortCircuit_ = false;
    openLoad_ = false;

    if (!config_.enabled) {
        return true;
    }

    TmcSerial.begin(115200, SERIAL_8N1, rxPin_, txPin_);
    delay(50);

    TMC2209Stepper driver(&TmcSerial, kSenseResistorOhms, kDriverAddress);
    driver.begin();
    driver.toff(5);
    driver.rms_current(config_.runCurrentMa, 0.5f);
    // Hold current as fraction of run current.
    const float holdFraction =
        config_.runCurrentMa == 0
            ? 0.5f
            : constrain(
                  static_cast<float>(config_.holdCurrentMa) /
                      static_cast<float>(config_.runCurrentMa),
                  0.0f,
                  1.0f
              );
    driver.rms_current(config_.runCurrentMa, holdFraction);
    driver.microsteps(config_.microsteps);
    driver.en_spreadCycle(!config_.stealthChop);
    driver.pwm_autoscale(true);

    const uint8_t version = driver.version();
    if (version == 0 || version == 0xFF) {
        lastError_ = "tmc_uart_no_response";
        ready_ = false;
        return false;
    }

    ready_ = true;
    lastError_ = "";
    refreshDiagnostics();
    return true;
}

bool TmcDriverController::refreshDiagnostics() {
    if (!config_.enabled || !ready_) {
        overtemperature_ = false;
        shortCircuit_ = false;
        openLoad_ = false;
        return false;
    }

    TMC2209Stepper driver(&TmcSerial, kSenseResistorOhms, kDriverAddress);
    lastStatus_ = driver.DRV_STATUS();
    overtemperature_ = driver.ot() || driver.otpw();
    shortCircuit_ = driver.s2ga() || driver.s2gb() || driver.s2vsa() || driver.s2vsb();
    openLoad_ = driver.ola() || driver.olb();
    return true;
}

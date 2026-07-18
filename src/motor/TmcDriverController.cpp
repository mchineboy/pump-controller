#include "motor/TmcDriverController.h"

#include <HardwareSerial.h>
#include <TMCStepper.h>

namespace {
HardwareSerial TmcSerial(2);
constexpr float kSenseResistorOhms = 0.11f;
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

bool TmcDriverController::configureDriver(
    uint8_t address,
    const TmcDriverConfig& config
) {
    TMC2209Stepper driver(&TmcSerial, kSenseResistorOhms, address);
    driver.begin();
    driver.toff(5);
    driver.rms_current(config.runCurrentMa, 0.5f);
    const float holdFraction =
        config.runCurrentMa == 0
            ? 0.5f
            : constrain(
                  static_cast<float>(config.holdCurrentMa) /
                      static_cast<float>(config.runCurrentMa),
                  0.0f,
                  1.0f
              );
    driver.rms_current(config.runCurrentMa, holdFraction);
    driver.microsteps(config.microsteps);
    driver.en_spreadCycle(!config.stealthChop);
    driver.pwm_autoscale(true);

    const uint8_t version = driver.version();
    if (version == 0 || version == 0xFF) {
        lastError_ = "tmc_uart_no_response";
        return false;
    }
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

    if (!configureDriver(config_.address, config_)) {
        ready_ = false;
        return false;
    }

    ready_ = true;
    lastError_ = "";
    refreshDiagnostics();
    return true;
}

bool TmcDriverController::applyToAddress(
    uint8_t address,
    const TmcDriverConfig& config
) {
    if (!config.enabled) {
        return true;
    }
    if (!ready_) {
        // Primary driver must succeed first so the UART bus is up.
        return false;
    }
    if (!configureDriver(address, config)) {
        return false;
    }
    return true;
}

bool TmcDriverController::readDiagnostics(uint8_t address) {
    TMC2209Stepper driver(&TmcSerial, kSenseResistorOhms, address);
    lastStatus_ = driver.DRV_STATUS();
    overtemperature_ = driver.ot() || driver.otpw();
    shortCircuit_ = driver.s2ga() || driver.s2gb() || driver.s2vsa() || driver.s2vsb();
    openLoad_ = driver.ola() || driver.olb();
    return true;
}

bool TmcDriverController::refreshDiagnostics() {
    return refreshDiagnostics(config_.address);
}

bool TmcDriverController::refreshDiagnostics(uint8_t address) {
    if (!config_.enabled || !ready_) {
        overtemperature_ = false;
        shortCircuit_ = false;
        openLoad_ = false;
        return false;
    }
    return readDiagnostics(address);
}

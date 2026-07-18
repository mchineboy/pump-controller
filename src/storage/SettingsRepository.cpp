#include "storage/SettingsRepository.h"

#include <Preferences.h>

#include "Config.h"

namespace {
Preferences prefs;
constexpr const char* kNamespace = "pump_settings";
}  // namespace

bool SettingsRepository::begin() {
    settings_.deviceName = Config::kDeviceName;
    settings_.hostname = Config::kHostname;
    settings_.wifiMode = "ap";
    settings_.emergencyStopEnabled = false;
    settings_.valveHardwarePresent = false;
    settings_.driverUartEnabled = false;
    settings_.driverRunCurrentMa = Config::kDefaultTmcRunCurrentMa;
    settings_.driverHoldCurrentMa = Config::kDefaultTmcHoldCurrentMa;
    settings_.driverMicrosteps = Config::kDefaultTmcMicrosteps;
    settings_.driverStealthChop = true;
    settings_.reservoirSensorEnabled = false;
    settings_.reservoirEmptyActiveLow = true;
    settings_.reservoirEmptyPolicy = "block";

    if (!prefs.begin(kNamespace, false)) {
        return true;
    }

    settings_.deviceName = prefs.getString("device_name", settings_.deviceName);
    settings_.hostname = prefs.getString("hostname", settings_.hostname);
    settings_.wifiMode = prefs.getString("wifi_mode", settings_.wifiMode);
    settings_.emergencyStopEnabled =
        prefs.getBool("estop_en", settings_.emergencyStopEnabled);
    settings_.driverUartEnabled =
        prefs.getBool("driver_uart", settings_.driverUartEnabled);
    settings_.driverRunCurrentMa =
        prefs.getUShort("tmc_run_ma", settings_.driverRunCurrentMa);
    settings_.driverHoldCurrentMa =
        prefs.getUShort("tmc_hold_ma", settings_.driverHoldCurrentMa);
    settings_.driverMicrosteps =
        prefs.getUShort("tmc_msteps", settings_.driverMicrosteps);
    settings_.driverStealthChop =
        prefs.getBool("tmc_stealth", settings_.driverStealthChop);
    settings_.loggingEnabled = prefs.getBool("logging", settings_.loggingEnabled);
    settings_.webAuthEnabled = prefs.getBool("web_auth", settings_.webAuthEnabled);
    settings_.valveHardwarePresent =
        prefs.getBool("valve_hw", settings_.valveHardwarePresent);
    settings_.reservoirSensorEnabled =
        prefs.getBool("res_en", settings_.reservoirSensorEnabled);
    settings_.reservoirEmptyActiveLow =
        prefs.getBool("res_empty_lo", settings_.reservoirEmptyActiveLow);
    settings_.reservoirEmptyPolicy =
        prefs.getString("res_policy", settings_.reservoirEmptyPolicy);
    prefs.end();

    // Replace earlier project brand names with the generic product name.
    if (settings_.deviceName == "Darkroom Pump Controller" ||
        settings_.deviceName == "Precision Pump Controller") {
        settings_.deviceName = Config::kDeviceName;
        save(settings_);
    }
    return true;
}

bool SettingsRepository::save(const GlobalSettings& settings) {
    settings_ = settings;
    if (!prefs.begin(kNamespace, false)) {
        return false;
    }

    prefs.putString("device_name", settings_.deviceName);
    prefs.putString("hostname", settings_.hostname);
    prefs.putString("wifi_mode", settings_.wifiMode);
    prefs.putBool("estop_en", settings_.emergencyStopEnabled);
    prefs.putBool("driver_uart", settings_.driverUartEnabled);
    prefs.putUShort("tmc_run_ma", settings_.driverRunCurrentMa);
    prefs.putUShort("tmc_hold_ma", settings_.driverHoldCurrentMa);
    prefs.putUShort("tmc_msteps", settings_.driverMicrosteps);
    prefs.putBool("tmc_stealth", settings_.driverStealthChop);
    prefs.putBool("logging", settings_.loggingEnabled);
    prefs.putBool("web_auth", settings_.webAuthEnabled);
    prefs.putBool("valve_hw", settings_.valveHardwarePresent);
    prefs.putBool("res_en", settings_.reservoirSensorEnabled);
    prefs.putBool("res_empty_lo", settings_.reservoirEmptyActiveLow);
    prefs.putString("res_policy", settings_.reservoirEmptyPolicy);
    prefs.end();
    return true;
}

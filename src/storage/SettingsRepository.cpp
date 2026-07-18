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
    settings_.pumpCount = Config::kDefaultPumpCount;
    settings_.pump2ValveHardwarePresent = false;
    settings_.driverUartEnabled = false;
    settings_.driverRunCurrentMa = Config::kDefaultTmcRunCurrentMa;
    settings_.driverHoldCurrentMa = Config::kDefaultTmcHoldCurrentMa;
    settings_.driverMicrosteps = Config::kDefaultTmcMicrosteps;
    settings_.driverStealthChop = true;
    settings_.reservoirSensorEnabled = false;
    settings_.reservoirEmptyActiveLow = true;
    settings_.reservoirEmptyPolicy = "block";
    settings_.loadCellEnabled = false;
    settings_.loadCellScale = Config::kDefaultLoadCellScale;
    settings_.loadCellOffset = 0;
    settings_.fluidDensityGPerMl = Config::kDefaultFluidDensityGPerMl;
    settings_.temperatureSensorEnabled = false;
    settings_.temperatureWarnLowC = Config::kDefaultTempWarnLowC;
    settings_.temperatureWarnHighC = Config::kDefaultTempWarnHighC;
    settings_.flowSensorEnabled = false;
    settings_.flowPulsesPerLiter = Config::kDefaultFlowPulsesPerLiter;
    settings_.dispenseFeedbackMode = "open_loop";
    settings_.dispenseFeedbackSource = "auto";
    settings_.dispenseFeedbackTolerancePercent = 5.0f;
    settings_.dispenseFeedbackOnMiss = "warn";

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
    settings_.pumpCount = static_cast<uint8_t>(
        prefs.getUChar("pump_count", settings_.pumpCount)
    );
    if (settings_.pumpCount < 1) {
        settings_.pumpCount = 1;
    }
    if (settings_.pumpCount > Config::kMaxPumpCount) {
        settings_.pumpCount = Config::kMaxPumpCount;
    }
    settings_.pump2ValveHardwarePresent =
        prefs.getBool("valve2_hw", settings_.pump2ValveHardwarePresent);
    settings_.reservoirSensorEnabled =
        prefs.getBool("res_en", settings_.reservoirSensorEnabled);
    settings_.reservoirEmptyActiveLow =
        prefs.getBool("res_empty_lo", settings_.reservoirEmptyActiveLow);
    settings_.reservoirEmptyPolicy =
        prefs.getString("res_policy", settings_.reservoirEmptyPolicy);
    settings_.loadCellEnabled = prefs.getBool("lc_en", settings_.loadCellEnabled);
    settings_.loadCellScale = prefs.getFloat("lc_scale", settings_.loadCellScale);
    settings_.loadCellOffset = prefs.getInt("lc_offset", settings_.loadCellOffset);
    settings_.fluidDensityGPerMl =
        prefs.getFloat("fluid_density", settings_.fluidDensityGPerMl);
    settings_.temperatureSensorEnabled =
        prefs.getBool("temp_en", settings_.temperatureSensorEnabled);
    settings_.temperatureWarnLowC =
        prefs.getFloat("temp_lo", settings_.temperatureWarnLowC);
    settings_.temperatureWarnHighC =
        prefs.getFloat("temp_hi", settings_.temperatureWarnHighC);
    settings_.flowSensorEnabled = prefs.getBool("flow_en", settings_.flowSensorEnabled);
    settings_.flowPulsesPerLiter =
        prefs.getFloat("flow_ppl", settings_.flowPulsesPerLiter);
    settings_.dispenseFeedbackMode =
        prefs.getString("fb_mode", settings_.dispenseFeedbackMode);
    settings_.dispenseFeedbackSource =
        prefs.getString("fb_source", settings_.dispenseFeedbackSource);
    settings_.dispenseFeedbackTolerancePercent =
        prefs.getFloat("fb_tol_pct", settings_.dispenseFeedbackTolerancePercent);
    settings_.dispenseFeedbackOnMiss =
        prefs.getString("fb_on_miss", settings_.dispenseFeedbackOnMiss);
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
    prefs.putUChar("pump_count", settings_.pumpCount);
    prefs.putBool("valve2_hw", settings_.pump2ValveHardwarePresent);
    prefs.putBool("res_en", settings_.reservoirSensorEnabled);
    prefs.putBool("res_empty_lo", settings_.reservoirEmptyActiveLow);
    prefs.putString("res_policy", settings_.reservoirEmptyPolicy);
    prefs.putBool("lc_en", settings_.loadCellEnabled);
    prefs.putFloat("lc_scale", settings_.loadCellScale);
    prefs.putInt("lc_offset", settings_.loadCellOffset);
    prefs.putFloat("fluid_density", settings_.fluidDensityGPerMl);
    prefs.putBool("temp_en", settings_.temperatureSensorEnabled);
    prefs.putFloat("temp_lo", settings_.temperatureWarnLowC);
    prefs.putFloat("temp_hi", settings_.temperatureWarnHighC);
    prefs.putBool("flow_en", settings_.flowSensorEnabled);
    prefs.putFloat("flow_ppl", settings_.flowPulsesPerLiter);
    prefs.putString("fb_mode", settings_.dispenseFeedbackMode);
    prefs.putString("fb_source", settings_.dispenseFeedbackSource);
    prefs.putFloat("fb_tol_pct", settings_.dispenseFeedbackTolerancePercent);
    prefs.putString("fb_on_miss", settings_.dispenseFeedbackOnMiss);
    prefs.end();
    return true;
}

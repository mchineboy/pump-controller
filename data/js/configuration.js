import {
  calibrateLoadCell,
  factoryReset,
  getSettings,
  tareLoadCell,
  updateSettings
} from "./api.js";

const byId = (id) => document.getElementById(id);

const fields = {
  deviceName: byId("device-name"),
  logging: byId("logging"),
  webAuth: byId("web-auth"),
  valveHw: byId("valve-hw"),
  pumpCount: byId("pump-count"),
  valve2Hw: byId("valve2-hw"),
  valve3Hw: byId("valve3-hw"),
  estopEn: byId("estop-en"),
  driverUart: byId("driver-uart"),
  tmcRunMa: byId("tmc-run-ma"),
  tmcHoldMa: byId("tmc-hold-ma"),
  tmcMicrosteps: byId("tmc-microsteps"),
  tmcStealth: byId("tmc-stealth"),
  reservoirEn: byId("reservoir-en"),
  reservoirEmptyLow: byId("reservoir-empty-low"),
  reservoirPolicy: byId("reservoir-policy"),
  loadCellEn: byId("loadcell-en"),
  fluidDensity: byId("fluid-density"),
  loadCellKnownG: byId("loadcell-known-g"),
  tempEn: byId("temp-en"),
  tempLo: byId("temp-lo"),
  tempHi: byId("temp-hi"),
  flowEn: byId("flow-en"),
  flowPpl: byId("flow-ppl"),
  feedbackMode: byId("fb-mode"),
  feedbackSource: byId("fb-source"),
  feedbackTolerance: byId("fb-tol"),
  feedbackOnMiss: byId("fb-on-miss")
};

const settingsStatus = byId("settings-status");
const pumpEffect = byId("pump-effect");
const feedbackEffect = byId("feedback-effect");
const feedbackOptions = byId("feedback-options");

function updatePumpEffect() {
  const count = Number(fields.pumpCount.value);
  document.querySelectorAll("[data-pump-min]").forEach((element) => {
    element.hidden = count < Number(element.dataset.pumpMin);
  });
  pumpEffect.textContent =
    count === 1
      ? "Profiles use Pump 1. Pump 2 and Pump 3 outputs remain disabled."
      : `${count} pump paths are available to profiles. Operations remain one-at-a-time.`;
}

function updateFeedbackEffect() {
  const mode = fields.feedbackMode.value;
  feedbackOptions.hidden = mode === "open_loop";
  if (mode === "verify_after") {
    feedbackEffect.textContent =
      "The pump runs from its calibration, then compares measured volume with the request.";
    return;
  }
  if (mode === "stop_on_feedback") {
    feedbackEffect.textContent =
      "The selected sensor controls when the pump stops. Dispensing will fault if that sensor is lost.";
    return;
  }
  feedbackEffect.textContent =
    "Dispensing uses the profile calibration only. Scale and flow readings do not change motion.";
}

function updateDependentControls() {
  fields.reservoirPolicy.disabled = !fields.reservoirEn.checked;
  fields.feedbackSource.disabled = fields.feedbackMode.value === "open_loop";
  fields.feedbackTolerance.disabled = fields.feedbackMode.value === "open_loop";
  fields.feedbackOnMiss.disabled = fields.feedbackMode.value === "open_loop";
}

async function loadSettings() {
  const settings = await getSettings();
  fields.deviceName.value = settings.device_name;
  fields.logging.checked = settings.logging_enabled;
  fields.webAuth.checked = settings.web_auth_enabled;
  fields.valveHw.checked = settings.valve_hardware_present;
  fields.pumpCount.value = String(settings.pump_count ?? 1);
  fields.valve2Hw.checked = settings.pump2_valve_hardware_present;
  fields.valve3Hw.checked = settings.pump3_valve_hardware_present;
  fields.estopEn.checked = settings.emergency_stop_enabled;
  fields.driverUart.checked = settings.driver_uart_enabled;
  fields.tmcRunMa.value = settings.driver_run_current_ma;
  fields.tmcHoldMa.value = settings.driver_hold_current_ma;
  fields.tmcMicrosteps.value = String(settings.driver_microsteps);
  fields.tmcStealth.checked = settings.driver_stealthchop;
  fields.reservoirEn.checked = settings.reservoir_sensor_enabled;
  fields.reservoirEmptyLow.checked = settings.reservoir_empty_active_low;
  fields.reservoirPolicy.value = settings.reservoir_empty_policy || "block";
  fields.loadCellEn.checked = settings.load_cell_enabled;
  fields.fluidDensity.value = settings.fluid_density_g_per_ml ?? 1;
  fields.tempEn.checked = settings.temperature_sensor_enabled;
  fields.tempLo.value = settings.temperature_warn_low_c ?? 5;
  fields.tempHi.value = settings.temperature_warn_high_c ?? 40;
  fields.flowEn.checked = settings.flow_sensor_enabled;
  fields.flowPpl.value = settings.flow_pulses_per_liter ?? 450;
  fields.feedbackMode.value = settings.dispense_feedback_mode || "open_loop";
  fields.feedbackSource.value = settings.dispense_feedback_source || "auto";
  fields.feedbackTolerance.value =
    settings.dispense_feedback_tolerance_percent ?? 5;
  fields.feedbackOnMiss.value =
    settings.dispense_feedback_on_miss || "warn";
  updatePumpEffect();
  updateFeedbackEffect();
  updateDependentControls();
}

byId("settings-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  settingsStatus.textContent = "Saving…";
  try {
    const saved = await updateSettings({
      device_name: fields.deviceName.value,
      logging_enabled: fields.logging.checked,
      web_auth_enabled: fields.webAuth.checked,
      valve_hardware_present: fields.valveHw.checked,
      pump_count: Number(fields.pumpCount.value),
      pump2_valve_hardware_present: fields.valve2Hw.checked,
      pump3_valve_hardware_present: fields.valve3Hw.checked,
      emergency_stop_enabled: fields.estopEn.checked,
      driver_uart_enabled: fields.driverUart.checked,
      driver_run_current_ma: Number(fields.tmcRunMa.value),
      driver_hold_current_ma: Number(fields.tmcHoldMa.value),
      driver_microsteps: Number(fields.tmcMicrosteps.value),
      driver_stealthchop: fields.tmcStealth.checked,
      reservoir_sensor_enabled: fields.reservoirEn.checked,
      reservoir_empty_active_low: fields.reservoirEmptyLow.checked,
      reservoir_empty_policy: fields.reservoirPolicy.value,
      load_cell_enabled: fields.loadCellEn.checked,
      fluid_density_g_per_ml: Number(fields.fluidDensity.value),
      temperature_sensor_enabled: fields.tempEn.checked,
      temperature_warn_low_c: Number(fields.tempLo.value),
      temperature_warn_high_c: Number(fields.tempHi.value),
      flow_sensor_enabled: fields.flowEn.checked,
      flow_pulses_per_liter: Number(fields.flowPpl.value),
      dispense_feedback_mode: fields.feedbackMode.value,
      dispense_feedback_source: fields.feedbackSource.value,
      dispense_feedback_tolerance_percent:
        Number(fields.feedbackTolerance.value),
      dispense_feedback_on_miss: fields.feedbackOnMiss.value
    });

    let message = "Configuration saved.";
    if (saved.driver_uart_enabled && !saved.driver_uart_ready) {
      message =
        `Saved, but motor-driver communication is not ready: ` +
        `${saved.driver_uart_error || "check wiring"}.`;
    } else if (saved.load_cell_enabled && !saved.load_cell_ready) {
      message = "Saved, but the scale is not responding. Check its wiring.";
    } else if (saved.temperature_sensor_enabled && !saved.temperature_ready) {
      message =
        "Saved, but the temperature sensor is not responding. Check its wiring.";
    }
    settingsStatus.textContent = message;
  } catch (error) {
    settingsStatus.textContent = error.message;
  }
});

fields.pumpCount.addEventListener("change", updatePumpEffect);
fields.feedbackMode.addEventListener("change", () => {
  updateFeedbackEffect();
  updateDependentControls();
});
fields.reservoirEn.addEventListener("change", updateDependentControls);

byId("loadcell-tare-btn").addEventListener("click", async () => {
  settingsStatus.textContent = "Setting the current scale load to zero…";
  try {
    await tareLoadCell();
    settingsStatus.textContent = "Scale zero set.";
  } catch (error) {
    settingsStatus.textContent = error.message;
  }
});

byId("loadcell-cal-btn").addEventListener("click", async () => {
  settingsStatus.textContent = "Calibrating scale…";
  try {
    await calibrateLoadCell(Number(fields.loadCellKnownG.value));
    settingsStatus.textContent = "Scale calibration saved.";
  } catch (error) {
    settingsStatus.textContent = error.message;
  }
});

byId("factory-reset-btn").addEventListener("click", async () => {
  const confirmInput = byId("factory-confirm");
  const factoryStatus = byId("factory-status");
  const typed = (confirmInput.value || "").trim();
  if (typed !== "FACTORY_RESET") {
    factoryStatus.textContent = "Type FACTORY_RESET exactly to continue.";
    return;
  }
  if (!confirm(
    "Erase all profiles, calibrations, and hardware settings, then restart?"
  )) {
    return;
  }
  try {
    factoryStatus.textContent = "Erasing settings and restarting…";
    await factoryReset(typed);
    factoryStatus.textContent = "Factory reset complete. Device is restarting…";
  } catch (error) {
    if (/Failed to fetch|NetworkError|abort/i.test(error.message)) {
      factoryStatus.textContent = "Reset sent. Waiting for the device to restart…";
      return;
    }
    factoryStatus.textContent = error.message;
  }
});

// Establish a stable initial layout before the settings request completes.
updatePumpEffect();
updateFeedbackEffect();
updateDependentControls();

loadSettings().catch((error) => {
  settingsStatus.textContent = error.message;
});

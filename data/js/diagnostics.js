import {
  acknowledgeFault,
  calibrateLoadCell,
  clearEventLog,
  getEventLog,
  getSettings,
  getStatus,
  injectDriverFault,
  resetFlowCumulative,
  tareLoadCell,
  updateSettings
} from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const deviceName = document.getElementById("device-name");
const logging = document.getElementById("logging");
const webAuth = document.getElementById("web-auth");
const valveHw = document.getElementById("valve-hw");
const pumpCount = document.getElementById("pump-count");
const valve2Hw = document.getElementById("valve2-hw");
const estopEn = document.getElementById("estop-en");
const driverUart = document.getElementById("driver-uart");
const tmcRunMa = document.getElementById("tmc-run-ma");
const tmcHoldMa = document.getElementById("tmc-hold-ma");
const tmcMicrosteps = document.getElementById("tmc-microsteps");
const tmcStealth = document.getElementById("tmc-stealth");
const reservoirEn = document.getElementById("reservoir-en");
const reservoirEmptyLow = document.getElementById("reservoir-empty-low");
const reservoirPolicy = document.getElementById("reservoir-policy");
const loadCellEn = document.getElementById("loadcell-en");
const fluidDensity = document.getElementById("fluid-density");
const loadCellKnownG = document.getElementById("loadcell-known-g");
const settingsStatus = document.getElementById("settings-status");
const estopLive = document.getElementById("estop-live");
const tmcLive = document.getElementById("tmc-live");
const tmcDiag = document.getElementById("tmc-diag");
const reservoirLive = document.getElementById("reservoir-live");
const loadCellLive = document.getElementById("loadcell-live");
const tempEn = document.getElementById("temp-en");
const tempLo = document.getElementById("temp-lo");
const tempHi = document.getElementById("temp-hi");
const tempLive = document.getElementById("temp-live");
const flowEn = document.getElementById("flow-en");
const flowPpl = document.getElementById("flow-ppl");
const flowLive = document.getElementById("flow-live");
const fbMode = document.getElementById("fb-mode");
const fbSource = document.getElementById("fb-source");
const fbTol = document.getElementById("fb-tol");
const fbOnMiss = document.getElementById("fb-on-miss");
const eventList = document.getElementById("event-list");

function renderEstop(status) {
  const enabled = status.estop_enabled ? "enabled" : "disabled";
  const active = status.estop_active ? "ASSERTED" : "clear";
  estopLive.textContent =
    `ESTOP: ${enabled} · input ${active}` +
    (status.fault ? ` · fault: ${status.fault}` : "");
}

function renderTmc(status) {
  if (!status.driver_uart_enabled) {
    tmcLive.textContent = "TMC UART: disabled (STEP/DIR only)";
    tmcDiag.textContent = "Driver flags: n/a (UART disabled)";
    return;
  }
  const ready = status.driver_uart_ready ? "ready" : "not ready";
  tmcLive.textContent =
    `TMC UART: enabled · ${ready}` +
    (status.driver_uart_error ? ` · ${status.driver_uart_error}` : "");
  const flags = [];
  if (status.driver_overtemperature) flags.push("OT");
  if (status.driver_short_circuit) flags.push("short");
  if (status.driver_open_load) flags.push("open-load");
  tmcDiag.textContent = flags.length
    ? `Driver flags: ${flags.join(", ")}`
    : "Driver flags: clear";
}

function renderReservoir(status) {
  if (!status.reservoir_sensor_enabled) {
    reservoirLive.textContent = "Reservoir: sensor disabled";
    return;
  }
  const level = status.reservoir_empty ? "EMPTY" : "ok";
  const warn = status.reservoir_empty_warning ? " · warn" : "";
  reservoirLive.textContent =
    `Reservoir: ${level} · policy ${status.reservoir_empty_policy || "—"}${warn}`;
}

function renderLoadCell(status) {
  if (!status.load_cell_enabled) {
    loadCellLive.textContent = "Load cell: disabled";
    return;
  }
  const ready = status.load_cell_ready ? "ready" : "not ready";
  const grams = status.load_cell_grams == null
    ? "—"
    : `${Number(status.load_cell_grams).toFixed(2)} g`;
  const ml = status.load_cell_ml == null
    ? ""
    : ` · ${Number(status.load_cell_ml).toFixed(2)} mL`;
  loadCellLive.textContent =
    `Load cell: ${ready} · ${grams}${ml}` +
    (status.load_cell_error ? ` · ${status.load_cell_error}` : "");
}

function renderTemperature(status) {
  if (!status.temperature_sensor_enabled) {
    tempLive.textContent = "Temperature: disabled";
    return;
  }
  const ready = status.temperature_ready ? "ready" : "not ready";
  const c = status.temperature_c == null
    ? "—"
    : `${Number(status.temperature_c).toFixed(2)} °C`;
  const warns = [];
  if (status.temperature_warn_low) warns.push("low");
  if (status.temperature_warn_high) warns.push("high");
  tempLive.textContent =
    `Temperature: ${ready} · ${c}` +
    (warns.length ? ` · warn ${warns.join("/")}` : "") +
    (status.temperature_error ? ` · ${status.temperature_error}` : "");
}

function renderFlow(status) {
  if (!status.flow_sensor_enabled) {
    flowLive.textContent = "Flow: disabled";
    return;
  }
  const rate = status.flow_ml_per_min == null
    ? "—"
    : `${Number(status.flow_ml_per_min).toFixed(1)} mL/min`;
  const cum = status.flow_cumulative_ml == null
    ? "—"
    : `${Number(status.flow_cumulative_ml).toFixed(1)} mL`;
  flowLive.textContent = `Flow: ${rate} · cumulative ${cum}`;
}

function renderStatus(status) {
  renderEstop(status);
  renderTmc(status);
  renderReservoir(status);
  renderLoadCell(status);
  renderTemperature(status);
  renderFlow(status);
}

async function loadSettings() {
  const settings = await getSettings();
  deviceName.value = settings.device_name;
  logging.checked = settings.logging_enabled;
  webAuth.checked = settings.web_auth_enabled;
  valveHw.checked = settings.valve_hardware_present;
  pumpCount.value = String(settings.pump_count ?? 1);
  valve2Hw.checked = settings.pump2_valve_hardware_present;
  estopEn.checked = settings.emergency_stop_enabled;
  driverUart.checked = settings.driver_uart_enabled;
  tmcRunMa.value = settings.driver_run_current_ma;
  tmcHoldMa.value = settings.driver_hold_current_ma;
  tmcMicrosteps.value = String(settings.driver_microsteps);
  tmcStealth.checked = settings.driver_stealthchop;
  reservoirEn.checked = settings.reservoir_sensor_enabled;
  reservoirEmptyLow.checked = settings.reservoir_empty_active_low;
  reservoirPolicy.value = settings.reservoir_empty_policy || "block";
  loadCellEn.checked = settings.load_cell_enabled;
  fluidDensity.value = settings.fluid_density_g_per_ml ?? 1;
  tempEn.checked = settings.temperature_sensor_enabled;
  tempLo.value = settings.temperature_warn_low_c ?? 5;
  tempHi.value = settings.temperature_warn_high_c ?? 40;
  flowEn.checked = settings.flow_sensor_enabled;
  flowPpl.value = settings.flow_pulses_per_liter ?? 450;
  fbMode.value = settings.dispense_feedback_mode || "open_loop";
  fbSource.value = settings.dispense_feedback_source || "auto";
  fbTol.value = settings.dispense_feedback_tolerance_percent ?? 5;
  fbOnMiss.value = settings.dispense_feedback_on_miss || "warn";
}

async function loadEvents() {
  const events = await getEventLog();
  eventList.innerHTML = "";
  if (!events.length) {
    eventList.innerHTML = "<li>No events logged.</li>";
    return;
  }
  events.slice().reverse().forEach((entry) => {
    const item = document.createElement("li");
    item.textContent = `${entry.timestamp}: ${entry.event}` +
      (entry.profile_id ? ` [${entry.profile_id}]` : "");
    eventList.appendChild(item);
  });
}

document.getElementById("settings-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    const saved = await updateSettings({
      device_name: deviceName.value,
      logging_enabled: logging.checked,
      web_auth_enabled: webAuth.checked,
      valve_hardware_present: valveHw.checked,
      pump_count: Number(pumpCount.value),
      pump2_valve_hardware_present: valve2Hw.checked,
      emergency_stop_enabled: estopEn.checked,
      driver_uart_enabled: driverUart.checked,
      driver_run_current_ma: Number(tmcRunMa.value),
      driver_hold_current_ma: Number(tmcHoldMa.value),
      driver_microsteps: Number(tmcMicrosteps.value),
      driver_stealthchop: tmcStealth.checked,
      reservoir_sensor_enabled: reservoirEn.checked,
      reservoir_empty_active_low: reservoirEmptyLow.checked,
      reservoir_empty_policy: reservoirPolicy.value,
      load_cell_enabled: loadCellEn.checked,
      fluid_density_g_per_ml: Number(fluidDensity.value),
      temperature_sensor_enabled: tempEn.checked,
      temperature_warn_low_c: Number(tempLo.value),
      temperature_warn_high_c: Number(tempHi.value),
      flow_sensor_enabled: flowEn.checked,
      flow_pulses_per_liter: Number(flowPpl.value),
      dispense_feedback_mode: fbMode.value,
      dispense_feedback_source: fbSource.value,
      dispense_feedback_tolerance_percent: Number(fbTol.value),
      dispense_feedback_on_miss: fbOnMiss.value
    });
    let message = "Settings saved.";
    if (saved.driver_uart_enabled && !saved.driver_uart_ready) {
      message = `Settings saved. UART warning: ${saved.driver_uart_error || "not ready"}`;
    } else if (saved.load_cell_enabled && !saved.load_cell_ready) {
      message = "Settings saved. Load cell not ready (check wiring).";
    } else if (saved.temperature_sensor_enabled && !saved.temperature_ready) {
      message = "Settings saved. Temperature sensor not ready (check wiring).";
    }
    settingsStatus.textContent = message;
    renderStatus(await getStatus());
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("flow-reset-btn").addEventListener("click", async () => {
  try {
    await resetFlowCumulative();
    settingsStatus.textContent = "Flow cumulative reset.";
    renderStatus(await getStatus());
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("loadcell-tare-btn").addEventListener("click", async () => {
  try {
    await tareLoadCell();
    settingsStatus.textContent = "Load cell tared.";
    renderStatus(await getStatus());
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("loadcell-cal-btn").addEventListener("click", async () => {
  try {
    await calibrateLoadCell(Number(loadCellKnownG.value));
    settingsStatus.textContent = "Load cell calibrated.";
    renderStatus(await getStatus());
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("inject-driver-fault-btn").addEventListener("click", async () => {
  try {
    await injectDriverFault();
    renderStatus(await getStatus());
    await loadEvents();
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("ack-fault-btn").addEventListener("click", async () => {
  try {
    await acknowledgeFault();
    renderStatus(await getStatus());
    await loadEvents();
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("refresh-log-btn").addEventListener("click", () => {
  loadEvents().catch((error) => alert(error.message));
});

document.getElementById("clear-log-btn").addEventListener("click", async () => {
  if (!confirm("Clear the event log?")) {
    return;
  }
  try {
    await clearEventLog();
    await loadEvents();
  } catch (error) {
    alert(error.message);
  }
});

connectStatusStream({
  onStatus: renderStatus,
  onOperation: () => {
    getStatus().then(renderStatus).catch(() => {});
  }
});

Promise.all([loadSettings(), loadEvents(), getStatus().then(renderStatus)])
  .catch((error) => alert(error.message));

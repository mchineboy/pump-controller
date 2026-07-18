import {
  clearEventLog,
  getEventLog,
  getSettings,
  getStatus,
  updateSettings
} from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const deviceName = document.getElementById("device-name");
const logging = document.getElementById("logging");
const webAuth = document.getElementById("web-auth");
const valveHw = document.getElementById("valve-hw");
const estopEn = document.getElementById("estop-en");
const driverUart = document.getElementById("driver-uart");
const tmcRunMa = document.getElementById("tmc-run-ma");
const tmcHoldMa = document.getElementById("tmc-hold-ma");
const tmcMicrosteps = document.getElementById("tmc-microsteps");
const tmcStealth = document.getElementById("tmc-stealth");
const settingsStatus = document.getElementById("settings-status");
const estopLive = document.getElementById("estop-live");
const tmcLive = document.getElementById("tmc-live");
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
    return;
  }
  const ready = status.driver_uart_ready ? "ready" : "not ready";
  tmcLive.textContent =
    `TMC UART: enabled · ${ready}` +
    (status.driver_uart_error ? ` · ${status.driver_uart_error}` : "");
}

function renderStatus(status) {
  renderEstop(status);
  renderTmc(status);
}

async function loadSettings() {
  const settings = await getSettings();
  deviceName.value = settings.device_name;
  logging.checked = settings.logging_enabled;
  webAuth.checked = settings.web_auth_enabled;
  valveHw.checked = settings.valve_hardware_present;
  estopEn.checked = settings.emergency_stop_enabled;
  driverUart.checked = settings.driver_uart_enabled;
  tmcRunMa.value = settings.driver_run_current_ma;
  tmcHoldMa.value = settings.driver_hold_current_ma;
  tmcMicrosteps.value = String(settings.driver_microsteps);
  tmcStealth.checked = settings.driver_stealthchop;
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
      emergency_stop_enabled: estopEn.checked,
      driver_uart_enabled: driverUart.checked,
      driver_run_current_ma: Number(tmcRunMa.value),
      driver_hold_current_ma: Number(tmcHoldMa.value),
      driver_microsteps: Number(tmcMicrosteps.value),
      driver_stealthchop: tmcStealth.checked
    });
    settingsStatus.textContent = saved.driver_uart_enabled && !saved.driver_uart_ready
      ? `Settings saved. UART warning: ${saved.driver_uart_error || "not ready"}`
      : "Settings saved.";
    renderStatus(await getStatus());
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

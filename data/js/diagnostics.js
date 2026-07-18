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
const settingsStatus = document.getElementById("settings-status");
const estopLive = document.getElementById("estop-live");
const eventList = document.getElementById("event-list");

function renderEstop(status) {
  const enabled = status.estop_enabled ? "enabled" : "disabled";
  const active = status.estop_active ? "ASSERTED" : "clear";
  estopLive.textContent =
    `ESTOP: ${enabled} · input ${active}` +
    (status.fault ? ` · fault: ${status.fault}` : "");
}

async function loadSettings() {
  const settings = await getSettings();
  deviceName.value = settings.device_name;
  logging.checked = settings.logging_enabled;
  webAuth.checked = settings.web_auth_enabled;
  valveHw.checked = settings.valve_hardware_present;
  estopEn.checked = settings.emergency_stop_enabled;
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
    await updateSettings({
      device_name: deviceName.value,
      logging_enabled: logging.checked,
      web_auth_enabled: webAuth.checked,
      valve_hardware_present: valveHw.checked,
      emergency_stop_enabled: estopEn.checked
    });
    settingsStatus.textContent = "Settings saved.";
    renderEstop(await getStatus());
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
  onStatus: renderEstop,
  onOperation: () => {
    getStatus().then(renderEstop).catch(() => {});
  }
});

Promise.all([loadSettings(), loadEvents(), getStatus().then(renderEstop)])
  .catch((error) => alert(error.message));

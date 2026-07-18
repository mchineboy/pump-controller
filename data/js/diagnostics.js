import {
  acknowledgeFault,
  clearEventLog,
  getEventLog,
  getStatus,
  injectDriverFault,
  resetFlowCumulative
} from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const byId = (id) => document.getElementById(id);
const systemLive = byId("system-live");
const estopLive = byId("estop-live");
const tmcLive = byId("tmc-live");
const tmcDiag = byId("tmc-diag");
const reservoirLive = byId("reservoir-live");
const loadCellLive = byId("loadcell-live");
const tempLive = byId("temp-live");
const flowLive = byId("flow-live");
const eventList = byId("event-list");

function setState(element, state) {
  element.dataset.state = state;
}

function renderSystem(status) {
  const activePump = status.active_pump ? ` · ${status.active_pump}` : "";
  const fault = status.fault ? ` · fault: ${status.fault}` : "";
  systemLive.textContent =
    `System: ${status.state} · ${status.pump_count || 1} pump(s)${activePump}${fault}`;
  setState(systemLive, status.fault ? "fault" : status.state === "idle" ? "ok" : "active");
}

function renderEstop(status) {
  if (!status.estop_enabled) {
    estopLive.textContent = "Emergency stop: not configured";
    setState(estopLive, "disabled");
    return;
  }
  estopLive.textContent =
    `Emergency stop: ${status.estop_active ? "ASSERTED" : "armed and clear"}`;
  setState(estopLive, status.estop_active ? "fault" : "ok");
}

function renderTmc(status) {
  if (!status.driver_uart_enabled) {
    tmcLive.textContent = "Motor drivers: STEP/DIR mode (UART diagnostics off)";
    tmcDiag.textContent = "Driver flags: unavailable";
    setState(tmcLive, "disabled");
    setState(tmcDiag, "disabled");
    return;
  }
  const ready = status.driver_uart_ready ? "communicating" : "not responding";
  tmcLive.textContent =
    `Motor drivers: ${ready}` +
    (status.driver_uart_error ? ` · ${status.driver_uart_error}` : "");
  setState(tmcLive, status.driver_uart_ready ? "ok" : "fault");

  const flags = [];
  if (status.driver_overtemperature) flags.push("overtemperature");
  if (status.driver_short_circuit) flags.push("short circuit");
  if (status.driver_open_load) flags.push("open load");
  tmcDiag.textContent =
    flags.length ? `Driver flags: ${flags.join(", ")}` : "Driver flags: clear";
  setState(tmcDiag, flags.length ? "fault" : "ok");
}

function renderReservoir(status) {
  if (!status.reservoir_sensor_enabled) {
    reservoirLive.textContent = "Source container: sensor not configured";
    setState(reservoirLive, "disabled");
    return;
  }
  reservoirLive.textContent =
    `Source container: ${status.reservoir_empty ? "EMPTY" : "level OK"} · ` +
    `action ${status.reservoir_empty_policy || "—"}`;
  setState(reservoirLive, status.reservoir_empty ? "fault" : "ok");
}

function renderLoadCell(status) {
  if (!status.load_cell_enabled) {
    loadCellLive.textContent = "Scale: not configured";
    setState(loadCellLive, "disabled");
    return;
  }
  const grams =
    status.load_cell_grams == null
      ? "—"
      : `${Number(status.load_cell_grams).toFixed(2)} g`;
  const ml =
    status.load_cell_ml == null
      ? ""
      : ` · ${Number(status.load_cell_ml).toFixed(2)} mL estimate`;
  loadCellLive.textContent =
    `Scale: ${status.load_cell_ready ? "ready" : "not responding"} · ` +
    `${grams}${ml}` +
    (status.load_cell_error ? ` · ${status.load_cell_error}` : "");
  setState(loadCellLive, status.load_cell_ready ? "ok" : "fault");
}

function renderTemperature(status) {
  if (!status.temperature_sensor_enabled) {
    tempLive.textContent = "Temperature: sensor not configured";
    setState(tempLive, "disabled");
    return;
  }
  const value =
    status.temperature_c == null
      ? "—"
      : `${Number(status.temperature_c).toFixed(2)} °C`;
  const warnings = [];
  if (status.temperature_warn_low) warnings.push("below limit");
  if (status.temperature_warn_high) warnings.push("above limit");
  tempLive.textContent =
    `Temperature: ${status.temperature_ready ? value : "not responding"}` +
    (warnings.length ? ` · ${warnings.join(", ")}` : "") +
    (status.temperature_error ? ` · ${status.temperature_error}` : "");
  setState(
    tempLive,
    !status.temperature_ready || warnings.length ? "fault" : "ok"
  );
}

function renderFlow(status) {
  if (!status.flow_sensor_enabled) {
    flowLive.textContent = "Flow: sensor not configured";
    setState(flowLive, "disabled");
    return;
  }
  const rate =
    status.flow_ml_per_min == null
      ? "—"
      : `${Number(status.flow_ml_per_min).toFixed(1)} mL/min`;
  const cumulative =
    status.flow_cumulative_ml == null
      ? "—"
      : `${Number(status.flow_cumulative_ml).toFixed(1)} mL total`;
  flowLive.textContent = `Flow: ${rate} · ${cumulative}`;
  setState(flowLive, status.flow_ready === false ? "fault" : "ok");
}

function renderStatus(status) {
  renderSystem(status);
  renderEstop(status);
  renderTmc(status);
  renderReservoir(status);
  renderLoadCell(status);
  renderTemperature(status);
  renderFlow(status);
}

async function refreshStatus() {
  renderStatus(await getStatus());
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
    const context = [
      entry.pump_id,
      entry.profile_id
    ].filter(Boolean).join(" / ");
    item.textContent =
      `${entry.timestamp}: ${entry.event}` + (context ? ` [${context}]` : "");
    eventList.appendChild(item);
  });
}

byId("refresh-status-btn").addEventListener("click", () => {
  refreshStatus().catch((error) => {
    systemLive.textContent = error.message;
    setState(systemLive, "fault");
  });
});

byId("flow-reset-btn").addEventListener("click", async () => {
  const message = byId("flow-action-status");
  try {
    await resetFlowCumulative();
    message.textContent = "Accumulated flow volume reset.";
    await refreshStatus();
  } catch (error) {
    message.textContent = error.message;
  }
});

byId("inject-driver-fault-btn").addEventListener("click", async () => {
  const message = byId("fault-action-status");
  if (!confirm("Simulate a motor-driver fault and verify that motion stops?")) {
    return;
  }
  try {
    await injectDriverFault();
    message.textContent = "Simulated fault raised.";
    await Promise.all([refreshStatus(), loadEvents()]);
  } catch (error) {
    message.textContent = error.message;
  }
});

byId("ack-fault-btn").addEventListener("click", async () => {
  const message = byId("fault-action-status");
  try {
    await acknowledgeFault();
    message.textContent = "Fault acknowledged.";
    await Promise.all([refreshStatus(), loadEvents()]);
  } catch (error) {
    message.textContent = error.message;
  }
});

byId("refresh-log-btn").addEventListener("click", () => {
  loadEvents().catch((error) => {
    eventList.innerHTML = `<li>${error.message}</li>`;
  });
});

byId("clear-log-btn").addEventListener("click", async () => {
  if (!confirm("Clear the diagnostic event log?")) {
    return;
  }
  try {
    await clearEventLog();
    await loadEvents();
  } catch (error) {
    eventList.innerHTML = `<li>${error.message}</li>`;
  }
});

connectStatusStream({
  onStatus: renderStatus,
  onOperation: () => refreshStatus().catch(() => {})
});

Promise.all([refreshStatus(), loadEvents()]).catch((error) => {
  systemLive.textContent = error.message;
  setState(systemLive, "fault");
});

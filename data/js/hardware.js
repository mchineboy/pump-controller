import { getStatus } from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const liveStatus = document.getElementById("live-status");

function render(status) {
  const pump =
    status.pump_count >= 2
      ? ` · pumps ${status.pump_count}` +
        (status.active_pump ? ` · active ${status.active_pump}` : "")
      : "";
  liveStatus.textContent =
    `State: ${status.state} · motor ${status.motor_running ? "running" : "idle"} · ` +
    `valve ${status.valve_open ? "open" : "closed"} · ` +
    `ESTOP ${status.estop_enabled ? (status.estop_active ? "ASSERTED" : "armed") : "off"}` +
    pump +
    (status.fault ? ` · fault: ${status.fault}` : "");
}

connectStatusStream({
  onStatus: render,
  onOperation: (operation) => {
    liveStatus.textContent = `Operation: ${operation.state} (${operation.operation_id || "none"})`;
  }
});

getStatus().then(render).catch((error) => {
  liveStatus.textContent = error.message;
});

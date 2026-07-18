import { getStatus } from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const liveStatus = document.getElementById("live-status");

function render(status) {
  liveStatus.textContent =
    `State: ${status.state} · motor ${status.motor_running ? "running" : "idle"} · ` +
    `valve ${status.valve_open ? "open" : "closed"}` +
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

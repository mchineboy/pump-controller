import {
  acknowledgeFault,
  fillProfileSelect,
  getOperation,
  getProfile,
  getStatus,
  startDispense,
  stopOperation
} from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const stateEl = document.getElementById("system-state");
const reservoirStatusEl = document.getElementById("reservoir-status");
const estimateEl = document.getElementById("estimate");
const calibrationMetaEl = document.getElementById("calibration-meta");
const progressPanel = document.getElementById("progress-panel");
const progressLabel = document.getElementById("progress-label");
const progressBar = document.getElementById("progress-bar");
const progressDetail = document.getElementById("progress-detail");
const faultPanel = document.getElementById("fault-panel");
const faultMessage = document.getElementById("fault-message");
const form = document.getElementById("dispense-form");
const volumeInput = document.getElementById("volume");
const profileSelect = document.getElementById("profile");

let profile = null;

function formatSeconds(ms) {
  if (!ms || ms <= 0) {
    return "—";
  }
  return `${(ms / 1000).toFixed(1)} seconds`;
}

function updateEstimate() {
  if (!profile?.calibrated || !profile.calibration?.steps_per_ml) {
    estimateEl.textContent = "Estimated duration: unavailable (uncalibrated)";
    calibrationMetaEl.textContent = "Calibration: not calibrated";
    return;
  }

  const volume = Number(volumeInput.value);
  const steps = volume * profile.calibration.steps_per_ml;
  const seconds = steps / profile.motor.speed_steps_per_second;
  estimateEl.textContent = `Estimated duration: ${seconds.toFixed(1)} seconds`;
  calibrationMetaEl.textContent =
    `Calibration: ${profile.calibration.steps_per_ml.toFixed(2)} steps/mL @ ` +
    `${profile.motor.speed_steps_per_second} steps/s`;
}

function renderOperation(operation) {
  stateEl.textContent = `State: ${operation.state}`;

  const active =
    operation.state === "dispensing" ||
    operation.state === "calibrating" ||
    operation.state === "stopping";

  progressPanel.hidden = !active && operation.state !== "completed";
  if (!progressPanel.hidden) {
    progressLabel.textContent =
      operation.state === "completed"
        ? "Dispense completed"
        : `Dispensing ${operation.profile_id || ""}`.trim();
    progressBar.value = operation.progress_percent || 0;
    progressDetail.textContent = [
      `Target: ${operation.requested_ml ?? "—"} mL`,
      `Delivered ≈ ${Number(operation.estimated_delivered_ml || 0).toFixed(1)} mL`,
      `Progress: ${Number(operation.progress_percent || 0).toFixed(1)}%`,
      `Remaining: ${formatSeconds(operation.estimated_remaining_ms)}`
    ].join(" · ");
  }
}

function renderStatus(status) {
  stateEl.textContent = `State: ${status.state}`;
  if (status.fault) {
    faultPanel.hidden = false;
    faultMessage.textContent = status.fault;
  } else {
    faultPanel.hidden = true;
  }

  if (status.reservoir_sensor_enabled && status.reservoir_empty) {
    reservoirStatusEl.hidden = false;
    reservoirStatusEl.textContent = status.reservoir_empty_warning
      ? "Reservoir empty (warning only — dispense still allowed)"
      : "Reservoir empty — new dispense blocked until refilled";
  } else {
    reservoirStatusEl.hidden = true;
  }
}

async function refreshProfile() {
  profile = await getProfile(profileSelect.value);
  volumeInput.min = profile.limits.minimum_ml;
  volumeInput.max = profile.limits.maximum_ml;
  updateEstimate();
}

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    await startDispense(profileSelect.value, Number(volumeInput.value));
    const operation = await getOperation();
    renderOperation(operation);
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("stop-btn").addEventListener("click", async () => {
  try {
    await stopOperation();
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("ack-fault-btn").addEventListener("click", async () => {
  try {
    await acknowledgeFault();
    faultPanel.hidden = true;
  } catch (error) {
    alert(error.message);
  }
});

volumeInput.addEventListener("input", updateEstimate);
profileSelect.addEventListener("change", () => {
  refreshProfile().catch((error) => alert(error.message));
});

connectStatusStream({
  onOperation: renderOperation,
  onStatus: renderStatus
});

fillProfileSelect(profileSelect)
  .then(() => refreshProfile())
  .then(() => getStatus())
  .then(renderStatus)
  .catch((error) => {
    stateEl.textContent = `Error: ${error.message}`;
  });

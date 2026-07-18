import {
  discardCalibration,
  fillProfileSelect,
  getCalibrationHistory,
  getProfile,
  removeCalibrationSample,
  saveCalibration,
  startCalibration,
  submitMeasurement,
  updateProfile
} from "./api.js";
import { connectStatusStream } from "./status-stream.js";

const profileSelect = document.getElementById("profile");
const durationSelect = document.getElementById("duration");
const confirmDialog = document.getElementById("confirm-dialog");
const confirmText = document.getElementById("confirm-text");
const measurementPanel = document.getElementById("measurement-panel");
const measurementHint = document.getElementById("measurement-hint");
const samplesPanel = document.getElementById("samples-panel");
const sampleList = document.getElementById("sample-list");
const sampleStats = document.getElementById("sample-stats");
const historyList = document.getElementById("history-list");
const speedInput = document.getElementById("speed");
const accelInput = document.getElementById("accel");

let samples = [];
let lastResult = null;

function renderSummary(profile) {
  document.getElementById("cal-status").textContent =
    profile.calibrated ? "Calibrated" : "Not calibrated";
  document.getElementById("cal-steps").textContent = profile.calibrated
    ? profile.calibration.steps_per_ml.toFixed(2)
    : "—";
  document.getElementById("cal-rate").textContent = profile.calibrated
    ? profile.calibration.ml_per_second.toFixed(2)
    : "—";
  document.getElementById("cal-speed").textContent =
    `${profile.motor.speed_steps_per_second} steps/second`;
  speedInput.value = profile.motor.speed_steps_per_second;
  accelInput.value = profile.motor.acceleration_steps_per_second_squared;
}

function renderSamples(result) {
  lastResult = result;
  samplesPanel.hidden = samples.length === 0;
  sampleList.innerHTML = "";
  samples.forEach((ml, index) => {
    const item = document.createElement("li");
    const remove = document.createElement("button");
    remove.type = "button";
    remove.textContent = "Remove";
    remove.className = "danger";
    remove.addEventListener("click", async () => {
      try {
        const next = await removeCalibrationSample(index);
        samples.splice(index, 1);
        renderSamples(next);
      } catch (error) {
        alert(error.message);
      }
    });
    item.textContent = `Run ${index + 1}: ${ml.toFixed(2)} mL `;
    item.appendChild(remove);
    sampleList.appendChild(item);
  });

  if (!result || !result.sample_count) {
    sampleStats.textContent = "";
    return;
  }

  const warning =
    result.variation_warning || result.coefficient_of_variation > 3
      ? ` · Warning: variation ${Number(result.coefficient_of_variation).toFixed(1)}%`
      : "";
  sampleStats.textContent =
    `Average: ${Number(result.average_ml).toFixed(2)} mL · ` +
    `${Number(result.steps_per_ml).toFixed(2)} steps/mL` +
    (result.sample_count >= 3 ? "" : " · recommend ≥ 3 samples") +
    warning;
}

async function refreshHistory() {
  const history = await getCalibrationHistory(profileSelect.value);
  historyList.innerHTML = "";
  if (!history.entries?.length) {
    historyList.innerHTML = "<li>No calibration history yet.</li>";
    return;
  }
  history.entries.slice().reverse().forEach((entry) => {
    const item = document.createElement("li");
    item.textContent =
      `${entry.timestamp}: ${Number(entry.measured_ml).toFixed(2)} mL → ` +
      `${Number(entry.steps_per_ml).toFixed(2)} steps/mL`;
    historyList.appendChild(item);
  });
}

async function refreshProfile() {
  const profile = await getProfile(profileSelect.value);
  renderSummary(profile);
  await refreshHistory();
}

document.getElementById("motor-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    const profile = await updateProfile(profileSelect.value, {
      motor: {
        speed_steps_per_second: Number(speedInput.value),
        acceleration_steps_per_second_squared: Number(accelInput.value),
        deceleration_steps_per_second_squared: Number(accelInput.value)
      }
    });
    renderSummary(profile);
    if (!profile.calibrated) {
      alert("Speed changed. Recalibration is required before dispensing.");
    }
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("calibration-form").addEventListener("submit", (event) => {
  event.preventDefault();
  const seconds = Number(durationSelect.value) / 1000;
  confirmText.textContent =
    `This will run the selected pump for ${seconds.toFixed(3)} second(s).`;
  confirmDialog.showModal();
});

document.getElementById("confirm-form").addEventListener("close", async () => {
  if (confirmDialog.returnValue !== "confirm") {
    return;
  }
  try {
    await startCalibration(profileSelect.value, Number(durationSelect.value));
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("measurement-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  const measured = Number(document.getElementById("measured-ml").value);
  try {
    const result = await submitMeasurement(measured);
    samples.push(measured);
    measurementPanel.hidden = true;
    renderSamples(result);
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("discard-btn").addEventListener("click", async () => {
  await discardCalibration();
  measurementPanel.hidden = true;
});

document.getElementById("save-cal-btn").addEventListener("click", async () => {
  try {
    await saveCalibration();
    samples = [];
    renderSamples(null);
    await refreshProfile();
    alert("Calibration saved.");
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("repeat-btn").addEventListener("click", () => {
  const seconds = Number(durationSelect.value) / 1000;
  confirmText.textContent =
    `This will run the selected pump for ${seconds.toFixed(3)} second(s).`;
  confirmDialog.showModal();
});

profileSelect.addEventListener("change", () => {
  samples = [];
  renderSamples(null);
  refreshProfile().catch((error) => alert(error.message));
});

connectStatusStream({
  onOperation: (operation) => {
    if (operation.state === "awaiting_calibration_measurement") {
      measurementPanel.hidden = false;
      measurementHint.textContent =
        `Motion complete · ${operation.completed_steps || 0} steps · ` +
        `${((operation.elapsed_ms || 0) / 1000).toFixed(3)} s`;
    }
  }
});

fillProfileSelect(profileSelect)
  .then(() => refreshProfile())
  .catch((error) => alert(error.message));

import {
  exportConfig,
  fillProfileSelect,
  getProfile,
  getSettings,
  importConfig,
  resetCalibration,
  updateProfile
} from "./api.js";

const profileSelect = document.getElementById("profile");
const nameInput = document.getElementById("name");
const pumpIdSelect = document.getElementById("pump-id");
const pumpCountNote = document.getElementById("pump-count-note");
const enabledInput = document.getElementById("enabled");
const speedInput = document.getElementById("speed");
const accelInput = document.getElementById("accel");
const minInput = document.getElementById("min-ml");
const maxInput = document.getElementById("max-ml");
const dirInput = document.getElementById("dir-inv");
const valveEnabled = document.getElementById("valve-enabled");
const valveActiveHigh = document.getElementById("valve-active-high");
const valvePreOpen = document.getElementById("valve-pre-open");
const valvePostClose = document.getElementById("valve-post-close");
const antiDrip = document.getElementById("anti-drip");
const antiDripSteps = document.getElementById("anti-drip-steps");
const antiDripSpeed = document.getElementById("anti-drip-speed");
const valveHwNote = document.getElementById("valve-hw-note");
const calNote = document.getElementById("cal-note");
const configStatus = document.getElementById("config-status");

async function refreshValveHardwareNote() {
  const settings = await getSettings();
  const count = settings.pump_count ?? 1;
  Array.from(pumpIdSelect.options).forEach((option) => {
    const required =
      option.value === "pump_3" ? 3 : option.value === "pump_2" ? 2 : 1;
    option.disabled = required > count;
    option.hidden = required > count;
  });
  if (pumpIdRequiredDisabled(pumpIdSelect.value, count)) {
    pumpIdSelect.value = "pump_1";
  }
  pumpIdSelect.disabled = count < 2;
  pumpCountNote.textContent =
    count < 2
      ? "Second/third pump paths are disabled in Diagnostics (pump count = 1). Profiles stay on Pump 1."
      : `Pump count = ${count}. Bind this profile to an enabled path. Only one path moves at a time.`;
  valveHwNote.textContent = settings.valve_hardware_present
    ? "Valve hardware is enabled in Diagnostics settings. Timing below applies when this profile uses the valve."
    : "Valve hardware is currently disabled in Diagnostics settings. Enable it there before profile valve timing takes effect.";
}

function pumpIdRequiredDisabled(pumpId, count) {
  if (pumpId === "pump_3") return count < 3;
  if (pumpId === "pump_2") return count < 2;
  return false;
}

async function loadProfile() {
  const profile = await getProfile(profileSelect.value);
  nameInput.value = profile.name;
  pumpIdSelect.value = profile.pump_id || "pump_1";
  enabledInput.checked = profile.enabled;
  speedInput.value = profile.motor.speed_steps_per_second;
  accelInput.value = profile.motor.acceleration_steps_per_second_squared;
  minInput.value = profile.limits.minimum_ml;
  maxInput.value = profile.limits.maximum_ml;
  dirInput.checked = profile.motor.direction_inverted;
  valveEnabled.checked = profile.valve.enabled;
  valveActiveHigh.checked = profile.valve.active_high;
  valvePreOpen.value = profile.valve.pre_open_ms;
  valvePostClose.value = profile.valve.post_motor_close_ms;
  antiDrip.checked = profile.valve.anti_drip_enabled;
  antiDripSteps.value = profile.valve.anti_drip_reverse_steps;
  antiDripSpeed.value = profile.valve.anti_drip_speed_steps_per_second;
  calNote.textContent = profile.calibrated
    ? `Calibrated: ${profile.calibration.steps_per_ml.toFixed(2)} steps/mL`
    : "Not calibrated";
  await refreshValveHardwareNote();
}

document.getElementById("profile-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    const profile = await updateProfile(profileSelect.value, {
      name: nameInput.value,
      pump_id: pumpIdSelect.value,
      enabled: enabledInput.checked,
      motor: {
        speed_steps_per_second: Number(speedInput.value),
        acceleration_steps_per_second_squared: Number(accelInput.value),
        deceleration_steps_per_second_squared: Number(accelInput.value),
        direction_inverted: dirInput.checked
      },
      limits: {
        minimum_ml: Number(minInput.value),
        maximum_ml: Number(maxInput.value)
      },
      valve: {
        enabled: valveEnabled.checked,
        active_high: valveActiveHigh.checked,
        pre_open_ms: Number(valvePreOpen.value),
        post_motor_close_ms: Number(valvePostClose.value),
        anti_drip_enabled: antiDrip.checked,
        anti_drip_reverse_steps: Number(antiDripSteps.value),
        anti_drip_speed_steps_per_second: Number(antiDripSpeed.value)
      }
    });
    calNote.textContent = profile.calibrated
      ? `Calibrated: ${profile.calibration.steps_per_ml.toFixed(2)} steps/mL`
      : "Not calibrated — recalibrate after speed changes";
    await fillProfileSelect(profileSelect, profile.id);
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("reset-cal-btn").addEventListener("click", async () => {
  if (!confirm("Reset calibration for this profile?")) {
    return;
  }
  try {
    await resetCalibration(profileSelect.value);
    await loadProfile();
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("export-btn").addEventListener("click", async () => {
  try {
    const config = await exportConfig();
    const blob = new Blob([JSON.stringify(config, null, 2)], {
      type: "application/json"
    });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "pump-controller-config.json";
    link.click();
    URL.revokeObjectURL(url);
    configStatus.textContent = "Export downloaded.";
  } catch (error) {
    alert(error.message);
  }
});

document.getElementById("import-file").addEventListener("change", async (event) => {
  const file = event.target.files?.[0];
  if (!file) {
    return;
  }
  try {
    const text = await file.text();
    const payload = JSON.parse(text);
    const result = await importConfig(payload);
    configStatus.textContent = `Imported ${result.profile_count} profiles.`;
    await fillProfileSelect(profileSelect);
    await loadProfile();
  } catch (error) {
    alert(error.message);
  } finally {
    event.target.value = "";
  }
});

profileSelect.addEventListener("change", () => {
  loadProfile().catch((error) => alert(error.message));
});

fillProfileSelect(profileSelect)
  .then(() => loadProfile())
  .catch((error) => alert(error.message));

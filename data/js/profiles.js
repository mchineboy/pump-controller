import {
  exportConfig,
  fillProfileSelect,
  getProfile,
  importConfig,
  resetCalibration,
  updateProfile
} from "./api.js";

const profileSelect = document.getElementById("profile");
const nameInput = document.getElementById("name");
const enabledInput = document.getElementById("enabled");
const speedInput = document.getElementById("speed");
const accelInput = document.getElementById("accel");
const minInput = document.getElementById("min-ml");
const maxInput = document.getElementById("max-ml");
const dirInput = document.getElementById("dir-inv");
const calNote = document.getElementById("cal-note");
const configStatus = document.getElementById("config-status");

async function loadProfile() {
  const profile = await getProfile(profileSelect.value);
  nameInput.value = profile.name;
  enabledInput.checked = profile.enabled;
  speedInput.value = profile.motor.speed_steps_per_second;
  accelInput.value = profile.motor.acceleration_steps_per_second_squared;
  minInput.value = profile.limits.minimum_ml;
  maxInput.value = profile.limits.maximum_ml;
  dirInput.checked = profile.motor.direction_inverted;
  calNote.textContent = profile.calibrated
    ? `Calibrated: ${profile.calibration.steps_per_ml.toFixed(2)} steps/mL`
    : "Not calibrated";
}

document.getElementById("profile-form").addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    const profile = await updateProfile(profileSelect.value, {
      name: nameInput.value,
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

export async function requestJson(url, options = {}) {
  const headers = {
    Accept: "application/json",
    ...options.headers
  };

  if (options.body && !(options.body instanceof FormData)) {
    headers["Content-Type"] = "application/json";
  }

  const response = await fetch(url, {
    ...options,
    headers
  });

  if (!response.ok) {
    let message = `Request failed with status ${response.status}`;
    try {
      const body = await response.json();
      message = body.error ?? message;
    } catch {
      // Response was not JSON.
    }
    throw new Error(message);
  }

  if (response.status === 204) {
    return null;
  }

  return response.json();
}

export function getStatus() {
  return requestJson("/api/status");
}

export function getProfiles() {
  return requestJson("/api/profiles");
}

export function getProfile(id) {
  return requestJson(`/api/profile?id=${encodeURIComponent(id)}`);
}

export function updateProfile(id, payload) {
  return requestJson(`/api/profile?id=${encodeURIComponent(id)}`, {
    method: "PUT",
    body: JSON.stringify(payload)
  });
}

export function resetCalibration(id) {
  return requestJson(`/api/profile/reset-calibration?id=${encodeURIComponent(id)}`, {
    method: "POST"
  });
}

export function getCalibrationHistory(id) {
  return requestJson(`/api/profile/history?id=${encodeURIComponent(id)}`);
}

export function getOperation() {
  return requestJson("/api/operation");
}

export function startDispense(profileId, volumeMl) {
  return requestJson("/api/dispense", {
    method: "POST",
    body: JSON.stringify({
      profile_id: profileId,
      volume_ml: volumeMl
    })
  });
}

export function stopOperation() {
  return requestJson("/api/stop", { method: "POST" });
}

export function acknowledgeFault() {
  return requestJson("/api/fault/ack", { method: "POST" });
}

export function injectDriverFault() {
  return requestJson("/api/fault/inject-driver", { method: "POST" });
}

export function startCalibration(profileId, durationMs) {
  return requestJson("/api/calibration/start", {
    method: "POST",
    body: JSON.stringify({
      profile_id: profileId,
      duration_ms: durationMs
    })
  });
}

export function submitMeasurement(measuredMl) {
  return requestJson("/api/calibration/measurement", {
    method: "POST",
    body: JSON.stringify({ measured_ml: measuredMl })
  });
}

export function removeCalibrationSample(index) {
  return requestJson("/api/calibration/sample/remove", {
    method: "POST",
    body: JSON.stringify({ index })
  });
}

export function saveCalibration() {
  return requestJson("/api/calibration/save", {
    method: "POST",
    body: "{}"
  });
}

export function discardCalibration() {
  return requestJson("/api/calibration/discard", { method: "POST" });
}

export function getSettings() {
  return requestJson("/api/settings");
}

export function updateSettings(payload) {
  return requestJson("/api/settings", {
    method: "PUT",
    body: JSON.stringify(payload)
  });
}

export function getEventLog() {
  return requestJson("/api/events/log");
}

export function clearEventLog() {
  return requestJson("/api/events/log", { method: "DELETE" });
}

export function factoryReset(confirm = "FACTORY_RESET") {
  return requestJson("/api/factory-reset", {
    method: "POST",
    body: JSON.stringify({ confirm })
  });
}

export function tareLoadCell() {
  return requestJson("/api/loadcell/tare", { method: "POST" });
}

export function calibrateLoadCell(knownGrams) {
  return requestJson("/api/loadcell/calibrate", {
    method: "POST",
    body: JSON.stringify({ known_grams: knownGrams })
  });
}

export function resetFlowCumulative() {
  return requestJson("/api/flow/reset", { method: "POST" });
}

export function exportConfig() {
  return requestJson("/api/config/export");
}

export function importConfig(payload) {
  return requestJson("/api/config/import", {
    method: "POST",
    body: JSON.stringify(payload)
  });
}

export async function fillProfileSelect(selectEl, selectedId = null) {
  const [profiles, settings] = await Promise.all([getProfiles(), getSettings()]);
  const showPump = (settings.pump_count ?? 1) >= 2;
  selectEl.innerHTML = "";
  profiles.forEach((profile) => {
    const option = document.createElement("option");
    option.value = profile.id;
    const pumpLabel =
      showPump && profile.pump_id ? ` · ${profile.pump_id}` : "";
    option.textContent =
      `${profile.name}${pumpLabel}${profile.calibrated ? "" : " (uncalibrated)"}`;
    selectEl.appendChild(option);
  });
  if (selectedId) {
    selectEl.value = selectedId;
  }
}

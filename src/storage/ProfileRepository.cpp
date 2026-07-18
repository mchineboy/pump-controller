#include "storage/ProfileRepository.h"

#include <LittleFS.h>
#include <Preferences.h>

#include "Config.h"

namespace {
constexpr const char* kProfilesPath = "/profiles.json";
}

FluidProfile ProfileRepository::makeDefault(const char* id, const char* name) const {
    FluidProfile profile;
    profile.id = id;
    profile.name = name;
    profile.pumpId = "pump_1";
    profile.enabled = true;
    profile.calibrated = false;
    profile.motor.speedStepsPerSecond = Config::kDefaultSpeedStepsPerSec;
    profile.motor.accelerationStepsPerSecondSquared =
        Config::kDefaultAccelStepsPerSec2;
    profile.motor.decelerationStepsPerSecondSquared =
        Config::kDefaultAccelStepsPerSec2;
    profile.limits.minimumMl = Config::kDefaultMinDispenseMl;
    profile.limits.maximumMl = Config::kDefaultMaxDispenseMl;
    return profile;
}

void ProfileRepository::seedDefaults() {
    profiles_.clear();
    profiles_.push_back(makeDefault("fluid_1", "Fluid 1"));
    profiles_.push_back(makeDefault("fluid_2", "Fluid 2"));
    profiles_.push_back(makeDefault("fluid_3", "Fluid 3"));
    profiles_.push_back(makeDefault("fluid_4", "Fluid 4"));
    profiles_.push_back(makeDefault("fluid_5", "Fluid 5"));
    profiles_.push_back(makeDefault("fluid_6", "Fluid 6"));
    activeId_ = "fluid_1";
}

void ProfileRepository::profileToJson(
    const FluidProfile& profile,
    JsonObject obj
) const {
    obj["id"] = profile.id;
    obj["name"] = profile.name;
    obj["pump_id"] = profile.pumpId;
    obj["enabled"] = profile.enabled;
    obj["calibrated"] = profile.calibrated;

    JsonObject cal = obj["calibration"].to<JsonObject>();
    cal["valid"] = profile.calibration.valid;
    cal["method"] = profile.calibration.method;
    cal["requested_duration_ms"] = profile.calibration.requestedDurationMs;
    cal["actual_duration_ms"] = profile.calibration.actualDurationMs;
    cal["step_count"] = profile.calibration.stepCount;
    cal["measured_ml"] = profile.calibration.measuredMl;
    cal["steps_per_ml"] = profile.calibration.stepsPerMl;
    cal["ml_per_second"] = profile.calibration.mlPerSecond;
    cal["sample_count"] = profile.calibration.sampleCount;
    cal["calibrated_at"] = profile.calibration.calibratedAt;

    JsonObject motor = obj["motor"].to<JsonObject>();
    motor["speed_steps_per_second"] = profile.motor.speedStepsPerSecond;
    motor["acceleration_steps_per_second_squared"] =
        profile.motor.accelerationStepsPerSecondSquared;
    motor["deceleration_steps_per_second_squared"] =
        profile.motor.decelerationStepsPerSecondSquared;
    motor["direction_inverted"] = profile.motor.directionInverted;
    motor["microsteps"] = profile.motor.microsteps;

    JsonObject valve = obj["valve"].to<JsonObject>();
    valve["enabled"] = profile.valve.enabled;
    valve["active_high"] = profile.valve.activeHigh;
    valve["pre_open_ms"] = profile.valve.preOpenMs;
    valve["post_motor_close_ms"] = profile.valve.postMotorCloseMs;
    valve["anti_drip_enabled"] = profile.valve.antiDripEnabled;
    valve["anti_drip_reverse_steps"] = profile.valve.antiDripReverseSteps;
    valve["anti_drip_speed_steps_per_second"] =
        profile.valve.antiDripSpeedStepsPerSecond;

    JsonObject limits = obj["limits"].to<JsonObject>();
    limits["minimum_ml"] = profile.limits.minimumMl;
    limits["maximum_ml"] = profile.limits.maximumMl;
}

bool ProfileRepository::profileFromJson(
    const JsonVariantConst src,
    FluidProfile& profile
) const {
    if (!src.is<JsonObjectConst>()) {
        return false;
    }
    JsonObjectConst obj = src.as<JsonObjectConst>();
    const char* id = obj["id"] | "";
    if (id[0] == '\0') {
        return false;
    }

    profile = makeDefault(id, obj["name"] | id);
    profile.name = obj["name"] | id;
    profile.pumpId = obj["pump_id"] | "pump_1";
    if (profile.pumpId != "pump_1" && profile.pumpId != "pump_2") {
        profile.pumpId = "pump_1";
    }
    profile.enabled = obj["enabled"] | true;
    profile.calibrated = obj["calibrated"] | false;

    JsonObjectConst cal = obj["calibration"].as<JsonObjectConst>();
    if (!cal.isNull()) {
        profile.calibration.valid = cal["valid"] | profile.calibrated;
        profile.calibration.method = cal["method"] | "timed_step_count";
        profile.calibration.requestedDurationMs =
            cal["requested_duration_ms"] | Config::kDefaultCalibrationDurationMs;
        profile.calibration.actualDurationMs =
            cal["actual_duration_ms"] | Config::kDefaultCalibrationDurationMs;
        profile.calibration.stepCount = cal["step_count"] | 0;
        profile.calibration.measuredMl = cal["measured_ml"] | 0.0f;
        profile.calibration.stepsPerMl = cal["steps_per_ml"] | 0.0f;
        profile.calibration.mlPerSecond = cal["ml_per_second"] | 0.0f;
        profile.calibration.sampleCount = cal["sample_count"] | 0;
        profile.calibration.calibratedAt = cal["calibrated_at"] | "";
        profile.calibrated =
            profile.calibrated && profile.calibration.stepsPerMl > 0.0f;
        profile.calibration.valid = profile.calibrated;
    }

    JsonObjectConst motor = obj["motor"].as<JsonObjectConst>();
    if (!motor.isNull()) {
        profile.motor.speedStepsPerSecond =
            motor["speed_steps_per_second"] | Config::kDefaultSpeedStepsPerSec;
        profile.motor.accelerationStepsPerSecondSquared =
            motor["acceleration_steps_per_second_squared"] |
            Config::kDefaultAccelStepsPerSec2;
        profile.motor.decelerationStepsPerSecondSquared =
            motor["deceleration_steps_per_second_squared"] |
            profile.motor.accelerationStepsPerSecondSquared;
        profile.motor.directionInverted = motor["direction_inverted"] | false;
        profile.motor.microsteps = motor["microsteps"] | 16;
    }

    JsonObjectConst valve = obj["valve"].as<JsonObjectConst>();
    if (!valve.isNull()) {
        profile.valve.enabled = valve["enabled"] | false;
        profile.valve.activeHigh = valve["active_high"] | true;
        profile.valve.preOpenMs = valve["pre_open_ms"] | 50;
        profile.valve.postMotorCloseMs = valve["post_motor_close_ms"] | 75;
        profile.valve.antiDripEnabled = valve["anti_drip_enabled"] | false;
        profile.valve.antiDripReverseSteps = valve["anti_drip_reverse_steps"] | 0;
        profile.valve.antiDripSpeedStepsPerSecond =
            valve["anti_drip_speed_steps_per_second"] | 300;
    }

    JsonObjectConst limits = obj["limits"].as<JsonObjectConst>();
    if (!limits.isNull()) {
        profile.limits.minimumMl =
            limits["minimum_ml"] | Config::kDefaultMinDispenseMl;
        profile.limits.maximumMl =
            limits["maximum_ml"] | Config::kDefaultMaxDispenseMl;
    }
    return true;
}

bool ProfileRepository::saveToFs() const {
    JsonDocument doc;
    doc["schema"] = Config::kProfilesSchemaVersion;
    doc["active_id"] = activeId_;
    JsonArray arr = doc["profiles"].to<JsonArray>();
    for (const FluidProfile& profile : profiles_) {
        profileToJson(profile, arr.add<JsonObject>());
    }

    File file = LittleFS.open(kProfilesPath, FILE_WRITE);
    if (!file) {
        return false;
    }
    const bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}

bool ProfileRepository::loadFromFs() {
    if (!LittleFS.exists(kProfilesPath)) {
        return false;
    }

    File file = LittleFS.open(kProfilesPath, FILE_READ);
    if (!file) {
        return false;
    }

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        return false;
    }

    const int schema = doc["schema"] | 0;
    if (schema < Config::kProfilesSchemaVersion) {
        // Reseed generic Fluid 1–6 defaults once when upgrading schema.
        return false;
    }

    profiles_.clear();
    JsonArrayConst arr = doc["profiles"].as<JsonArrayConst>();
    for (JsonVariantConst item : arr) {
        if (profiles_.size() >= kMaxProfiles) {
            break;
        }
        FluidProfile profile;
        if (profileFromJson(item, profile)) {
            profiles_.push_back(profile);
        }
    }

    activeId_ = doc["active_id"] | "fluid_1";
    if (profiles_.empty()) {
        return false;
    }
    if (!has(activeId_)) {
        activeId_ = profiles_.front().id;
    }
    return true;
}

bool ProfileRepository::begin() {
    seedDefaults();

    // Migrate single-profile NVS data into Fluid 1 if present.
    Preferences nvs;
    if (nvs.begin("pump", true)) {
        if (nvs.isKey("steps_per_ml")) {
            const int index = findIndex("fluid_1");
            if (index >= 0) {
                FluidProfile& profile = profiles_[static_cast<size_t>(index)];
                profile.calibrated = nvs.getBool("calibrated", false);
                profile.calibration.valid = profile.calibrated;
                profile.calibration.stepsPerMl = nvs.getFloat("steps_per_ml", 0.0f);
                profile.calibration.mlPerSecond = nvs.getFloat("ml_per_sec", 0.0f);
                profile.calibration.measuredMl = nvs.getFloat("measured_ml", 0.0f);
                profile.calibration.stepCount = nvs.getLong64("step_count", 0);
                profile.calibration.requestedDurationMs =
                    nvs.getUInt("req_dur_ms", Config::kDefaultCalibrationDurationMs);
                profile.calibration.actualDurationMs =
                    nvs.getUInt("act_dur_ms", Config::kDefaultCalibrationDurationMs);
                profile.calibration.sampleCount = nvs.getUChar("sample_count", 0);
                profile.calibration.calibratedAt = nvs.getString("calibrated_at", "");
                profile.motor.speedStepsPerSecond =
                    nvs.getUInt("speed", Config::kDefaultSpeedStepsPerSec);
                profile.motor.accelerationStepsPerSecondSquared =
                    nvs.getUInt("accel", Config::kDefaultAccelStepsPerSec2);
                profile.motor.decelerationStepsPerSecondSquared =
                    nvs.getUInt("decel", Config::kDefaultAccelStepsPerSec2);
                profile.motor.directionInverted = nvs.getBool("dir_inv", false);
                profile.limits.minimumMl =
                    nvs.getFloat("min_ml", Config::kDefaultMinDispenseMl);
                profile.limits.maximumMl =
                    nvs.getFloat("max_ml", Config::kDefaultMaxDispenseMl);
                profile.calibrated =
                    profile.calibrated && profile.calibration.stepsPerMl > 0.0f;
            }
        }
        nvs.end();
    }

    if (!loadFromFs()) {
        saveToFs();
    }
    return true;
}

int ProfileRepository::findIndex(const String& id) const {
    for (size_t i = 0; i < profiles_.size(); ++i) {
        if (profiles_[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ProfileRepository::has(const String& id) const {
    return findIndex(id) >= 0;
}

FluidProfile ProfileRepository::get(const String& id) const {
    const int index = findIndex(id);
    if (index < 0) {
        FluidProfile empty;
        empty.id = id;
        empty.enabled = false;
        return empty;
    }
    return profiles_[static_cast<size_t>(index)];
}

bool ProfileRepository::save(const FluidProfile& profile) {
    if (profile.id.isEmpty()) {
        return false;
    }

    const int index = findIndex(profile.id);
    if (index >= 0) {
        profiles_[static_cast<size_t>(index)] = profile;
    } else {
        if (profiles_.size() >= kMaxProfiles) {
            return false;
        }
        profiles_.push_back(profile);
    }
    return saveToFs();
}

bool ProfileRepository::remove(const String& id) {
    const int index = findIndex(id);
    if (index < 0 || profiles_.size() <= 1) {
        return false;
    }
    profiles_.erase(profiles_.begin() + index);
    if (activeId_ == id) {
        activeId_ = profiles_.front().id;
    }
    return saveToFs();
}

FluidProfile& ProfileRepository::activeProfile() {
    const int index = findIndex(activeId_);
    if (index < 0) {
        activeId_ = profiles_.front().id;
        return profiles_.front();
    }
    return profiles_[static_cast<size_t>(index)];
}

const FluidProfile& ProfileRepository::activeProfile() const {
    const int index = findIndex(activeId_);
    if (index < 0) {
        return profiles_.front();
    }
    return profiles_[static_cast<size_t>(index)];
}

bool ProfileRepository::setActiveProfileId(const String& id) {
    if (!has(id)) {
        return false;
    }
    activeId_ = id;
    return saveToFs();
}

bool ProfileRepository::loadHistory(const String& profileId, JsonDocument& doc) const {
    const String path = String("/history_") + profileId + ".json";
    if (!LittleFS.exists(path)) {
        doc["entries"].to<JsonArray>();
        return true;
    }
    File file = LittleFS.open(path, FILE_READ);
    if (!file) {
        return false;
    }
    const DeserializationError err = deserializeJson(doc, file);
    file.close();
    return !err;
}

bool ProfileRepository::saveHistory(
    const String& profileId,
    const JsonDocument& doc
) const {
    const String path = String("/history_") + profileId + ".json";
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        return false;
    }
    const bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}

bool ProfileRepository::appendCalibrationHistory(
    const String& profileId,
    const CalibrationHistoryEntry& entry
) {
    if (!has(profileId)) {
        return false;
    }

    JsonDocument doc;
    if (!loadHistory(profileId, doc)) {
        doc.clear();
    }
    JsonArray entries = doc["entries"].is<JsonArray>()
        ? doc["entries"].as<JsonArray>()
        : doc["entries"].to<JsonArray>();

    JsonObject obj = entries.add<JsonObject>();
    obj["timestamp"] = entry.timestamp;
    obj["requested_duration_ms"] = entry.requestedDurationMs;
    obj["actual_duration_ms"] = entry.actualDurationMs;
    obj["step_count"] = entry.stepCount;
    obj["measured_ml"] = entry.measuredMl;
    obj["steps_per_ml"] = entry.stepsPerMl;

    while (entries.size() > kMaxHistoryEntries) {
        entries.remove(0);
    }
    return saveHistory(profileId, doc);
}

std::vector<CalibrationHistoryEntry> ProfileRepository::getCalibrationHistory(
    const String& profileId
) const {
    std::vector<CalibrationHistoryEntry> out;
    JsonDocument doc;
    if (!loadHistory(profileId, doc)) {
        return out;
    }
    for (JsonObjectConst item : doc["entries"].as<JsonArrayConst>()) {
        CalibrationHistoryEntry entry;
        entry.timestamp = item["timestamp"] | "";
        entry.requestedDurationMs = item["requested_duration_ms"] | 0;
        entry.actualDurationMs = item["actual_duration_ms"] | 0;
        entry.stepCount = item["step_count"] | 0;
        entry.measuredMl = item["measured_ml"] | 0.0f;
        entry.stepsPerMl = item["steps_per_ml"] | 0.0f;
        out.push_back(entry);
    }
    return out;
}

bool ProfileRepository::exportToJson(JsonDocument& doc) const {
    doc["schema"] = Config::kProfilesSchemaVersion;
    doc["active_id"] = activeId_;
    JsonArray arr = doc["profiles"].to<JsonArray>();
    for (const FluidProfile& profile : profiles_) {
        profileToJson(profile, arr.add<JsonObject>());
    }
    return true;
}

bool ProfileRepository::importFromJson(const JsonDocument& doc) {
    JsonArrayConst arr = doc["profiles"].as<JsonArrayConst>();
    if (arr.isNull() || arr.size() == 0) {
        return false;
    }

    std::vector<FluidProfile> imported;
    for (JsonVariantConst item : arr) {
        if (imported.size() >= kMaxProfiles) {
            break;
        }
        FluidProfile profile;
        if (profileFromJson(item, profile)) {
            imported.push_back(profile);
        }
    }
    if (imported.empty()) {
        return false;
    }

    profiles_ = imported;
    activeId_ = doc["active_id"] | profiles_.front().id;
    if (!has(activeId_)) {
        activeId_ = profiles_.front().id;
    }
    return saveToFs();
}

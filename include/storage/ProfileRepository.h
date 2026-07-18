#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "models/FluidProfile.h"

struct CalibrationHistoryEntry {
    String timestamp;
    uint32_t requestedDurationMs = 0;
    uint32_t actualDurationMs = 0;
    int64_t stepCount = 0;
    float measuredMl = 0.0f;
    float stepsPerMl = 0.0f;
};

class ProfileRepository {
public:
    static constexpr size_t kMaxProfiles = 8;
    static constexpr size_t kMaxHistoryEntries = 20;

    bool begin();

    size_t count() const { return profiles_.size(); }
    const std::vector<FluidProfile>& all() const { return profiles_; }

    bool has(const String& id) const;
    FluidProfile get(const String& id) const;
    bool save(const FluidProfile& profile);
    bool remove(const String& id);

    FluidProfile& activeProfile();
    const FluidProfile& activeProfile() const;
    bool setActiveProfileId(const String& id);
    const String& activeProfileId() const { return activeId_; }

    bool appendCalibrationHistory(
        const String& profileId,
        const CalibrationHistoryEntry& entry
    );
    std::vector<CalibrationHistoryEntry> getCalibrationHistory(
        const String& profileId
    ) const;

    bool exportToJson(JsonDocument& doc) const;
    bool importFromJson(const JsonDocument& doc);

    /** Wipe NVS profile store and reseed defaults. Does not touch settings NVS. */
    bool factoryResetProfiles();
    /** Delete LittleFS history_* and legacy profiles.json copies. */
    void eraseFilesystemArtifacts();

private:
    std::vector<FluidProfile> profiles_;
    String activeId_ = "fluid_1";

    void seedDefaults();
    FluidProfile makeDefault(const char* id, const char* name) const;
    int findIndex(const String& id) const;
    bool loadFromNvs();
    bool saveToNvs() const;
    bool loadFromFs();
    bool migrateLegacySingleProfileNvs();
    bool loadHistory(const String& profileId, JsonDocument& doc) const;
    bool saveHistory(const String& profileId, const JsonDocument& doc) const;
    void profileToJson(const FluidProfile& profile, JsonObject obj) const;
    bool profileFromJson(const JsonVariantConst src, FluidProfile& profile) const;
    String profileNvsKey(const String& id) const;
};

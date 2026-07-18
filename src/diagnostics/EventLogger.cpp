#include "diagnostics/EventLogger.h"

#include <LittleFS.h>
#include <vector>

namespace {
constexpr const char* kLogPath = "/events.log";
constexpr size_t kMaxLogBytes = 24 * 1024;
}  // namespace

bool EventLogger::begin() {
    // ApplicationController mounts LittleFS before services start.
    ready_ = true;
    return ready_;
}

void EventLogger::log(const char* event) {
    JsonDocument doc;
    log(event, doc);
}

void EventLogger::log(const char* event, const JsonDocument& fields) {
    if (!ready_ || !enabled_ || event == nullptr) {
        return;
    }

    JsonDocument entry;
    entry["timestamp"] = millis();
    entry["event"] = event;
    if (fields.is<JsonObjectConst>()) {
        for (JsonPairConst kv : fields.as<JsonObjectConst>()) {
            entry[kv.key()] = kv.value();
        }
    }

    File file = LittleFS.open(kLogPath, FILE_APPEND);
    if (!file) {
        file = LittleFS.open(kLogPath, FILE_WRITE);
    }
    if (!file) {
        return;
    }

    serializeJson(entry, file);
    file.print('\n');
    file.close();
    trimIfNeeded();
}

String EventLogger::readRecent(size_t maxEntries) const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    if (!ready_ || !LittleFS.exists(kLogPath)) {
        String out;
        serializeJson(doc, out);
        return out;
    }

    File file = LittleFS.open(kLogPath, FILE_READ);
    if (!file) {
        String out;
        serializeJson(doc, out);
        return out;
    }

    std::vector<String> lines;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (!line.isEmpty()) {
            lines.push_back(line);
        }
    }
    file.close();

    const size_t start =
        lines.size() > maxEntries ? lines.size() - maxEntries : 0;
    for (size_t i = start; i < lines.size(); ++i) {
        JsonDocument entry;
        if (deserializeJson(entry, lines[i]) == DeserializationError::Ok) {
            arr.add(entry.as<JsonObject>());
        }
    }

    String out;
    serializeJson(doc, out);
    return out;
}

bool EventLogger::clear() {
    if (!ready_) {
        return false;
    }
    if (LittleFS.exists(kLogPath)) {
        return LittleFS.remove(kLogPath);
    }
    return true;
}

void EventLogger::trimIfNeeded() {
    File file = LittleFS.open(kLogPath, FILE_READ);
    if (!file) {
        return;
    }
    const size_t size = file.size();
    if (size <= kMaxLogBytes) {
        file.close();
        return;
    }

    std::vector<String> lines;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (!line.isEmpty()) {
            lines.push_back(line);
        }
    }
    file.close();

    const size_t keep = lines.size() / 2;
    File out = LittleFS.open(kLogPath, FILE_WRITE);
    if (!out) {
        return;
    }
    for (size_t i = keep; i < lines.size(); ++i) {
        out.println(lines[i]);
    }
    out.close();
}

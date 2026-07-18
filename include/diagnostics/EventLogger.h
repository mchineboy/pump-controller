#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class EventLogger {
public:
    bool begin();
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void log(const char* event);
    void log(const char* event, const JsonDocument& fields);

    String readRecent(size_t maxEntries = 50) const;
    bool clear();

private:
    bool enabled_ = true;
    bool ready_ = false;

    void trimIfNeeded();
};

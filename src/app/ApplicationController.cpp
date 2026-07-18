#include "app/ApplicationController.h"

#include <LittleFS.h>
#include <WiFi.h>

#include "Config.h"
#include "secrets.h"

namespace {
constexpr uint32_t kWifiConnectTimeoutMs = 30000;
wifi_err_reason_t gLastDisconnectReason = WIFI_REASON_UNSPECIFIED;

void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        gLastDisconnectReason =
            static_cast<wifi_err_reason_t>(info.wifi_sta_disconnected.reason);
    }
}

const char* wifiStatusName(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS: return "idle";
        case WL_NO_SSID_AVAIL: return "no_ssid";
        case WL_SCAN_COMPLETED: return "scan_completed";
        case WL_CONNECTED: return "connected";
        case WL_CONNECT_FAILED: return "connect_failed";
        case WL_CONNECTION_LOST: return "connection_lost";
        case WL_DISCONNECTED: return "disconnected";
        default: return "unknown";
    }
}
}

void ApplicationController::beginSafeOutputs() {
    pinMode(PUMP_STEP_PIN, OUTPUT);
    pinMode(PUMP_DIR_PIN, OUTPUT);
    pinMode(PUMP_ENABLE_PIN, OUTPUT);
    digitalWrite(PUMP_STEP_PIN, LOW);
    digitalWrite(PUMP_DIR_PIN, LOW);
    digitalWrite(PUMP_ENABLE_PIN, HIGH);  // driver disabled (active-low enable)

    pinMode(PUMP2_STEP_PIN, OUTPUT);
    pinMode(PUMP2_DIR_PIN, OUTPUT);
    pinMode(PUMP2_ENABLE_PIN, OUTPUT);
    digitalWrite(PUMP2_STEP_PIN, LOW);
    digitalWrite(PUMP2_DIR_PIN, LOW);
    digitalWrite(PUMP2_ENABLE_PIN, HIGH);
}

void ApplicationController::beginNetwork() {
    const GlobalSettings settings = settings_.get();
    WiFi.persistent(false);
    WiFi.onEvent(onWifiEvent);
    WiFi.setHostname(settings.hostname.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect(true, true);
    delay(200);

    Serial.print("STA MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Password length: ");
    Serial.println(strlen(Secrets::kWifiPassword));

    int8_t channel = 0;
    uint8_t bssid[6] = {};
    bool targetFound = false;

    const int networkCount = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
    for (int i = 0; i < networkCount; ++i) {
        if (WiFi.SSID(i) != Secrets::kWifiSsid) {
            continue;
        }
        targetFound = true;
        channel = WiFi.channel(i);
        const uint8_t* foundBssid = WiFi.BSSID(i);
        if (foundBssid != nullptr) {
            memcpy(bssid, foundBssid, sizeof(bssid));
        }
        Serial.print("Target Wi-Fi found; RSSI: ");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm, channel: ");
        Serial.print(channel);
        Serial.print(", encryption: ");
        Serial.print(static_cast<int>(WiFi.encryptionType(i)));
        Serial.print(", BSSID: ");
        Serial.println(WiFi.BSSIDstr(i));
        break;
    }
    if (!targetFound) {
        Serial.println("Target Wi-Fi SSID was not found by the ESP32.");
    }
    WiFi.scanDelete();

    if (targetFound && channel > 0) {
        WiFi.begin(Secrets::kWifiSsid, Secrets::kWifiPassword, channel, bssid);
    } else {
        WiFi.begin(Secrets::kWifiSsid, Secrets::kWifiPassword);
    }

    Serial.print("Connecting to Wi-Fi");
    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startedAt < kWifiConnectTimeoutMs) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Wi-Fi SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("DHCP IP: ");
        Serial.println(WiFi.localIP());
        return;
    }

    Serial.print("Wi-Fi status: ");
    Serial.println(wifiStatusName(WiFi.status()));
    Serial.print("Disconnect reason: ");
    Serial.print(static_cast<int>(gLastDisconnectReason));
    Serial.print(" (");
    Serial.print(WiFi.disconnectReasonName(gLastDisconnectReason));
    Serial.println(")");
    Serial.println("Wi-Fi connection failed; starting fallback access point.");
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(Config::kApSsid, Config::kApPassword);
    Serial.print("Fallback AP SSID: ");
    Serial.println(Config::kApSsid);
    Serial.print("Fallback AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void ApplicationController::applyTmcSettings(const GlobalSettings& settings) {
    TmcDriverConfig config;
    config.enabled = settings.driverUartEnabled;
    config.runCurrentMa = settings.driverRunCurrentMa;
    config.holdCurrentMa = settings.driverHoldCurrentMa;
    config.microsteps = settings.driverMicrosteps;
    config.stealthChop = settings.driverStealthChop;
    config.address = 0b00;

    if (!tmc_.apply(config)) {
        Serial.print("TMC2209 UART apply failed: ");
        Serial.println(tmc_.lastError());
        JsonDocument fields;
        fields["error"] = tmc_.lastError();
        logger_.log("tmc_uart_warning", fields);
        return;
    }

    if (config.enabled) {
        Serial.println("TMC2209 UART configured (pump_1)");
        logger_.log("tmc_uart_ok");
        if (settings.pumpCount >= 2) {
            TmcDriverConfig pump2 = config;
            pump2.address = PUMP2_TMC_ADDRESS;
            if (!tmc_.applyToAddress(PUMP2_TMC_ADDRESS, pump2)) {
                Serial.print("TMC2209 UART pump_2 apply failed: ");
                Serial.println(tmc_.lastError());
                JsonDocument fields;
                fields["error"] = tmc_.lastError();
                fields["pump_id"] = "pump_2";
                logger_.log("tmc_uart_warning", fields);
            } else {
                Serial.println("TMC2209 UART configured (pump_2)");
            }
        }
    }
}

void ApplicationController::applyReservoirSettings(const GlobalSettings& settings) {
    reservoir_.begin(
        PUMP_RESERVOIR_PIN,
        settings.reservoirSensorEnabled,
        settings.reservoirEmptyActiveLow
    );
    pump_.setReservoirEmptyPolicy(
        parseReservoirEmptyPolicy(settings.reservoirEmptyPolicy)
    );
}

void ApplicationController::applyLoadCellSettings(const GlobalSettings& settings) {
    loadCell_.begin(PUMP_LOADCELL_DT_PIN, PUMP_LOADCELL_SCK_PIN);
    loadCell_.configure(
        settings.loadCellEnabled,
        settings.loadCellScale,
        settings.loadCellOffset
    );
    if (settings.loadCellEnabled && !loadCell_.isReady()) {
        Serial.print("Load cell warning: ");
        Serial.println(loadCell_.lastError());
        JsonDocument fields;
        fields["error"] = loadCell_.lastError();
        logger_.log("loadcell_warning", fields);
    } else if (settings.loadCellEnabled) {
        logger_.log("loadcell_ok");
    }
}

void ApplicationController::applyTemperatureSettings(const GlobalSettings& settings) {
    temperature_.begin(PUMP_TEMP_PIN);
    temperature_.configure(
        settings.temperatureSensorEnabled,
        settings.temperatureWarnLowC,
        settings.temperatureWarnHighC
    );
    temperatureWarned_ = false;
    if (settings.temperatureSensorEnabled && !temperature_.isReady()) {
        Serial.print("Temperature sensor warning: ");
        Serial.println(temperature_.lastError());
        JsonDocument fields;
        fields["error"] = temperature_.lastError();
        logger_.log("temp_warning", fields);
    } else if (settings.temperatureSensorEnabled) {
        logger_.log("temp_ok");
    }
}

void ApplicationController::applyFlowSettings(const GlobalSettings& settings) {
    flow_.begin(PUMP_FLOW_PIN);
    flow_.configure(settings.flowSensorEnabled, settings.flowPulsesPerLiter);
    if (settings.flowSensorEnabled) {
        logger_.log("flow_ok");
    }
}

void ApplicationController::applyFeedbackSettings(const GlobalSettings& settings) {
    pump_.setFeedbackConfig(
        parseDispenseFeedbackMode(settings.dispenseFeedbackMode),
        parseDispenseFeedbackSource(settings.dispenseFeedbackSource),
        settings.dispenseFeedbackTolerancePercent,
        parseDispenseFeedbackOnMiss(settings.dispenseFeedbackOnMiss),
        settings.fluidDensityGPerMl
    );
}

void ApplicationController::begin() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("Fluid Dispensing Controller boot");

    beginSafeOutputs();

    if (!LittleFS.begin(false)) {
        Serial.println("LittleFS mount failed (not formatting; use Factory Reset if needed)");
    }

    settings_.begin();
    logger_.begin();
    logger_.setEnabled(settings_.get().loggingEnabled);
    profiles_.begin();

    const GlobalSettings settings = settings_.get();
    stepper_.begin(PUMP_STEP_PIN, PUMP_DIR_PIN, PUMP_ENABLE_PIN);
    stepper2_.begin(PUMP2_STEP_PIN, PUMP2_DIR_PIN, PUMP2_ENABLE_PIN);
    tmc_.begin(PUMP_TMC_RX_PIN, PUMP_TMC_TX_PIN);
    applyTmcSettings(settings);
    valve_.begin(PUMP_VALVE_PIN, settings.valveHardwarePresent, true);
    valve2_.begin(PUMP2_VALVE_PIN, settings.pump2ValveHardwarePresent, true);
    safety_.begin(PUMP_ESTOP_PIN, settings.emergencyStopEnabled);

    pump_.begin(
        stepper_,
        valve_,
        profiles_,
        safety_,
        logger_,
        tmc_,
        reservoir_,
        loadCell_,
        flow_
    );
    pump_.configureSecondPath(stepper2_, valve2_);
    pump_.setPumpCount(settings.pumpCount);
    applyReservoirSettings(settings);
    applyLoadCellSettings(settings);
    applyTemperatureSettings(settings);
    applyFlowSettings(settings);
    applyFeedbackSettings(settings);
    beginNetwork();
    web_.begin(
        pump_,
        profiles_,
        settings_,
        stepper_,
        valve_,
        safety_,
        logger_,
        tmc_,
        reservoir_,
        loadCell_,
        temperature_,
        flow_
    );

    logger_.log("boot");
    Serial.println("Boot complete. Previous operations are not resumed.");
}

void ApplicationController::loop() {
    safety_.update();
    loadCell_.update();
    temperature_.update();
    flow_.update();
    if (temperature_.isEnabled() && temperature_.isReady()) {
        const bool warning = temperature_.warnLow() || temperature_.warnHigh();
        if (warning && !temperatureWarned_) {
            JsonDocument fields;
            fields["celsius"] = temperature_.celsius();
            fields["warn_low"] = temperature_.warnLow();
            fields["warn_high"] = temperature_.warnHigh();
            logger_.log("temp_threshold", fields);
            temperatureWarned_ = true;
        } else if (!warning) {
            temperatureWarned_ = false;
        }
    }
    pump_.update();
    web_.update();
}

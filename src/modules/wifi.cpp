#include "wifi.h"

WiFiController::WiFiController() : eventsRegistered(false), failures(0), lastRetryCheckAt(0) {
    ssid[0] = '\0';
    password[0] = '\0';
    urlBuffer[0] = '\0';
    ipBuffer[0] = '\0';
};

bool WiFiController::connect(const char* ssid, const char* password) {
    if (!eventsRegistered) {
        registerEvents();
    }

    strlcpy(this->ssid, ssid, sizeof(this->ssid));
    strlcpy(this->password, password, sizeof(this->password));

    if (this->ssid[0] == '\0' || this->password[0] == '\0') {
        Log::instance()->info("No WiFi credentials configured\n");

        return startAccessPoint();
    }

    return startStation();
};

void WiFiController::loop() {
    if (ssid[0] == '\0' || password[0] == '\0' || WiFi.getMode() != WIFI_MODE_AP) {
        return;
    }

    if (millis() - lastRetryCheckAt < AP_RETRY_INTERVAL_MS) {
        return;
    }

    // startStation() stops the AP, skip and reset the timer if someone is connected
    if (WiFi.softAPgetStationNum() > 0) {
        lastRetryCheckAt = millis();

        return;
    }

    Log::instance()->info("Retrying WiFi network '%s'\n", ssid);
    startStation();
};

const char* WiFiController::getIp() {
    ipBuffer[0] = '\0';

    if (WiFi.getMode() & WIFI_MODE_AP) {
        snprintf(ipBuffer, sizeof(ipBuffer), "%s", WiFi.softAPIP().toString().c_str());
    } else if (WiFi.getMode() & WIFI_MODE_STA) {
        if (WiFi.status() != WL_CONNECTED) {
            Log::instance()->debug("Unable to get IP, WiFi not connected\n");

            return ipBuffer;
        }

        snprintf(ipBuffer, sizeof(ipBuffer), "%s", WiFi.localIP().toString().c_str());
    } else {
        Log::instance()->warning("No IP availble because not connected to a nerwork\n");
    }

    return ipBuffer;
};

wifi_mode_t WiFiController::getMode() {
    return WiFi.getMode();
};

const char* WiFiController::getUrl() {
    const char* ip = getIp();

    if (ip[0] == '\0') {
        urlBuffer[0] = '\0';

        return urlBuffer;
    }

    snprintf(urlBuffer, MAX_URL_LENGTH, "http://%s/", ip);

    return urlBuffer;
};

void WiFiController::registerEvents() {
    // Don't store credentials in NVS
    WiFi.persistent(false);
    // Autoreconnect gives up on some reasons (ex. AUTH_FAIL) and never stops on others, so we count retries ourselves
    WiFi.setAutoReconnect(false);
    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        onConnected();
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        onDisconnected(info.wifi_sta_disconnected.reason);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    eventsRegistered = true;
};

bool WiFiController::startStation() {
    failures = 0;
    Log::instance()->info("Connecting to WiFi network '%s'\n", ssid);

    if (!WiFi.mode(WIFI_STA)) {
        Log::instance()->error("Error setting STA mode\n");

        return false;
    }

    WiFi.begin(ssid, password);

    return true;
};

bool WiFiController::startAccessPoint() {
    // softAP() does not disable STA, so we change mode first
    if (!WiFi.mode(WIFI_AP) || !WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS)) {
        Log::instance()->error("Error starting AP mode\n");

        return false;
    }

    lastRetryCheckAt = millis();
    Log::instance()->info("Started network '%s' with IP %s\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());

    return true;
};

void WiFiController::onConnected() {
    failures = 0;
    Log::instance()->info("Connected to '%s' with IP %s (%d dBm)\n", ssid, WiFi.localIP().toString().c_str(), WiFi.RSSI());
};

void WiFiController::onDisconnected(uint8_t reason) {
    if (reason == WIFI_REASON_ASSOC_LEAVE) {
        return;
    }

    failures++;
    Log::instance()->warning("WiFi disconnected (reason %u: %s), failure %u/%u\n", reason, WiFi.disconnectReasonName((wifi_err_reason_t) reason), failures, MAX_CONNECT_FAILURES);

    if (failures < MAX_CONNECT_FAILURES) {
        WiFi.reconnect();

        return;
    }

    Log::instance()->warning("Unable to connect to '%s', falling back to AP mode\n", ssid);
    startAccessPoint();
};

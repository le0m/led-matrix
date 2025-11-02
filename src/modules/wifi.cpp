#include "wifi.h"

WiFiController::WiFiController() {
    urlBuffer[0] = '\0';
    ipBuffer[0] = '\0';
};

WiFiController::~WiFiController() {};

bool WiFiController::connect(const char* ssid, const char* password) {
    if (!disconnect()) {
        return false;
    }
    if (ssid[0] == '\0' || password[0] == '\0') {
        Log::instance()->info("Starting WiFi in AP mode\n");

        if (!WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS)) {
            Log::instance()->error("Error starting AP mode\n");

            return false;
        }

        Log::instance()->info("Started network '%s' with IP %s\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());

        return true;
    }

    Log::instance()->info("Starting WiFi in STA mode\n");
    if (!WiFi.mode(WIFI_STA)) {
        Log::instance()->error("Error setting STA mode\n");

        return false;
    }

    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);
    Log::instance()->info("Connecting to WiFi\n");
    wl_status_t s;
    uint8_t t = 0;
    do {
        t++;
        s = WiFi.status();
        delay(1000);
    } while (s != WL_CONNECTED && t <= MAX_TRIES);
    if (t > MAX_TRIES) {
        Log::instance()->warning("Timeout reached trying to connect to WiFi (status: %d)\nFallback to AP mode\n", s);

        return connect("", "");
    }

    Log::instance()->info("Connected to '%s' with IP %s (%d dBm)\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.getTxPower());

    return true;
};

bool WiFiController::disconnect() {
    if (WiFi.getMode() & WIFI_MODE_STA) {
        if (WiFi.status() == WL_CONNECTED) {
            if (!WiFi.disconnect()) {
                Log::instance()->error("Error disconnecting from STA mode\n");

                return false;
            }
        }
    }
    if (WiFi.getMode() & WIFI_MODE_AP) {
        if (!WiFi.softAPdisconnect()) {
            Log::instance()->error("Error disconnecting from AP mode\n");

            return false;
        }
    }

    return true;
};

const char* WiFiController::getIp() {
    ipBuffer[0] = '\0';

    if (WiFi.getMode() & WIFI_MODE_AP) {
        strncpy(ipBuffer, WiFi.softAPIP().toString().c_str(), MAX_IP_LENGTH);
    } else if (WiFi.getMode() & WIFI_MODE_STA) {
        if (WiFi.status() != WL_CONNECTED) {
            Log::instance()->debug("Unable to get IP, WiFi not connected\n");

            return ipBuffer;
        }

        strncpy(ipBuffer, WiFi.localIP().toString().c_str(), MAX_IP_LENGTH);
    } else {
        Log::instance()->warning("No IP availble because not connected to a nerwork\n");
    }

    ipBuffer[MAX_IP_LENGTH - 1] = '\0';

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

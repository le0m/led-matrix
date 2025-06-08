#include "wifi.h"

WiFiController::WiFiController() {};

WiFiController::~WiFiController() {};

bool WiFiController::connect(String ssid, String password) {
    if (!disconnect()) {
        return false;
    }
    if (ssid == "" || password == "") {
        Log::instance()->info("Starting WiFi in AP mode\n");

        if (!WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS)) {
            Log::instance()->error("Error starting AP mode\n");

            return false;
        }

        Log::instance()->info("Started network '%s' with IP %s\n", WiFi.softAPSSID().c_str(), WiFi.softAPIP().toString().c_str());

        return true;
    }

    Log::instance()->info("Stating WiFi in STA mode\n");
    if (!WiFi.mode(WIFI_STA)) {
        Log::instance()->error("Error setting STA mode\n");

        return false;
    }

    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid.c_str(), password.c_str());
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

String WiFiController::getIp() {
    if (WiFi.getMode() & WIFI_MODE_AP) {
        return WiFi.softAPIP().toString();
    }
    if (WiFi.getMode() & WIFI_MODE_STA) {
        if (WiFi.status() != WL_CONNECTED) {
            return String("");
        }

        return WiFi.localIP().toString();
    }

    Log::instance()->warning("No IP availble because not connected to a nerwork\n");

    return String("");
};

wifi_mode_t WiFiController::getMode() {
    return WiFi.getMode();
};

String WiFiController::getUrl() {
    String ip = getIp();
    if (ip == "") {
        return ip;
    }

    String url = "http://";
    url.concat(ip);

    return url;
};

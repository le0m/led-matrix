#ifndef _WIFI_H
#define _WIFI_H

#include "log.h"
#include <WiFi.h>

#define SETUP_AP_SSID "esp32-led-matrix"
#define SETUP_AP_PASS "daftpunk"
#define MAX_TRIES 15
#define MAX_URL_LENGTH 32

class WiFiController {
    private:
        char urlBuffer[MAX_URL_LENGTH];

    public:
        WiFiController();
        ~WiFiController();
        bool connect(const char* = "", const char* = "");
        bool disconnect();
        const char* getIp();
        wifi_mode_t getMode();
        const char* getUrl();
};

#endif

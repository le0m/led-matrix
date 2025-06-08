#ifndef _WIFI_H
#define _WIFI_H

#include "log.h"
#include <WiFi.h>

#define SETUP_AP_SSID "esp32-led-matrix"
#define SETUP_AP_PASS "daftpunk"
#define MAX_TRIES 15

class WiFiController {
    public:
        WiFiController();
        ~WiFiController();
        bool connect(String = "", String = "");
        bool disconnect();
        String getIp();
        wifi_mode_t getMode();
        String getUrl();
};

#endif

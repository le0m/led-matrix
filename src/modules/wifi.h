#ifndef _WIFI_H
#define _WIFI_H

#include "log.h"
#include <WiFi.h>

#define SETUP_AP_SSID "esp32-led-matrix"
#define SETUP_AP_PASS "daftpunk"
#define MAX_CONNECT_FAILURES 10
#define AP_RETRY_INTERVAL_MS 300000
#define MAX_SSID_LENGTH 32
#define MAX_PASSWORD_LENGTH 64
#define MAX_URL_LENGTH 32
#define MAX_IP_LENGTH 15 // IPv4

class WiFiController {
    private:
        char ssid[MAX_SSID_LENGTH + 1];
        char password[MAX_PASSWORD_LENGTH + 1];
        bool eventsRegistered;
        uint8_t failures;
        ulong lastRetryCheckAt;
        char urlBuffer[MAX_URL_LENGTH];
        char ipBuffer[MAX_IP_LENGTH + 1];

        void registerEvents();
        bool startStation();
        bool startAccessPoint();
        void onConnected();
        void onDisconnected(uint8_t);

    public:
        WiFiController();
        bool connect(const char* = "", const char* = "");
        void loop();
        const char* getIp();
        wifi_mode_t getMode();
        const char* getUrl();
};

#endif

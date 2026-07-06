#ifndef _MONITOR_H
#define _MONITOR_H

#include <ESPAsyncWebServer.h>
#include <freertos/task.h>
#include "wifi.h"
#include "monitor.h"
#include "log.h"
#include "filesystem.h"

#define MONITOR_FILE        "/status.log"
#define MONITOR_REC_SIZE    48
#define MONITOR_MAX_RECS    4096
#define MONITOR_INTERVAL_MS 60000UL

class Monitor {
    public:
        Monitor(WiFiController* wifi);
        void initServer(AsyncWebServer* server);
        void setEnabled(bool enabled);

    private:
        WiFiController* wifi = nullptr;
        TaskHandle_t task = nullptr;
        uint16_t head = 0;
        uint16_t count = 0;

        void writeRecord();
        char wifiChar();
        static void taskFn(void* param);
};

#endif

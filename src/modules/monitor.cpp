#include "monitor.h"

Monitor::Monitor(WiFiController* w) : wifi(w) {
    File f = LittleFS.open(MONITOR_FILE, "r");
    if (f) {
        uint16_t records = (uint16_t)(f.size() / MONITOR_REC_SIZE);
        f.close();
        count = min(records, (uint16_t)MONITOR_MAX_RECS);
        head = count % MONITOR_MAX_RECS;
        Log::instance()->info("Monitor found %d existing records\n", count);
    }
}

void Monitor::initServer(AsyncWebServer* server) {
    server->on("/monitor", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!Filesystem::pathExists(MONITOR_FILE)) {
            request->send(404);

            return;
        }

        request->send(LittleFS, MONITOR_FILE, "text/plain");
    });
}

void Monitor::setEnabled(bool enabled) {
    if (enabled && task == nullptr) {
        xTaskCreate(taskFn, "monitor", 4096, this, 1, &task);
        Log::instance()->info("Monitor started\n");
    } else if (!enabled && task != nullptr) {
        vTaskDelete(task);
        task = nullptr;
        Log::instance()->info("Monitor stopped\n");
    }
}

char Monitor::wifiChar() {
    wifi_mode_t mode = wifi->getMode();
    switch (mode) {
        case WIFI_MODE_APSTA:
            return 'B';

        case WIFI_MODE_STA:
            return 'S';

        case WIFI_MODE_AP:
            return 'A';

        default:
            return 'X';
    }
}

void Monitor::writeRecord() {
    char buf[MONITOR_REC_SIZE];
    char tmp[MONITOR_REC_SIZE];

    int written = snprintf(
        tmp,
        sizeof(tmp),
        "%lu f=%lu m=%lu b=%lu w=%c",
        (unsigned long)millis(),
        (unsigned long)ESP.getFreeHeap(),
        (unsigned long)ESP.getMinFreeHeap(),
        (unsigned long)ESP.getMaxAllocHeap(),
        wifiChar()
    );
    if (written <= 0) {
        Log::instance()->error("Error writing monitor record to buffer\n");

        return;
    }
    if (written >= MONITOR_REC_SIZE) {
        Log::instance()->error("Buffered monitor record is %d bytes, maximum is %d bytes\n", written, MONITOR_REC_SIZE);

        return;
    }

    memset(buf, ' ', MONITOR_REC_SIZE - 1);
    buf[MONITOR_REC_SIZE - 1] = '\n';
    memcpy(buf, tmp, written);
    const char* mode = (count == 0) ? "w" : "r+";
    File f = LittleFS.open(MONITOR_FILE, mode);
    if (!f) {
        Log::instance()->error("Error opening monitor file %s\n", MONITOR_FILE);

        return;
    }

    f.seek((size_t)head * MONITOR_REC_SIZE);
    f.write((const uint8_t*)buf, MONITOR_REC_SIZE);
    f.close();

    head = (head + 1) % MONITOR_MAX_RECS;
    if (count < MONITOR_MAX_RECS) {
        count++;
    }
}

void Monitor::taskFn(void* param) {
    Monitor* self = static_cast<Monitor*>(param);
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(MONITOR_INTERVAL_MS));
        self->writeRecord();
    }
}

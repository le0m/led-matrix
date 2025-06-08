#include "ota.h"

OTA::OTA(uint32_t delay) {
    rebootDelay = delay;
};

OTA::~OTA() {};

void OTA::initServer(AsyncWebServer* server) {
    server->on("/ota", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (!request->hasParam("mode")) {
            Log::instance()->error("Missing mode parameter\n");
            request->send(400, "text/plain", "missing mode parameter");

            return;
        }

        uint8_t mode = U_FLASH;
        String modeParam = request->getParam("mode")->value();
        if (modeParam == "filesystem") {
            mode = U_SPIFFS;
        } else if (modeParam == "firmware") {
            mode = U_FLASH;
        } else {
            Log::instance()->error("Unknown update mode %s\n", modeParam);
            request->send(400, "text/plain", "unknown mode");

            return;
        }

        if (request->hasParam("md5")) {
            String hash = request->getParam("md5")->value();
            Log::instance()->info("Setting MD5 hash: %s\n", hash);
            if (!Update.setMD5(hash.c_str())) {
                Log::instance()->error("Error setting MD5 update hash\n");
                request->send(500, "text/plain", "error setting MD5 hash");

                return;
            }
        }

        if (!request->hasParam("size")) {
            Log::instance()->error("Missing size parameter\n");
            request->send(400, "text/plain", "missing size parameter");

            return;

        }

        size_t size = UPDATE_SIZE_UNKNOWN;
        String sizeParam = request->getParam("size")->value();
        sscanf(sizeParam.c_str(), "%zu", &size); // string to size_t
        if (!Update.begin(size, mode) || Update.hasError()) {
            Log::instance()->error("Error starting update: %s\n", Update.errorString());
            request->send(500, "text/plain", "error starting update");

            return;
        }

        request->send(204);
    });
    server->on("/ota", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            Log::instance()->info("Update received\n");
            if (Update.hasError()) {
                Log::instance()->error("Error completing update: %s\n", Update.errorString());
                request->send(500, "text/plain", "error completing update");

                return;
            }

            reboot = millis();
            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            Log::instance()->trace("Received update chunk: %d-%d (%d bytes)\n", index, index + len, len);
            if (len > 0) {
                if (Update.write(data, len) != len) {
                    Log::instance()->error("Error writing update chunk: %s\n", Update.errorString());
                    request->send(500, "text/plain", "error writing chunk");

                    return;
                }
            }
            if (index + len == total && !Update.end()) {
                Log::instance()->error("Error ending update: %s\n", Update.errorString());
                request->send(500, "text/plain", "error ending update");
            }
        }
    );
};

void OTA::loop() {
    if (reboot > 0 && millis() - reboot > rebootDelay) {
        Log::instance()->info("Rebooting to update\n");
        reboot = 0;
        ESP.restart();
    }
};

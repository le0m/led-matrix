#include "config.h"

Config::Config(ConfigChangeHandler handler) {
    configChangeHandler = handler;
};

Config::~Config() {
    newCfg.clear();
    current.clear();
};

bool Config::save(const char *path) {
    return writeConfig(path, current);
};

bool Config::load(const char *path) {
    current = readConfig(path);

    return !current.isNull();
};

// Asynchronously update configuration
void Config::taskRunner(void *p) {
    Config *t = (Config*) p;
    if (t->configChangeHandler) {
        t->configChangeHandler(t->newCfg);
    }

    t->current.clear();
    t->current = t->newCfg;
    t->newCfg.clear();
    t->save();

    vTaskDelete(NULL);
};

void Config::initServer(AsyncWebServer *server) {
    server->on("/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Log::instance()->info("Sending current configuration\n");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(current, *response);
        response->setCode(200);
        request->send(response);
    });
    server->on("/config", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            Log::instance()->info("Config JSON received\n");
            request->send(204);
            deserializeJson(newCfg, reqBody);
            free(reqBody);
            delete reqBody;
            xTaskCreate(taskRunner, "Update config", 8192, this, 0, NULL);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // Allocate memory for whole body on first chunk
            if (index == 0) {
                // Cleanup if variable was already used
                if (reqBody != nullptr) {
                    free(reqBody);
                    delete reqBody;
                }

                reqBody = (char*)malloc(total);
            }

            Log::instance()->trace("Received config JSON chunk: %d-%d (%d / %d bytes)\n", index, index + len, len, total);
            memcpy(reqBody + index, data, len);
        }
    );
};

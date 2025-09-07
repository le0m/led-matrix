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
    Config::merge(t->current, t->newCfg);
    t->newCfg.clear();
    t->save();

    vTaskDelete(NULL);
};

void Config::merge(JsonVariant dst, JsonVariantConst src) {
    if (src.is<JsonObjectConst>()) {
        for (JsonPairConst kvp : src.as<JsonObjectConst>()) {
            if (dst[kvp.key()]) {
                Config::merge(dst[kvp.key()], kvp.value());

                return;
            }

            dst[kvp.key()] = kvp.value();
        }

        return;
    }

    if (!src.isNull() && strcmp(src.as<const char*>(), "") != 0) {
        dst.set(src);
    }
};

void Config::initServer(AsyncWebServer *server) {
    server->on("/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Log::instance()->info("Sending current configuration\n");
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(current, *response);
        response->setCode(200);
        request->send(response);
    });
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/config", [&](AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->methodToString() != "POST") {
            request->send(405);

            return;
        }

        Log::instance()->info("Config JSON received\n");
        request->send(204);
        JsonObject obj = json.as<JsonObject>();
        newCfg.set(obj);
        xTaskCreate(taskRunner, "Update config", 8192, this, 0, NULL);
    });
    server->addHandler(handler);
};

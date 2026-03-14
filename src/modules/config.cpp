#include "config.h"

Config::Config(ConfigChangeHandler handler) {
    configChangeHandler = handler;
};

Config::~Config() {
    newCfg.clear();
    current.clear();
};

bool Config::save(const char *path) {
    return Filesystem::writeConfig(path, current);
};

bool Config::load(const char *path) {
    current = Filesystem::readConfig(path);

    return !current.isNull();
};

void Config::asyncUpdateConfig(void *p) {
    Config *t = (Config*) p;
    // Merge new config with current for partial updates
    JsonDocument merged;
    merged.set(t->current);
    Config::merge(merged.as<JsonVariant>(), t->newCfg.as<JsonVariantConst>());
    if (t->configChangeHandler) {
        t->configChangeHandler(merged);
    }

    t->newCfg.clear();
    t->current.clear();
    t->current.set(merged);
    t->save();
    vTaskDelete(NULL);
};

void Config::merge(JsonVariant dst, JsonVariantConst src) {
    if (!src.is<JsonObjectConst>()) {
        dst.set(src);

        return;
    }

    for (JsonPairConst kvp : src.as<JsonObjectConst>()) {
        if (dst[kvp.key()]) {
            Config::merge(dst[kvp.key()], kvp.value());
        } else {
            dst[kvp.key()] = kvp.value();
        }
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
        if (strcasecmp(request->methodToString(), "POST") != 0) {
            request->send(405);

            return;
        }

        Log::instance()->info("Config JSON received\n");
        request->send(204);
        newCfg.set(json);
        xTaskCreate(asyncUpdateConfig, "Update config", 8192, this, 0, NULL);
    });
    server->addHandler(handler);
};

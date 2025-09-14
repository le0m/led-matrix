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
    // Merge new config with current for partial updates
    JsonDocument merged = t->current;
    Config::merge(merged.as<JsonVariant>(), t->newCfg.as<JsonVariantConst>());
    if (t->configChangeHandler) {
        t->configChangeHandler(merged);
    }

    t->newCfg.clear();
    t->current.clear();
    t->current = merged;
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
        if (request->methodToString() != "POST") {
            request->send(405);

            return;
        }

        Log::instance()->info("Config JSON received\n");
        request->send(204);
        JsonObject obj = json.as<JsonObject>();
        newCfg.set(obj);
        xTaskCreate(taskRunner, "Update config", 8192, this, 0, NULL);
        newCfg.set(json);
    });
    server->addHandler(handler);
};

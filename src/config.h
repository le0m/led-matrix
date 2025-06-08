#ifndef _CONFIG_H
#define _CONFIG_H

#include "log.h"
#include "filesystem.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

typedef std::function<void(JsonDocument cfg)> ConfigChangeHandler;

class Config {
    private:
        JsonDocument newCfg;
        char *reqBody = nullptr;
        ConfigChangeHandler configChangeHandler;
        static void taskRunner(void*);

    public:
        JsonDocument current;
        Config(ConfigChangeHandler);
        ~Config();
        void initServer(AsyncWebServer*);
        bool save(const char* = "/config.json");
        bool load(const char* = "/config.json");
};

#endif

#ifndef _OTA_H
#define _OTA_H

#include "Arduino.h"
#include "log.h"
#include "stdlib_noniso.h"
#include <functional>
#include "FS.h"
#include "Update.h"
#include "StreamString.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

#if !defined(ESP32)
#error "This OTA module supports esp32 only"
#endif

class OTA {
    private:
        ulong reboot = 0;
        uint32_t rebootDelay = 1000;

    public:
        OTA(uint32_t);
        ~OTA();
        void initServer(AsyncWebServer*);
        void loop();
};

#endif


#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "modules/log.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

#define MAX_FILE_SIZE 1048576 // 1 MiB

class Filesystem {
    public:
        static void tree(const char*, uint8_t);
        static bool pathExists(const char*);
        static bool deleteFile(const char*);
        static bool writeBytes(const char*, uint8_t*, size_t);
        static bool readBytes(const char*, uint8_t*, size_t);
        static bool writeConfig(const char*, JsonDocument&);
        static JsonDocument readConfig(const char*);
};

#endif

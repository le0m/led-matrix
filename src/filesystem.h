#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "log.h"
#include <LittleFS.h>
// #include <sys/stat.h>
#include <ArduinoJson.h>

#define MAX_GIF_SIZE 1048576 // 1 MiB

bool pathExists(const char*);
bool writeBytes(const char*, uint8_t*, size_t);
bool readBytes(const char*, uint8_t*, size_t);
bool deleteFile(const char*);
bool writeConfig(const char*, JsonDocument);
JsonDocument readConfig(const char*);

#endif

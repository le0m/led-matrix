#include "filesystem.h"

// see readme "Note on PlatformIO"
bool pathExists(const char *path) {
    static struct stat pathStat;

    return stat((String("/littlefs")+String(path)).c_str(), &pathStat) == 0;
};

bool deleteFile(const char *path) {
    if (!pathExists(path)) {
        return true;
    }

    return LittleFS.remove(path);
};

bool writeBytes(const char *path, uint8_t *buf, size_t size) {
    File file = LittleFS.open(path, FILE_APPEND);
    if (!file) {
        Log::instance()->error("Error opening file: %s\n", path);

        return false;
    }

    size_t w = file.write(buf, size);
    if (w != size) {
        file.close();
        Log::instance()->error("Error writing file: %s\n", path);
        Log::instance()->trace("Error writing file, written %d bytes instead of %d\n", w, size);

        return false;
    }

    file.close();

    return true;
};

bool readBytes(const char *path, uint8_t *buf, size_t size) {
    if (!pathExists(path)) {
        return false;
    }


    File file = LittleFS.open(path);
    if (!file) {
        Log::instance()->error("Error opening file: %s\n", path);

        return false;
    }

    size_t r = file.read(buf, size);
    if (r != size) {
        file.close();
        Log::instance()->error("Error reading file: %s\n", path);
        Log::instance()->trace("Error reading file, read %d bytes instead of %d\n", r, size);

        return false;
    }

    file.close();

    return true;
};

bool writeConfig(const char *path, JsonDocument cfg) {
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Log::instance()->error("Error opening file: %s\n", path);

        return false;
    }

    if (!serializeJson(cfg, file)) {
        file.close();
        Log::instance()->error("Error writing configuration\n");

        return false;
    }

    file.close();

    return true;
};

JsonDocument readConfig(const char *path) {
    JsonDocument cfg;
    if (!pathExists(path)) {
        Log::instance()->error("Config file does not exist: %s\n", path);

        return cfg;
    }

    File file = LittleFS.open(path);
    if (!file) {
        Log::instance()->error("Error opening file: %s\n", path);

        return cfg;
    }

    DeserializationError e = deserializeJson(cfg, file);
    if (e) {
        Log::instance()->error("Error reading configuration: %s\n", e.c_str());
    }

    file.close();

    return cfg;
};

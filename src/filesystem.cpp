#include "filesystem.h"

void Filesystem::tree(const char *path, uint8_t depth) {
    File root = LittleFS.open(path);

    if (!root) {
        Log::instance()->error("error opening path: %s\n", path);

        return;
    }

    if (!root.isDirectory()) {
        Log::instance()->error("path is not a directory: %s\n", path);
        root.close();

        return;
    }

    Log::instance()->info("%s + %s\n", std::string(depth > 0 ? depth - 1 : depth, ' ').c_str(), root.name());
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Filesystem::tree(file.path(), depth + 1);
        } else {
            Log::instance()->info("%s |- %s\n", std::string(depth, ' ').c_str(), file.name());
        }

        file.close();
        file = root.openNextFile();
    }

    root.close();
};

// see readme "Note on PlatformIO"
bool Filesystem::pathExists(const char *path) {
    static struct stat pathStat;
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "/littlefs%s", path);

    return stat(fullPath, &pathStat) == 0;
};

bool Filesystem::deleteFile(const char *path) {
    if (!Filesystem::pathExists(path)) {
        return true;
    }

    return LittleFS.remove(path);
};

bool Filesystem::writeBytes(const char *path, uint8_t *buf, size_t size) {
    File file = LittleFS.open(path, FILE_APPEND);
    if (!file) {
        Log::instance()->error("Error opening file for write: %s\n", path);

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

bool Filesystem::readBytes(const char *path, uint8_t *buf, size_t size) {
    if (!Filesystem::pathExists(path)) {
        return false;
    }

    File file = LittleFS.open(path);
    if (!file) {
        Log::instance()->error("Error opening file for read: %s\n", path);

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

bool Filesystem::writeConfig(const char *path, JsonDocument& cfg) {
    File file = LittleFS.open(path, FILE_WRITE);
    if (!file) {
        Log::instance()->error("Error opening config for write: %s\n", path);

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

JsonDocument Filesystem::readConfig(const char *path) {
    JsonDocument cfg;
    if (!Filesystem::pathExists(path)) {
        Log::instance()->error("Config file does not exist: %s\n", path);

        return cfg;
    }

    File file = LittleFS.open(path);
    if (!file) {
        Log::instance()->error("Error opening config for read: %s\n", path);

        return cfg;
    }

    DeserializationError e = deserializeJson(cfg, file);
    if (e) {
        Log::instance()->error("Error reading configuration: %s\n", e.c_str());
    }

    file.close();

    return cfg;
};

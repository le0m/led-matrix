#include "map.h"

Map::Map(uint8_t w, uint8_t h) {
    width = w;
    height = h;
};

Map::~Map() {
    jpeg.close();
};

void Map::print() {};

bool Map::openImage() {
    if (isOpen) {
        return true;
    }

    isOpen = true;

    return jpeg.open(MAP_PATH, openFile, closeFile, readFile, seekFile, draw);
};

void Map::closeImage() {
    if (!isOpen) {
        return;
    }

    isOpen = false;
    delay(50); // wait for possible renderFrame() execution to finish
    jpeg.close();
    lastRender = 0;
};

void Map::setConfig(JsonVariantConst c) {
    config.clear();
    config.set(c);
    forceCropUpdate = true;
    Log::instance()->info("Updated map configuration\n");
};

void Map::initServer(AsyncWebServer *server) {
    server->on("/map", HTTP_GET, [&](AsyncWebServerRequest *request) {
        const AsyncWebHeader *accept = request->getHeader("Accept");
        if (accept->value() != "image/jpeg") {
            Log::instance()->error("Map request has unhandled accept: %s\n", accept->value());
            request->send(400, "text/plain", "unhandled accept type");

            return;
        }
        if (!pathExists(MAP_PATH)) {
            Log::instance()->info("No map currently stored\n");
            request->send(204);

            return;
        }

        Log::instance()->info("Sending current map\n");
        request->send(LittleFS, MAP_PATH, "image/jpeg");
    });
    server->on("/map", HTTP_DELETE, [&](AsyncWebServerRequest *request) {
        closeImage();
        Log::instance()->info("Deleting map\n");
        if (!deleteFile(MAP_PATH)) {
            Log::instance()->error("Error deleting map\n");
            request->send(500, "text/plain", "error deleting map");

            return;
        }

        Log::instance()->info("Map deleted\n");
        request->send(204);
    });
    server->on("/map", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            Log::instance()->info("Map received\n");
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType->value() != "image/jpeg") {
                Log::instance()->error("Uploaded map has unhandled content-type: %s\n", contentType->value());
                request->send(400, "text/plain", "unhandled content type");

                return;
            }

            openImage();
            forceCropUpdate = true;
            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType->value() != "image/jpeg") {
                Log::instance()->error("Uploaded map has unhandled content-type: %s\n", contentType->value());
                request->send(400, "text/plain", "unhandled content type");

                return;
            }
            if (index == 0) {
                Log::instance()->debug("Receiving map: %d bytes\n", total);
                if (total > MAX_IMAGE_SIZE) {
                    Log::instance()->error("Map size %d exceeds maximum size %d\n", total, MAX_IMAGE_SIZE);
                    request->send(400, "text/plain", "map is too big");

                    return;
                }
                if (total > LittleFS.totalBytes() - LittleFS.usedBytes()) {
                    Log::instance()->error("Map size %d is within limit, but disk has not enough free space (%d)\n", total, LittleFS.totalBytes() - LittleFS.usedBytes());
                    request->send(500, "text/plain", "not enough free space");

                    return;
                }

                closeImage();
                if (!deleteFile(MAP_PATH)) {
                    Log::instance()->warning("Error deleting previous map\n");
                    request->send(500, "text/plain", "error deleting previous map");

                    return;
                }

                Log::instance()->debug("Previous map deleted or not present\n");
            }

            Log::instance()->trace("Received map chunk: %d-%d (%d bytes)\n", index, index + len, len);
            if (!writeBytes(MAP_PATH, data, len)) {
                Log::instance()->error("Error writing map chunk\n");
                request->send(500, "text/plain", "error writing map");

                return;
            }
        }
    );
};

void Map::loadMedia() {
    if (!pathExists(MAP_PATH)) {
        Log::instance()->info("No map stored in FLASH\n");

        return;
    }
    if (!openImage()) {
        Log::instance()->error("Error opening map from FLASH\n");

        return;
    }

    forceCropUpdate = true;
    Log::instance()->info("Loaded map from FLASH\n");
};

bool Map::setHeaders(HTTPClient &client, const char *headers) {
    JsonDocument h;
    DeserializationError err = deserializeJson(h, headers);
    if (err) {
        Log::instance()->error("Error deserializing headers: %s\n", err.c_str());

        return false;
    }
    if (!h.is<JsonObjectConst>()) {
        Log::instance()->error("Header is not a JSON object\n");

        return false;
    }

    for (JsonPairConst kvp : h.as<JsonObjectConst>()) {
        String key(kvp.key().c_str());
        std::string value = kvp.value().as<std::string>();
        if (key.equalsIgnoreCase("Authorization")) {
            size_t i = value.find(' ');
            if (i == std::string::npos) {
                Log::instance()->error("Missing header authorization type or value\n");

                return false;
            }

            client.setAuthorizationType(value.substr(0, i).c_str());
            client.setAuthorization(value.substr(i + 1).c_str());
            continue;
        }
        if (key.equalsIgnoreCase("User-Agent")) {
            client.setUserAgent(value.c_str());
            continue;
        }
        if (key.equalsIgnoreCase("Connection")) {
            client.setReuse(strcmp(value.c_str(), "close") != 0);
            continue;
        }
        if (key.equalsIgnoreCase("Host")) {
            continue;
        }

        client.addHeader(key.c_str(), value.c_str());
    }

    return true;
};

std::array<double, 2> Map::matchRegex(String res, const char *r) {
    std::array<double, 2> pos = { 0, 0 };
    std::regex regex(r);
    std::smatch matches;
    std::string str = std::string(res.c_str());
    if (!std::regex_search(str, matches, regex)) {
        Log::instance()->error("Regex %s did not match position API response:\n%s\n\n", r, str.c_str());

        return pos;
    }
    if (matches.size() != 3) {
        Log::instance()->error("Regex %s found %d matches in position API response, need 2:\n%s\n\n", r, matches.size() - 1, str.c_str());

        return pos;
    }

    pos[0] = strtod(matches.str(1).c_str(), NULL);
    pos[1] = strtod(matches.str(2).c_str(), NULL);

    return pos;
};

std::array<double, 2> Map::getPositionFromAPI() {
    std::array<double, 2> pos = { 0, 0 };
    WiFiClient client;
    HTTPClient http;
    http.setReuse(false);
    http.setConnectTimeout(10000);
    http.setTimeout(60000);
    String u = config["url"].as<String>();
    String m = config["method"].as<String>();
    const char *r = config["regex"].as<const char*>();
    if (u.isEmpty()) {
        Log::instance()->error("Position API URL is not set\n");

        return pos;
    }
    if (m.isEmpty()) {
        Log::instance()->error("Position API method is not set\n");

        return pos;
    }
    if (strcmp(r, "") == 0) {
        Log::instance()->error("Position API regexp is not set\n");

        return pos;
    }

    // Apply query params
    String b = config["body"].as<String>();
    if (m.equalsIgnoreCase("GET") && !b.isEmpty()) {
        if (!u.concat(b)) {
            Log::instance()->error("Error adding query parameters\n");

            return pos;
        }
    }

    http.begin(client, u);
    // Apply headers
    const char *hs = config["headers"].as<const char*>();
    if (strcmp(hs, "") != 0 && !setHeaders(http, hs)) {
        return pos;
    }

    // Send request
    int code;
    if (m.equalsIgnoreCase("POST")) {
        code = http.POST(b);
    } else {
        code = http.GET();
    }
    if (code < 0) {
        Log::instance()->error("Error in API request: %d %s\n", code, http.errorToString(code).c_str());

        return pos;
    }

    // Match regex
    String res = http.getString();
    pos = matchRegex(res, r);

    return pos;
};

std::array<double, 2> Map::getPosition() {
    std::array<double, 2> pos = { 0, 0 };
    if (
        strcmp(config["url"].as<const char*>(), "") != 0
        && strcmp(config["method"].as<const char*>(), "") != 0
        && strcmp(config["regex"].as<const char*>(), "") != 0
    ) {
        pos = getPositionFromAPI();
        Log::instance()->info("Getting position from API (%s): %f, %f\n", config["url"].as<const char*>(), pos[0], pos[1]);
    }
    // default to static position, if available
    if (
        pos[0] == 0 && pos[1] == 0
        && strcmp(config["latitude"].as<const char*>(), "") != 0
        && strcmp(config["longitude"].as<const char*>(), "") != 0
    ) {
        pos[0] = strtod(config["latitude"].as<const char*>(), NULL);
        pos[1] = strtod(config["longitude"].as<const char*>(), NULL);
        Log::instance()->info("Getting position from configuration: %f, %f\n", pos[0], pos[1]);
    }

    return pos;
};

void Map::asyncUpdateCrop(void *p) {
    Map *m = (Map*) p;
    Log::instance()->debug("Updating crop asynchronously\n");
    m->updateCrop();
    m->lastCropUpdate = millis();
    m->forceCropUpdate = false;
    m->isUpdating = false;
    vTaskDelete(NULL);
};

void Map::updateCrop() {
    if (!isOpen) {
        return;
    }

    Log::instance()->info("Updating map crop area and position\n");
    // Crop area
    int imgW = jpeg.getWidth();
    int imgH = jpeg.getHeight();
    std::array<double, 2> position = getPosition();
    double x = ((position[1] + 180.0) / 360.0) * imgW;
    double y = ((90.0 - position[0]) / 180.0) * imgH;
    int col = static_cast<int>(x / width);
    int row = static_cast<int>(y / height);
    cropArea[0] = width * col;
    cropArea[1] = height * row;
    // Position
    cropPosition[0] = x - cropArea[0];
    cropPosition[1] = y - cropArea[1];
    // Offset position to fit the point in the panel
    cropPosition[0] -= std::max(0, (cropPosition[0] + config["pointSize"].as<uint8_t>()) - width);
    cropPosition[1] -= std::max(0, (cropPosition[1] + config["pointSize"].as<uint8_t>()) - height);
};

void Map::render(MatrixPanel_I2S_DMA *display) {
    if (!isOpen) {
        display->clearScreen();

        return;
    }
    // Re-render at most once a second and avoid running while updating position
    if (isUpdating || millis() - lastRender < 1000) {
        return;
    }
    // Update crop area (and position) once every 15 minutes
    if (forceCropUpdate || millis() - lastCropUpdate > UPDATE_CROP_AREA) {
        isUpdating = true;
        xTaskCreate(asyncUpdateCrop, "Update crop area", 8192, this, 0, NULL);

        return;
    }

    // Re-open image to rewind file position, otherwise calling decode() more than once corrupts the displayed image
    jpeg.open(MAP_PATH, openFile, closeFile, readFile, seekFile, draw);
    lastRender = millis();
    drawUserData data = {
        display,
        cropPosition,
        config["pointSize"].as<uint8_t>(),
        config["pointColor"].as<uint16_t>(),
        config["trackColor"].as<uint16_t>()
    };
    jpeg.setUserPointer(static_cast<void*>(&data));
    jpeg.setCropArea(cropArea[0], cropArea[1], width, height);
    if (!jpeg.decode(0, 0, 0)) {
        Log::instance()->error("Error decoding JPEG: %d\n", jpeg.getLastError());
    }

    jpeg.close();
};

void* Map::openFile(const char *path, int32_t *size) {
    File *file = new File();
    *file = LittleFS.open(path);
    if (file) {
        *size = file->size();

        return static_cast<void*>(file);
    }

    delete file;

    return NULL;
};

void Map::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) {
        f->close();
        delete f;
    }
};

int32_t Map::readFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen) {
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    }
    if (iBytesRead <= 0) {
        return 0;
    }

    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();

    return iBytesRead;
};

int32_t Map::seekFile(JPEGFILE *pFile, int32_t iPosition) {
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();

    return pFile->iPos;
};

int Map::draw(JPEGDRAW *pDraw) {
    drawUserData *data = static_cast<drawUserData*>(pDraw->pUser);
    uint16_t *s = pDraw->pPixels;
    int lastX = pDraw->x + pDraw->iWidth;
    int lastY = pDraw->y + pDraw->iHeight;
    for (int y = pDraw->y; y < lastY; y++) {
        for (int x = pDraw->x; x < lastX; x++) {
            data->display->drawPixel(x, y, *s++);
        }
    }

    // Draw position
    for (int y = 0; y < data->size; y++) {
        for (int x = 0; x < data->size; x++) {
            data->display->drawPixel(data->position[0] + x, data->position[1] + y, data->color);
        }
    }

    return 1;
};

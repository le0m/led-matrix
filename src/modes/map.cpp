#include "map.h"

Map::Map(uint8_t w, uint8_t h) {
    width = w;
    height = h;
};

Map::~Map() {
    close();
};

bool Map::open() {
    if (isOpen) {
        return true;
    }

    if (!Filesystem::pathExists(MAP_PATH)) {
        Log::instance()->trace("No map stored\n");

        return false;
    }

    if (!jpeg.open(MAP_PATH, openFile, closeFile, readFile, seekFile, draw)) {
        Log::instance()->error("Error opening map\n");

        return false;
    }

    lastRender = 0;
    isOpen = true;
    Log::instance()->info("Loaded map\n");

    return true;
};

bool Map::close() {
    if (!isOpen) {
        return true;
    }

    isOpen = false;
    delay(50); // wait for possible renderFrame() execution to finish
    jpeg.close();
    lastRender = 0;

    return true;
};

void Map::print() {};

void Map::setConfig(JsonVariantConst c) {
    config.clear();
    config.set(c);
    forceCropUpdate = true;
    Log::instance()->info("Updated map configuration\n");
};

void Map::initServer(AsyncWebServer *server) {
    server->on("/map", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (!Filesystem::pathExists(MAP_PATH)) {
            Log::instance()->info("No map currently stored\n");
            request->send(204);

            return;
        }

        Log::instance()->info("Sending current map\n");
        request->send(LittleFS, MAP_PATH, "image/jpeg");
    });
    server->on("/map", HTTP_DELETE, [&](AsyncWebServerRequest *request) {
        close();
        Log::instance()->info("Deleting map\n");
        if (!Filesystem::deleteFile(MAP_PATH)) {
            Log::instance()->error("Error deleting map\n");
            request->send(500, "text/plain", "error deleting map");

            return;
        }

        request->send(204);
    });
    server->on("/map", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            Log::instance()->info("Map received\n");
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType == nullptr) {
                Log::instance()->error("Uploaded map has no content-type header");
                request->send(400, "text/plain", "no content-type header");

                return;
            }
            if (contentType->value() != "image/jpeg") {
                Log::instance()->error("Uploaded map has unhandled content-type: %s\n", contentType->value());
                request->send(400, "text/plain", "unhandled content type");

                return;
            }

            isUpdating = false;
            forceCropUpdate = true;
            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType == nullptr) {
                Log::instance()->error("Uploaded map has no content-type header");
                request->send(400, "text/plain", "no content-type header");

                return;
            }
            if (contentType->value() != "image/jpeg") {
                Log::instance()->error("Uploaded map has unhandled content-type: %s\n", contentType->value());
                request->send(400, "text/plain", "unhandled content type");

                return;
            }
            if (index == 0) {
                Log::instance()->debug("Receiving map: %d bytes\n", total);
                if (total > MAX_FILE_SIZE) {
                    Log::instance()->error("Map size %d exceeds maximum size %d\n", total, MAX_FILE_SIZE);
                    request->send(400, "text/plain", "map is too big");

                    return;
                }
                if (total > LittleFS.totalBytes() - LittleFS.usedBytes()) {
                    Log::instance()->error("Map size %d is within limit, but disk has not enough free space (%d)\n", total, LittleFS.totalBytes() - LittleFS.usedBytes());
                    request->send(500, "text/plain", "not enough free space");

                    return;
                }

                isUpdating = true;
                close();
                if (!Filesystem::deleteFile(MAP_PATH)) {
                    Log::instance()->warning("Error deleting previous map\n");
                    request->send(500, "text/plain", "error deleting previous map");

                    return;
                }

                Log::instance()->debug("Previous map deleted or not present\n");
            }

            Log::instance()->trace("Received map chunk: %d-%d (%d bytes)\n", index, index + len, len);
            if (!Filesystem::writeBytes(MAP_PATH, data, len)) {
                Log::instance()->error("Error writing map chunk\n");
                request->send(500, "text/plain", "error writing map");

                return;
            }
        }
    );
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
        const char* key = kvp.key().c_str();
        std::string value = kvp.value().as<std::string>();
        if (strcasecmp(key, "Authorization") == 0) {
            size_t i = value.find(' ');
            if (i == std::string::npos) {
                Log::instance()->error("Missing header authorization type or value\n");

                return false;
            }

            client.setAuthorizationType(value.substr(0, i).c_str());
            client.setAuthorization(value.substr(i + 1).c_str());
            continue;
        }
        if (strcasecmp(key, "User-Agent") == 0) {
            client.setUserAgent(value.c_str());
            continue;
        }
        if (strcasecmp(key, "Connection") == 0) {
            client.setReuse(strcmp(value.c_str(), "close") != 0);
            continue;
        }
        if (strcasecmp(key, "Host") == 0) {
            continue;
        }

        client.addHeader(key, value.c_str());
    }

    return true;
};

std::array<double, 2> Map::matchRegex(const char* res, const char *r) {
    std::array<double, 2> pos = { 0, 0 };
    std::regex regex(r);
    std::smatch matches;
    std::string str = std::string(res);
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
    WiFiClientSecure client;
    client.setInsecure(); // TODO: load a CA certificates bundle
    HTTPClient http;
    http.setReuse(false);
    http.setConnectTimeout(10000);
    http.setTimeout(60000);
    String u = config["url"].as<String>();
    String m = config["method"].as<String>();
    String r = config["regex"].as<String>();
    String b = config["body"].as<String>();

    if (u.isEmpty()) {
        Log::instance()->error("Position API URL is not set\n");

        return pos;
    }
    if (r.isEmpty()) {
        Log::instance()->error("Position API regexp is not set\n");

        return pos;
    }
    if (m.isEmpty()) {
        m = "GET";
    }

    // Apply query params for GET requests
    if (m.equalsIgnoreCase("GET") && !b.isEmpty()) {
        // Need a "/" after hostname, otherwise the query parameters end up in the DNS query
        if (u.endsWith("/")) {
            u += b;
        } else {
            u += "/" + b;
        }
    }

    http.begin(client, u.c_str());

    // Apply headers
    String hs = config["headers"].as<String>();
    if (!hs.isEmpty() && !setHeaders(http, hs.c_str())) {
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
    String resp = http.getString();
    pos = matchRegex(resp.c_str(), r.c_str());

    return pos;
};

std::array<double, 2> Map::getPosition() {
    std::array<double, 2> pos = { 0, 0 };
    if (
        config["url"].as<String>() != ""
        && config["method"].as<String>() != ""
        && config["regex"].as<String>() != ""
    ) {
        pos = getPositionFromAPI();
        String url = config["url"].as<String>();
        Log::instance()->info("Position from API (%s): %f, %f\n", url.c_str(), pos[0], pos[1]);
    }
    // default to static position, if available
    if (
        pos[0] == 0 && pos[1] == 0
        && config["latitude"].as<String>() != ""
        && config["longitude"].as<String>() != ""
    ) {
        String lat = config["latitude"].as<String>();
        String lon = config["longitude"].as<String>();
        pos[0] = strtod(lat.c_str(), NULL);
        pos[1] = strtod(lon.c_str(), NULL);
        Log::instance()->info("Position from configuration: %f, %f\n", pos[0], pos[1]);
    }

    return pos;
};

void Map::asyncUpdateCrop(void *p) {
    Map *m = (Map*) p;
    Log::instance()->debug("Updating crop asynchronously\n");
    m->updateCrop();
    m->lastCropUpdate = millis();
    m->forceCropUpdate = false;
    // Close to re-render in next render() call
    m->close();
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
    if (isUpdating) {
        return;
    }

    // TODO: test if this is still true
    // Call to open is here because trying to open in webserver endpoint (right after finished writing) causes a panic and reboot
    if (!open()) {
        return;
    }

    // Update crop area (and position) once every 15 minutes
    if (forceCropUpdate || millis() - lastCropUpdate > UPDATE_CROP_AREA) {
        isUpdating = true;
        xTaskCreate(asyncUpdateCrop, "Update crop area", 8192, this, 0, NULL);

        return;
    }

    // Render once
    if (lastRender > 0) {
        return;
    }

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
};

void* Map::openFile(const char *path, int32_t *size) {
    File *f = new File();
    *f = LittleFS.open(path);
    if (*f) {
        *size = f->size();

        return static_cast<void*>(f);
    }

    delete f;

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

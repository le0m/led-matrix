#include "media.h"

Media::Media(uint8_t w, uint8_t h) {
    width = w;
    height = h;
};

Media::~Media() {
    close();
};

bool Media::open() {
    if (current != nullptr && current->isOpen()) {
        return true;
    }

    if (current != nullptr) {
        delete current;
    }

    if (DecoderJPEG::fileExists()) {
        current = new DecoderJPEG();
        if (!current->open()) {
            Log::instance()->error("Error opening JPEG\n");
            mediaType = MEDIA_TYPE_NONE;

            return false;
        }

        Log::instance()->info("Loaded JPEG\n");
        mediaType = MEDIA_TYPE_IMAGE;

        return true;
    }

    if (DecoderGIF::fileExists()) {
        current = new DecoderGIF();
        if (!current->open()) {
            Log::instance()->error("Error opening GIF\n");
            mediaType = MEDIA_TYPE_NONE;

            return false;
        }

        Log::instance()->info("Loaded GIF\n");
        mediaType = MEDIA_TYPE_GIF;

        return true;
    }

    Log::instance()->trace("No media stored\n");
    mediaType = MEDIA_TYPE_NONE;

    return false;
};

bool Media::close() {
    if (current == nullptr) {
        return true;
    }


    delete current;
    current = nullptr;

    return true;
};

void Media::print() {
    // TODO
};

bool Media::checkSize(AsyncWebServerRequest *request, size_t size) {
    if (size > MAX_FILE_SIZE) {
        Log::instance()->error("File size %d exceeds maximum size %d\n", size, MAX_FILE_SIZE);
        request->send(400, "text/plain", "file is too big");

        return false;
    }

    if (size > LittleFS.totalBytes() - LittleFS.usedBytes()) {
        Log::instance()->error("File size %d is within limit, but disk has not enough free space (%d)\n", size, LittleFS.totalBytes() - LittleFS.usedBytes());
        request->send(500, "text/plain", "not enough free space");

        return false;
    }

    return true;
};

void Media::initServer(AsyncWebServer *server) {
    server->on("/drawer", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (current != nullptr && current->sendFile(request)) {
            return;
        }

        Log::instance()->info("No media currently stored\n");
        request->send(204);
    });
    server->on("/drawer", HTTP_DELETE, [&](AsyncWebServerRequest *request) {
        if (current == nullptr) {
            Log::instance()->info("No media to delete\n");
            request->send(204);

            return;
        }

        mediaType = MEDIA_TYPE_NONE;
        current->close();
        Log::instance()->info("Deleting current media\n");

        if (!current->deleteFile()) {
            Log::instance()->error("Error deleting file\n");
            request->send(500, "text/plain", "error deleting file");

            return;
        }

        delete current;
        current = nullptr;
        request->send(204);
    });
    server->on("/drawer", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            Log::instance()->info("Media received\n");
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType == nullptr) {
                Log::instance()->error("File upload has no content-type header");
                request->send(400, "text/plain", "no content-type header");

                return;
            }
            if (contentType->value() == "image/jpeg") {
                Log::instance()->info("Image stored\n");
                mediaType = MEDIA_TYPE_IMAGE;
            } else if (contentType->value() == "image/gif") {
                Log::instance()->info("GIF stored\n");
                mediaType = MEDIA_TYPE_GIF;
            }

            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                Log::instance()->debug("Receiving file: %d bytes\n", total);
                const AsyncWebHeader *contentType = request->getHeader("Content-Type");

                if (contentType->value() != "image/jpeg" && contentType->value() != "image/gif") {
                    Log::instance()->error("Uploaded file has unhandled content-type: %s\n", contentType->value());
                    request->send(400, "text/plain", "unhandled content type");

                    return;
                }

                if (!checkSize(request, total)) {
                    return;
                }

                mediaType = MEDIA_TYPE_NONE;

                if (current != nullptr) {
                    current->close();

                    if (!current->deleteFile()) {
                        Log::instance()->warning("Error deleting previous file\n");
                        request->send(500, "text/plain", "error deleting previous file");

                        return;
                    }

                    Log::instance()->debug("Previous media deleted or not present\n");
                    delete current;
                    current = nullptr;
                }

                if (contentType->value() == "image/jpeg") {
                    current = new DecoderJPEG();
                } else if (contentType->value() == "image/gif") {
                    current = new DecoderGIF();
                } // else... checked before
            }

            Log::instance()->trace("Received file chunk: %d-%d (%d bytes)\n", index, index + len, len);

            if (current == nullptr || !current->writeFile(data, len)) {
                Log::instance()->error("Error writing file chunk\n");
                request->send(500, "text/plain", "error writing file");
            }
        }
    );
};

void Media::render(MatrixPanel_I2S_DMA *display) {
    if (mediaType == MEDIA_TYPE_NONE || current == nullptr) {
        display->clearScreen();

        return;
    }

    // TODO: test if this is still true
    // Call to open is here because trying to open in webserver endpoint (right after finished writing) causes a panic and reboot
    if (!open()) {
        return;
    }

    current->renderFrame(display);
};

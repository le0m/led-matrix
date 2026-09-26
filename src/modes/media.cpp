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

bool Media::deleteFiles() {
    return Filesystem::deleteFile(JPEG_FILE_PATH) && Filesystem::deleteFile(GIF_FILE_PATH);
};

void Media::initServer(AsyncWebServer *server) {
    server->on("/drawer", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (DecoderJPEG::fileExists()) {
            Log::instance()->info("Sending current JPEG\n");
            request->send(LittleFS, JPEG_FILE_PATH, JPEG_FILE_TYPE);

            return;
        }

        if (DecoderGIF::fileExists()) {
            Log::instance()->info("Sending current GIF\n");
            request->send(LittleFS, GIF_FILE_PATH, GIF_FILE_TYPE);

            return;
        }

        Log::instance()->info("No media currently stored\n");
        request->send(204);
    });
    server->on("/drawer", HTTP_DELETE, [&](AsyncWebServerRequest *request) {
        mediaType = MEDIA_TYPE_NONE;
        close();
        Log::instance()->info("Deleting current media\n");

        if (!deleteFiles()) {
            Log::instance()->error("Error deleting file\n");
            request->send(500, "text/plain", "error deleting file");

            return;
        }

        request->send(204);
    });
    server->on("/drawer", HTTP_POST,
        [&](AsyncWebServerRequest *request) {
            // a response was already set by the body handler
            if (request->getResponse() != nullptr) {
                return;
            }

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
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (index == 0) {
                Log::instance()->debug("Receiving file: %d bytes\n", total);
                if (contentType == nullptr) {
                    Log::instance()->error("Uploaded file has no content-type header");
                    request->send(400, "text/plain", "no content-type header");

                    return;
                }
                if (contentType->value() != "image/jpeg" && contentType->value() != "image/gif") {
                    Log::instance()->error("Uploaded file has unhandled content-type: %s\n", contentType->value());
                    request->send(400, "text/plain", "unhandled content type");

                    return;
                }

                if (!checkSize(request, total)) {
                    return;
                }

                mediaType = MEDIA_TYPE_NONE;
                close();

                if (!deleteFiles()) {
                    Log::instance()->warning("Error deleting previous file\n");
                    request->send(500, "text/plain", "error deleting previous file");

                    return;
                }

                Log::instance()->debug("Previous media deleted or not present\n");
            }

            // a response was already set from a previous chunk's error
            if (request->getResponse() != nullptr) {
                return;
            }

            Log::instance()->trace("Received file chunk: %d-%d (%d bytes)\n", index, index + len, len);

            const char *path = contentType->value() == "image/jpeg" ? JPEG_FILE_PATH : GIF_FILE_PATH;
            if (!Filesystem::writeBytes(path, data, len)) {
                Log::instance()->error("Error writing file chunk\n");
                request->send(500, "text/plain", "error writing file");
            }
        }
    );
};

void Media::render(MatrixPanel_I2S_DMA *display) {
    if (mediaType == MEDIA_TYPE_NONE) {
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

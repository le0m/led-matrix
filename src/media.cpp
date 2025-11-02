#include "media.h"

Media::Media(uint8_t w, uint8_t h) {
    width = w;
    height = h;
};

Media::~Media() {
    close();
};

bool Media::open() {
    return loadMedia();
};

bool Media::close() {
    gif.close();
    image.close();

    return true;
};

void Media::print() {
    // TODO
};

bool Media::processImageChunk(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        mediaType = MEDIA_TYPE_NONE;
        Log::instance()->debug("Receiving image: %d bytes\n", total);
        if (total > MAX_IMAGE_SIZE) {
            Log::instance()->error("Image size %d exceeds maximum size %d\n", total, MAX_IMAGE_SIZE);
            request->send(400, "text/plain", "image is too big");

            return false;
        }
        if (total > LittleFS.totalBytes() - LittleFS.usedBytes()) {
            Log::instance()->error("Image size %d is within limit, but disk has not enough free space (%d)\n", total, LittleFS.totalBytes() - LittleFS.usedBytes());
            request->send(500, "text/plain", "not enough free space");

            return false;
        }

        isUpdating = true;
        gif.close();
        image.close();
        if (!deleteFile(GIF_PATH)) {
            Log::instance()->warning("Error deleting previous GIF\n");
            request->send(500, "text/plain", "error deleting previous GIF");

            return false;
        }
        if (!deleteFile(IMAGE_PATH)) {
            Log::instance()->warning("Error deleting previous image\n");
            request->send(500, "text/plain", "error deleting previous image");

            return false;
        }

        Log::instance()->debug("Previous image and GIF deleted or not present\n");
    }

    Log::instance()->trace("Received image chunk: %d-%d (%d bytes)\n", index, index + len, len);
    if (!writeBytes(IMAGE_PATH, data, len)) {
        Log::instance()->error("Error writing image chunk\n");
        request->send(500, "text/plain", "error writing image");

        return false;
    }

    return true;
};

bool Media::processGIFChunk(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        mediaType = MEDIA_TYPE_NONE;
        Log::instance()->debug("Receiving GIF: %d bytes\n", total);
        if (total > MAX_GIF_SIZE) {
            Log::instance()->debug("GIF size %d exceeds maximum size %d\n", total, MAX_GIF_SIZE);
            request->send(400, "text/plain", "GIF too big");

            return false;
        }
        if (total > LittleFS.totalBytes() - LittleFS.usedBytes()) {
            Log::instance()->error("GIF size %d is within limit, but disk has not enough free space (%d)\n", total, LittleFS.totalBytes() - LittleFS.usedBytes());
            request->send(500, "text/plain", "not enough free space");

            return false;
        }

        isUpdating = true;
        gif.close();
        image.close();
        if (!deleteFile(GIF_PATH)) {
            Log::instance()->error("Error deleting previous GIF\n");
            request->send(500, "text/plain", "error deleting previous GIF");

            return false;
        }
        if (!deleteFile(IMAGE_PATH)) {
            Log::instance()->error("Error deleting previous image\n");
            request->send(500, "text/plain", "error deleting previous image");

            return false;
        }

        Log::instance()->debug("Previous image and GIF deleted or not present\n");
    }

    Log::instance()->trace("Received GIF chunk: %d-%d (%d bytes)\n", index, index + len, len);
    if (!writeBytes(GIF_PATH, data, len)) {
        Log::instance()->error("Error writing GIF chunk\n");
        request->send(500, "text/plain", "error writing GIF");

        return false;
    }

    return true;
};

void Media::initServer(AsyncWebServer *server) {
    server->on("/drawer", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (pathExists(IMAGE_PATH)) {
            Log::instance()->info("Sending current image\n");
            request->send(LittleFS, IMAGE_PATH, "image/jpeg");

            return;
        }

        if (pathExists(GIF_PATH)) {
            Log::instance()->info("Sending current GIF\n");
            request->send(LittleFS, GIF_PATH, "image/gif");

            return;
        }

        Log::instance()->info("No media currently stored\n");
        request->send(204);
    });
    server->on("/drawer", HTTP_DELETE, [&](AsyncWebServerRequest *request) {
        mediaType = MEDIA_TYPE_NONE;
        gif.close();
        image.close();
        Log::instance()->info("Deleting media\n");
        if (!deleteFile(GIF_PATH)) {
            Log::instance()->error("Error deleting GIF\n");
            request->send(500, "text/plain", "error deleting GIF");

            return;
        }
        if (!deleteFile(IMAGE_PATH)) {
            Log::instance()->error("Error deleting image\n");
            request->send(500, "text/plain", "error deleting image");

            return;
        }

        Log::instance()->info("Media deleted\n");
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
                drawNewImage = true;
                Log::instance()->info("Image stored\n");
                mediaType = MEDIA_TYPE_IMAGE;
            }
            if (contentType->value() == "image/gif") {
                Log::instance()->info("GIF stored\n");
                mediaType = MEDIA_TYPE_GIF;
            }

            isUpdating = false;
            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType->value() == "image/jpeg") {
                processImageChunk(request, data, len, index, total);

                return;
            }
            if (contentType->value() == "image/gif") {
                processGIFChunk(request, data, len, index, total);

                return;
            }

            Log::instance()->error("Uploaded file has unhandled content-type: %s\n", contentType->value());
            request->send(400, "text/plain", "unhandled content type");
        }
    );
};

bool Media::loadMedia() {
    if (image.isOpen || gif.isOpen) {
        return true;
    }

    if (pathExists(IMAGE_PATH)) {
        if (image.open(IMAGE_PATH)) {
            Log::instance()->info("Loaded image from FLASH\n");
            mediaType = MEDIA_TYPE_IMAGE;

            return true;
        }

        Log::instance()->error("Error opening image from FLASH\n");
        mediaType = MEDIA_TYPE_NONE;

        return false;
    }

    if (pathExists(GIF_PATH)) {
        if (gif.open(GIF_PATH)) {
            Log::instance()->info("Loaded GIF from FLASH\n");
            mediaType = MEDIA_TYPE_GIF;

            return true;
        }

        Log::instance()->error("Error opening GIF from FLASH\n");
        mediaType = MEDIA_TYPE_NONE;

        return false;
    }

    Log::instance()->trace("No media stored in FLASH\n");
    mediaType = MEDIA_TYPE_NONE;

    return false;
};

void Media::render(MatrixPanel_I2S_DMA *display) {
    if (isUpdating) {
        return;
    }

    if (mediaType == MEDIA_TYPE_NONE) {
        display->clearScreen();

        return;
    }

    // Call to open is here because trying to open in webserver endpoint (right after finished writing) causes a panic and reboot
    if (!loadMedia()) {
        return;
    }

    if (mediaType == MEDIA_TYPE_IMAGE) {
        // Clear screen only if a new image was uploaded.
        if (drawNewImage) {
            display->clearScreen();
            drawNewImage = false;
        }

        image.renderFrame(display);

        return;
    }

    if (mediaType == MEDIA_TYPE_GIF) {
        gif.renderFrame(display);

        return;
    }
};

#include "image.h"

Image::Image(uint8_t w, uint8_t h) {
    width = w;
    height = h;
    size = width * height * sizeof(uint8_t) * 3;
    image = (uint8_t*)malloc(size);
    gif = new GIF();
};

Image::~Image() {
    free(image);
    delete image;
    gif->close();
};

void Image::print() {
    Log::instance()->trace("Image bytes:\n");
    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t x = 0; x < width; x++) {
            Log::instance()->trace("%02X%02X%02X ", image[x + y * width], image[1 + (x + y * width)], image[2 + (x + y * width)]);
        }

        Log::instance()->trace("\n");
    }
};

bool Image::processImageChunk(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        hasMedia = MEDIA_TYPE_NONE;
        Log::instance()->debug("Receiving image: %d bytes\n", total);
        if (total > size) {
            Log::instance()->error("Image is bigger than max size: %d bytes\n", size);
            request->send(400, "text/plain", "image is too big");

            return false;
        }
        if (total > LittleFS.totalBytes() - LittleFS.usedBytes()) {
            Log::instance()->error("Image size %d is within limit, but disk has not enough free space (%d)\n", total, LittleFS.totalBytes() - LittleFS.usedBytes());
            request->send(500, "text/plain", "not enough free space");

            return false;
        }

        gif->close();
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
    if (index + len > size) {
        Log::instance()->error("Data chunk exceeds image size boundaries: chunk start %d, end %d\n", index, index + len);
        request->send(400, "text/plain", "chunk exceeds image size boundaries");

        return false;
    }

    Log::instance()->trace("Received image chunk: %d-%d (%d bytes)\n", index, index + len, len);
    memcpy(&image[index], data, len);
    if (!writeBytes(IMAGE_PATH, data, len)) {
        Log::instance()->error("Error writing image chunk\n");
        request->send(500, "text/plain", "error writing image");

        return false;
    }

    return true;
};

bool Image::processGIFChunk(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        hasMedia = MEDIA_TYPE_NONE;
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

        gif->close();
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

void Image::initServer(AsyncWebServer* server) {
    server->on("/drawer", HTTP_GET, [&](AsyncWebServerRequest *request) {
        if (hasMedia == MEDIA_TYPE_NONE) {
            Log::instance()->info("No image currently stored\n");
            request->send(204);

            return;
        }
        if (hasMedia == MEDIA_TYPE_IMAGE) {
            Log::instance()->info("Sending current image\n");
            AsyncResponseStream *response = request->beginResponseStream("application/octet-stream");
            response->addHeader("Content-Length", size);
            response->write(image, size);
            request->send(response);

            return;
        }
        if (hasMedia == MEDIA_TYPE_GIF) {
            request->send(204);

            return;
        }
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
            if (contentType->value() == "application/octet-stream") {
                drawNewImage = true;
                hasMedia = MEDIA_TYPE_IMAGE;
                Log::instance()->info("Image stored\n");
            }
            if (contentType->value() == "image/gif") {
                Log::instance()->info("GIF stored\n");
                hasMedia = MEDIA_TYPE_GIF;
            }

            request->send(204);
        },
        nullptr,
        [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            const AsyncWebHeader *contentType = request->getHeader("Content-Type");
            if (contentType == nullptr) {
                Log::instance()->error("File upload has no content-type header");
                request->send(400, "text/plain", "no content-type header");

                return;
            }
            if (contentType->value() == "application/octet-stream") {
                processImageChunk(request, data, len, index, total);

                return;
            }
            if (contentType->value() == "image/gif") {
                processGIFChunk(request, data, len, index, total);

                return;
            }

            request->send(400, "text/plain", "unknown content type");
        }
    );
};

void Image::loadMedia() {
    if (readBytes(IMAGE_PATH, image, width * height * 3)) {
        Log::instance()->info("Loaded image from FLASH\n");
        hasMedia = MEDIA_TYPE_IMAGE;
        drawNewImage = true;

        return;
    }
    if (pathExists(GIF_PATH)) {
        Log::instance()->info("Using GIF from FLASH\n");
        hasMedia = MEDIA_TYPE_GIF;
        if (!gif->open(GIF_PATH)) {
            Log::instance()->error("Error opening GIF from FLASH\n");
            hasMedia = MEDIA_TYPE_NONE;
        }

        return;
    }

    Log::instance()->info("No media stored in FLASH\n");
};

void Image::render(MatrixPanel_I2S_DMA *display) {
    if (hasMedia == MEDIA_TYPE_NONE) {
        return;
    }
    // GIF handles timing internally
    if (hasMedia == MEDIA_TYPE_GIF) {
        // Call to open is here because trying to open in webserver endpoint (right after finished writing) causes a panic and reboot
        if (!gif->open(GIF_PATH)) {
            Log::instance()->error("Error opening GIF from FLASH\n");
            hasMedia = MEDIA_TYPE_NONE;

            return;
        }

        gif->renderFrame(display);

        return;
    }
    // Render image once a second
    if (millis() - lastRender < 1000) {
        return;
    }
    // Clear screen only if a new image was uploaded.
    if (drawNewImage) {
        display->clearScreen();
        drawNewImage = false;
    }

    // Keep redrawing whether the images has changed or not so that it gets redrawn when the picture frame is rotated.
    lastRender = millis();
    uint16_t index;
    for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
            index = 3 * (x + y * width);
            display->drawPixelRGB888(
                x,
                y,
                image[index],
                image[1 + index],
                image[2 + index]
            );
        }
    }
};

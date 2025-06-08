#ifndef _IMAGE_H
#define _IMAGE_H

#include "log.h"
#include "renderer.h"
#include <stdint.h>
#include "filesystem.h"
#include "gif.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define IMAGE_PATH "/image.bin"
#define GIF_PATH "/media.gif"

class Image : public Renderer {
    private:
        typedef enum {
            MEDIA_TYPE_NONE  = 0,
            MEDIA_TYPE_IMAGE = 1,
            MEDIA_TYPE_GIF   = 2,
        } media_type;
        uint8_t *image;
        bool drawNewImage = false;
        ulong lastRender = 0;
        GIF *gif;
        media_type hasMedia = MEDIA_TYPE_NONE;
        bool processImageChunk(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t);
        bool processGIFChunk(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t);

    public:
        Image(uint8_t, uint8_t);
        virtual ~Image();
        virtual void initServer(AsyncWebServer*);
        void loadMedia();
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
};

#endif

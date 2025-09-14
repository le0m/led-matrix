#ifndef _MEDIA_H
#define _MEDIA_H

#include "log.h"
#include "renderer.h"
#include <stdint.h>
#include "filesystem.h"
#include "image.h"
#include "gif.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define IMAGE_PATH "/image.jpeg"
#define GIF_PATH "/media.gif"

class Media : public Renderer {
    private:
        typedef enum {
            MEDIA_TYPE_NONE  = 0,
            MEDIA_TYPE_IMAGE = 1,
            MEDIA_TYPE_GIF   = 2,
        } media_type;
        bool drawNewImage = false;
        GIF gif;
        Image image;
        media_type mediaType = MEDIA_TYPE_NONE;
        bool processImageChunk(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t);
        bool processGIFChunk(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t);

    public:
        Media(uint8_t, uint8_t);
        virtual ~Media();
        virtual void initServer(AsyncWebServer*);
        void loadMedia();
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
};

#endif

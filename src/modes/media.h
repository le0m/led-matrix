#ifndef _MEDIA_H
#define _MEDIA_H

#include "../modules/log.h"
#include "renderer.h"
#include <stdint.h>
#include "filesystem.h"
#include "../decoders/jpeg.h"
#include "../decoders/gif.h"
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
        Decoder *current;
        media_type mediaType = MEDIA_TYPE_NONE;
        bool checkSize(AsyncWebServerRequest*, size_t);

    public:
        Media(uint8_t, uint8_t);
        virtual ~Media();
        virtual void initServer(AsyncWebServer*);
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
        virtual bool open();
        virtual bool close();
};

#endif

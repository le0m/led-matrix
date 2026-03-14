#ifndef _DECODER_H
#define _DECODER_H

#include "../filesystem.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESPAsyncWebServer.h>

class Decoder {
    protected:
        bool isOpen_ = false;

    public:
        Decoder() {};
        virtual ~Decoder() {};
        virtual bool isOpen();
        virtual bool deleteFile() = 0;
        virtual bool writeFile(uint8_t*, size_t) = 0;
        virtual bool sendFile(AsyncWebServerRequest*) = 0;
        virtual bool open() = 0;
        virtual void close() = 0;
        virtual void renderFrame(MatrixPanel_I2S_DMA*) = 0;
};

#endif

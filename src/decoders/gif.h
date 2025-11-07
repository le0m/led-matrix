#ifndef _GIF_H
#define _GIF_H

#include "../modules/log.h"
#include "../filesystem.h"
#include "decoder.h"
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define GIF_FILE_PATH "/image.gif"
#define GIF_FILE_TYPE "image/gif"

class DecoderGIF : public Decoder {
    private:
        AnimatedGIF gif;
        int frameDelay = 0;
        ulong lastGifFrameRender = 0;
        static void drawLine(GIFDRAW*);
        static void* openFile(const char*, int32_t*);
        static void closeFile(void*);
        static int32_t readFile(GIFFILE*, uint8_t*, int32_t);
        static int32_t seekFile(GIFFILE*, int32_t);

    public:
        DecoderGIF();
        virtual ~DecoderGIF();
        static bool fileExists();
        virtual bool deleteFile();
        virtual bool writeFile(uint8_t*, size_t);
        virtual bool sendFile(AsyncWebServerRequest*);
        virtual bool open();
        virtual void close();
        virtual void renderFrame(MatrixPanel_I2S_DMA*);
};

#endif

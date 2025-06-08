#ifndef _GIF_H
#define _GIF_H

#include "log.h"
#include "filesystem.h"
#include <AnimatedGIF.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class GIF {
    private:
        AnimatedGIF gif;
        int frameDelay = 0;
        ulong lastGifFrameRender = 0;
        bool playing = false;
        static void drawLine(GIFDRAW*);
        static void* openFile(const char*, int32_t*);
        static void closeFile(void*);
        static int32_t readFile(GIFFILE*, uint8_t*, int32_t);
        static int32_t seekFile(GIFFILE*, int32_t);

    public:
        GIF();
        ~GIF();
        bool open(const char*);
        void renderFrame(MatrixPanel_I2S_DMA*);
        void close();
};

#endif

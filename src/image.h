#ifndef _IMAGE_H
#define _IMAGE_H

#include "log.h"
#include "filesystem.h"
#include <JPEGDEC.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class Image {
    private:
        JPEGDEC jpeg;
        ulong lastRender = 0;
        const char* filePath;
        static int draw(JPEGDRAW*);
        static void* openFile(const char*, int32_t*);
        static void closeFile(void*);
        static int32_t readFile(JPEGFILE*, uint8_t*, int32_t);
        static int32_t seekFile(JPEGFILE*, int32_t);

    public:
        bool isOpen = false;
        Image();
        ~Image();
        bool open(const char*);
        void renderFrame(MatrixPanel_I2S_DMA*);
        void close();
};

#endif

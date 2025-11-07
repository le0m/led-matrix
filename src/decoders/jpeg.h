#ifndef _JPEG_H
#define _JPEG_H

#include "../modules/log.h"
#include "../filesystem.h"
#include "decoder.h"
#include <JPEGDEC.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define JPEG_FILE_PATH "/image.jpeg"
#define JPEG_FILE_TYPE "image/jpeg"

class DecoderJPEG : public Decoder {
    private:
        JPEGDEC jpeg;
        ulong lastRender = 0;
        static int draw(JPEGDRAW*);
        static void* openFile(const char*, int32_t*);
        static void closeFile(void*);
        static int32_t readFile(JPEGFILE*, uint8_t*, int32_t);
        static int32_t seekFile(JPEGFILE*, int32_t);

    public:
        DecoderJPEG();
        virtual ~DecoderJPEG();
        static bool fileExists();
        virtual bool deleteFile();
        virtual bool writeFile(uint8_t*, size_t);
        virtual bool sendFile(AsyncWebServerRequest*);
        virtual bool open();
        virtual void close();
        virtual void renderFrame(MatrixPanel_I2S_DMA*);
};

#endif

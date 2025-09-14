#ifndef _MAP_H
#define _MAP_H

#include <string>
#include "log.h"
#include "renderer.h"
#include "filesystem.h"
#include <ArduinoJson.h>
#include <JPEGDEC.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define MAP_PATH "/map.jpeg"

class Map : public Renderer {
    private:
        typedef struct drawUserData {
            MatrixPanel_I2S_DMA *display;
            uint16_t *position;
            uint8_t size;
            uint16_t color;
            uint16_t track;
        } drawUserData;
        JPEGDEC jpeg;
        ulong lastRender = 0;
        ulong lastCropUpdate = 0;
        uint16_t cropArea[2];
        uint16_t cropPosition[2];
        bool isUpdating = false;
        bool isOpen = false;
        JsonDocument config;
        static void asyncUpdateCrop(void*);
        bool openImage();
        void closeImage();
        static int draw(JPEGDRAW*);
        static void* openFile(const char*, int32_t*);
        static void closeFile(void*);
        static int32_t readFile(JPEGFILE*, uint8_t*, int32_t);
        static int32_t seekFile(JPEGFILE*, int32_t);
        std::array<double, 2> getPositionFromAPI();
        std::array<double, 2> getPosition();
        void updateCrop();

    public:
        Map(uint8_t, uint8_t);
        virtual ~Map();
        virtual void initServer(AsyncWebServer*);
        void setConfig(JsonVariantConst);
        void loadMedia();
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
};

#endif

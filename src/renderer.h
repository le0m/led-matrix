#ifndef _RENDERER_H
#define _RENDERER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESPAsyncWebServer.h>

class Renderer {
    protected:
        uint8_t width;
        uint8_t height;
        uint16_t size;

    public:
        Renderer() : Renderer(0, 0) {};
        Renderer(uint8_t, uint8_t) {};
        virtual ~Renderer() {};
        virtual void initServer(AsyncWebServer*) = 0;
        virtual void render(MatrixPanel_I2S_DMA*) = 0;
        virtual void print() = 0;
};

#endif

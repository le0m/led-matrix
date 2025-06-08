#ifndef _LIFE_H
#define _LIFE_H

#include "log.h"
#include "renderer.h"
#include <stdint.h>
#include <FastLED.h>

struct Cell {
    uint8_t alive : 1;
    uint8_t prev : 1;
    uint8_t hue;
};

class Life : public Renderer {
    private:
        Cell *world;
        uint16_t generation = 0;
        uint16_t maxGeneration = 512;
        uint8_t density = 50;
        uint8_t fps = 15;
        ulong lastRender = 0;
        CHSV hsv = CHSV(0, 255, 255);
        CRGB rgb = CRGB(0, 0, 0);

        virtual void populate();
        virtual uint8_t neighbors(uint8_t, uint8_t);

    public:
        Life(uint8_t, uint8_t);
        virtual ~Life();
        void setFPS(uint8_t);
        virtual void initServer(AsyncWebServer*);
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
};

#endif

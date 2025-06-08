#include "life.h"

Life::Life(uint8_t w, uint8_t h) {
    width = w;
    height = h;
    size = width * height * sizeof(Cell);
    world = (Cell*)malloc(size);
};

Life::~Life() {
    free(world);
};

void Life::setFPS(uint8_t f) {
    fps = f;
};

void Life::populate() {
    for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
            world[x + y * width].alive = random(100) < density;
            world[x + y * width].prev = world[x + y * width].alive;
            world[x + y * width].hue = 0;
        }
    }
};

uint8_t Life::neighbors(uint8_t x, uint8_t y) {
    return (world[((x + 1) % width) + y * width].prev) +
            (world[x + ((y + 1) % height) * width].prev) +
            (world[((x + width - 1) % width) + y * width].prev) +
            (world[x + ((y + height - 1) % height) * width].prev) +
            (world[((x + 1) % width) + ((y + 1) % height) * width].prev) +
            (world[((x + width - 1) % width) + ((y + 1) % height) * width].prev) +
            (world[((x + width - 1) % width) + ((y + height - 1) % height) * width].prev) +
            (world[((x + 1) % width) + ((y + height - 1) % height) * width].prev);
};

void Life::print() {
    Log::instance()->info("Gen %d\n", generation);
    for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
            Log::instance()->info("%d", world[x + y * width].prev);
        }
        Log::instance()->info("\n");
    }
};

void Life::initServer(AsyncWebServer* server) {};

void Life::render(MatrixPanel_I2S_DMA* display) {
    if (generation == 0) {
        populate();
    }
    if (millis() - lastRender < 1000 / fps) {
        return;
    }

    lastRender = millis();

    // render current generation
    for (uint8_t y = 0; y < height; y++) {
        for (uint8_t x = 0; x < width; x++) {
            if (world[x + y * width].prev == 1) {
                hsv.hue = world[x + y * width].hue;
                hsv2rgb_rainbow(hsv, rgb);
                display->drawPixelRGB888(x, y, rgb.r, rgb.g, rgb.b);
            } else {
                display->drawPixelRGB888(x, y, 0, 0, 0);
            }
        }
    }

    // apply rules of life
    uint8_t n;
    for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
            n = neighbors(x, y);
            if (world[x + y * width].prev == 1 && (n < 2 || n > 3)) {
                world[x + y * width].alive = 0;
            } else if (world[x + y * width].prev == 0 && n == 3) {
                world[x + y * width].alive = 1;
                world[x + y * width].hue = generation / 2; // generation is in range [0, 511], while hue is uint8 [0, 255], so dividing by 2 should do the trick
            }
        }
    }

    // move to next generation
    for (uint8_t x = 0; x < width; x++) {
        for (uint8_t y = 0; y < height; y++) {
            world[x + y * width].prev = world[x + y * width].alive;
        }
    }

    generation++;
    if (generation >= maxGeneration) {
        generation = 0;
    }
};

#include "led.h"

MatrixPanel_I2S_DMA* setupLED(HUB75_I2S_CFG config) {
    Log::instance()->info("Setting up LED matrix\n");
    MatrixPanel_I2S_DMA *display = new MatrixPanel_I2S_DMA(config);
    display->setTextSize(1);
    display->setTextWrap(true);
    display->setCursor(0, 0);
    display->setTextColor(display->color565(255, 0, 0));
    display->begin();
    display->clearScreen();
    display->setBrightness(127);

    return display;
}

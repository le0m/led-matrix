#ifndef _LED_H
#define _LED_H

#include "log.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

MatrixPanel_I2S_DMA* setupLED(HUB75_I2S_CFG);

#endif

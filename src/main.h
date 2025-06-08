#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include "log.h"
#include "config.h"
#include "wifi.h"
#include "led.h"
#include "modeselector.h"
#include "image.h"
#include "life.h"
#include "qrcode.h"
#include "ota.h"
#include <ArduinoJson.h>

// Pins
#define R1_PIN R1_PIN_DEFAULT
#define G1_PIN G1_PIN_DEFAULT
#define B1_PIN B1_PIN_DEFAULT
#define R2_PIN R2_PIN_DEFAULT
#define G2_PIN G2_PIN_DEFAULT
#define B2_PIN B2_PIN_DEFAULT
#define A_PIN A_PIN_DEFAULT
#define B_PIN 22 // changed, was 19
#define C_PIN C_PIN_DEFAULT
#define D_PIN D_PIN_DEFAULT
#define E_PIN 32 // changed, was -1
#define LAT_PIN LAT_PIN_DEFAULT
#define OE_PIN OE_PIN_DEFAULT
#define CLK_PIN CLK_PIN_DEFAULT

// Matrix
#define WIDTH MATRIX_WIDTH
#define HEIGHT 64 // changed, was 32
#define CHAIN CHAIN_LENGTH
#define ROWS_IN_PARALLEL MATRIX_ROWS_IN_PARALLEL
#define PIXEL_COLOR_DEPTH_BITS 8
#define LAT_BLANKING 2
#define I2C_CLK HUB75_I2S_CFG::HZ_8M

// Config
#define FPS 15

void updateConfig(JsonDocument);

#endif

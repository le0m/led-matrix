#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include "modules/log.h"
#include "modules/config.h"
#include "modules/wifi.h"
#include "modules/led.h"
#include "modules/monitor.h"
#include "modules/statusled.h"
#include "modules/modeselector.h"
#include "modes/media.h"
#include "modes/map.h"
#include "modes/life.h"
#include "modes/qrcode.h"
#include "modules/ota.h"
#include <ArduinoJson.h>

#if defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)
// Pins for the Adafruit MatrixPortal S3 board
#define R1_PIN 42
#define G1_PIN 41
#define B1_PIN 40
#define R2_PIN 38
#define G2_PIN 39
#define B2_PIN 37
#define A_PIN 45
#define B_PIN 36
#define C_PIN 48
#define D_PIN 35
#define E_PIN 21
#define LAT_PIN 47
#define OE_PIN 14
#define CLK_PIN 2
#else
// Pins for the esp32 board
#define R1_PIN R1_PIN_DEFAULT
#define G1_PIN G1_PIN_DEFAULT
#define B1_PIN B1_PIN_DEFAULT
#define R2_PIN R2_PIN_DEFAULT
#define G2_PIN G2_PIN_DEFAULT
#define B2_PIN B2_PIN_DEFAULT
#define A_PIN A_PIN_DEFAULT
#define B_PIN 22
#define C_PIN C_PIN_DEFAULT
#define D_PIN D_PIN_DEFAULT
#define E_PIN 32
#define LAT_PIN LAT_PIN_DEFAULT
#define OE_PIN OE_PIN_DEFAULT
#define CLK_PIN CLK_PIN_DEFAULT
#endif

// Matrix
#define WIDTH MATRIX_WIDTH
#define HEIGHT 64
#define CHAIN CHAIN_LENGTH
#define ROWS_IN_PARALLEL MATRIX_ROWS_IN_PARALLEL
#define PIXEL_COLOR_DEPTH_BITS 8
#define LAT_BLANKING 2
#define I2C_CLK HUB75_I2S_CFG::HZ_20M

#if defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)
#define CLK_PHASE false
#else
#define CLK_PHASE true
#endif

// Config
#define FPS 15

void updateConfig(JsonDocument& newConfig);

#endif

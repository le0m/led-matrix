#if defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)

#include "statusled.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define NEOPIXEL_BRIGHTNESS 30

static const uint16_t BREATHE_PERIOD_MS = 3000;
static const uint8_t  BREATHE_UPDATE_MS = 20; // ~50 fps

static Adafruit_NeoPixel strip(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Priority: FATAL > BLINK > OTA > WIFI

static bool enabled = true;

static bool fatalActive = false;

static bool     blinkActive    = false;
static uint32_t blinkColor     = 0;
static uint16_t blinkOnMs      = 0;
static uint16_t blinkOffMs     = 0;
static uint8_t  blinkRemaining = 0;
static uint32_t blinkToggleAt  = 0;
static bool     blinkOn        = false;

static bool     otaActive = false;
static uint32_t otaColor  = 0;

static bool     wifiActive = false;
static uint32_t wifiColor  = 0;

static uint32_t lastBreathUpdate = 0;

static uint32_t applyBreathing(uint32_t color) {
    float t     = (float)(millis() % BREATHE_PERIOD_MS) / (float)BREATHE_PERIOD_MS;
    float level = 0.5f * (1.0f - cosf(t * 2.0f * 3.14159265f));
    uint8_t r   = (uint8_t)(((color >> 16) & 0xFF) * level);
    uint8_t g   = (uint8_t)(((color >>  8) & 0xFF) * level);
    uint8_t b   = (uint8_t)(( color        & 0xFF) * level);
    return Adafruit_NeoPixel::Color(r, g, b);
}

static void startBlinkN(uint32_t color, uint16_t onMs, uint16_t offMs, uint8_t count) {
    blinkActive    = true;
    blinkColor     = color;
    blinkOnMs      = onMs;
    blinkOffMs     = offMs;
    blinkRemaining = count;
    blinkOn        = true;
    blinkToggleAt  = millis() + onMs;
    strip.setPixelColor(0, color);
    strip.show();
}

void StatusLed::init() {
    strip.begin();
    strip.setBrightness(NEOPIXEL_BRIGHTNESS);
    strip.clear();
    strip.show();
}

void StatusLed::setEnabled(bool val) {
    enabled = val;
    if (!enabled) {
        fatalActive = false;
        blinkActive = false;
        otaActive   = false;
        wifiActive  = false;
        strip.clear();
        strip.show();
    }
}

void StatusLed::tick() {
    if (!enabled) return;
    uint32_t now = millis();

    if (fatalActive) return;

    if (blinkActive) {
        if (now >= blinkToggleAt) {
            if (blinkOn) {
                strip.clear();
                strip.show();
                blinkOn = false;
                if (--blinkRemaining == 0) {
                    blinkActive = false;
                } else {
                    blinkToggleAt = now + blinkOffMs;
                }
            } else {
                strip.setPixelColor(0, blinkColor);
                strip.show();
                blinkOn       = true;
                blinkToggleAt = now + blinkOnMs;
            }
        }
        return;
    }

    if (otaActive || wifiActive) {
        if (now - lastBreathUpdate >= BREATHE_UPDATE_MS) {
            uint32_t color = otaActive ? otaColor : wifiColor;
            strip.setPixelColor(0, applyBreathing(color));
            strip.show();
            lastBreathUpdate = now;
        }
    }
}

void StatusLed::clear() {
    fatalActive = false;
    blinkActive = false;
    otaActive   = false;
    wifiActive  = false;
    strip.clear();
    strip.show();
}

void StatusLed::onBoot() {
    if (!enabled) return;
    startBlinkN(Adafruit_NeoPixel::Color(255, 200, 100), 300, 100, 1); // warm white
}

void StatusLed::onWifiConnected() {
    if (!enabled) return;
    wifiActive = true;
    wifiColor  = Adafruit_NeoPixel::Color(0, 255, 0); // green
}

void StatusLed::onApStarted() {
    if (!enabled) return;
    wifiActive = true;
    wifiColor  = Adafruit_NeoPixel::Color(0, 127, 255); // light blue
}

void StatusLed::onWifiReconnecting() {
    if (!enabled) return;
    wifiActive = true;
    wifiColor  = Adafruit_NeoPixel::Color(255, 160, 0); // amber
}

void StatusLed::onModeChanged(draw_mode mode) {
    if (!enabled) return;
    uint32_t color = 0;
    switch (mode) {
        case DRAW_MODE_IMAGE: color = Adafruit_NeoPixel::Color(255, 140,   0); break; // gold
        case DRAW_MODE_LIFE:  color = Adafruit_NeoPixel::Color(139,   0, 255); break; // violet
        case DRAW_MODE_MAP:   color = Adafruit_NeoPixel::Color(  0, 255, 255); break; // cyan
        case DRAW_MODE_QR:    color = Adafruit_NeoPixel::Color(255, 255, 255); break; // white
        default: return;
    }
    startBlinkN(color, 150, 100, 2);
}

void StatusLed::onOtaStart() {
    if (!enabled) return;
    otaActive = true;
    otaColor  = Adafruit_NeoPixel::Color(0, 255, 255); // cyan
}

void StatusLed::onOtaEnd() {
    if (!enabled) return;
    otaActive = false;
    if (!wifiActive) {
        strip.clear();
        strip.show();
    }
}

void StatusLed::onFatalError() {
    if (!enabled) return;
    fatalActive = true;
    strip.setPixelColor(0, Adafruit_NeoPixel::Color(255, 0, 0));
    strip.show();
}

#endif

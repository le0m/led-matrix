#if !defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)

#include "statusled.h"
#include <Arduino.h>
#include <math.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t  LEDC_CHANNEL    = 0;
static const uint32_t LEDC_FREQ_HZ    = 5000;
static const uint8_t  LEDC_RES_BITS   = 8; // duty 0–255
static const uint8_t  BREATHE_UPDATE_MS = 20; // ~50 fps

// Priority: FATAL > BLINK > OTA > WIFI

static bool enabled = true;

static bool fatalActive = false;

static bool     blinkActive    = false;
static uint16_t blinkOnMs      = 0;
static uint16_t blinkOffMs     = 0;
static uint8_t  blinkRemaining = 0;
static uint32_t blinkToggleAt  = 0;
static bool     blinkOn        = false;

static bool     otaActive   = false;
static uint16_t otaPeriodMs = 0;

static bool     wifiActive   = false;
static uint16_t wifiPeriodMs = 0;

static uint32_t lastBreathUpdate = 0;

static void applyBreathing(uint16_t periodMs) {
    float t     = (float)(millis() % periodMs) / (float)periodMs;
    float level = 0.5f * (1.0f - cosf(t * 2.0f * 3.14159265f));
    ledcWrite(LEDC_CHANNEL, (uint8_t)(level * 255.0f));
}

static void startBlinkN(uint16_t onMs, uint16_t offMs, uint8_t count) {
    blinkActive    = true;
    blinkOnMs      = onMs;
    blinkOffMs     = offMs;
    blinkRemaining = count;
    blinkOn        = true;
    blinkToggleAt  = millis() + onMs;
    ledcWrite(LEDC_CHANNEL, 255);
}

void StatusLed::init() {
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RES_BITS);
    ledcAttachPin(LED_BUILTIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 0);
}

void StatusLed::setEnabled(bool val) {
    enabled = val;
    if (!enabled) {
        fatalActive = false;
        blinkActive = false;
        otaActive   = false;
        wifiActive  = false;
        ledcWrite(LEDC_CHANNEL, 0);
    }
}

void StatusLed::tick() {
    if (!enabled) return;
    uint32_t now = millis();

    if (fatalActive) return;

    if (blinkActive) {
        if (now >= blinkToggleAt) {
            if (blinkOn) {
                ledcWrite(LEDC_CHANNEL, 0);
                blinkOn = false;
                if (--blinkRemaining == 0) {
                    blinkActive = false;
                } else {
                    blinkToggleAt = now + blinkOffMs;
                }
            } else {
                ledcWrite(LEDC_CHANNEL, 255);
                blinkOn       = true;
                blinkToggleAt = now + blinkOnMs;
            }
        }
        return;
    }

    if (otaActive || wifiActive) {
        if (now - lastBreathUpdate >= BREATHE_UPDATE_MS) {
            applyBreathing(otaActive ? otaPeriodMs : wifiPeriodMs);
            lastBreathUpdate = now;
        }
    }
}

void StatusLed::clear() {
    fatalActive = false;
    blinkActive = false;
    otaActive   = false;
    wifiActive  = false;
    ledcWrite(LEDC_CHANNEL, 0);
}

void StatusLed::onBoot() {
    if (!enabled) return;
    startBlinkN(300, 100, 1);
}

void StatusLed::onWifiConnected() {
    if (!enabled) return;
    wifiActive   = true;
    wifiPeriodMs = 3000; // slow
}

void StatusLed::onApStarted() {
    if (!enabled) return;
    wifiActive   = true;
    wifiPeriodMs = 1500; // moderate
}

void StatusLed::onWifiReconnecting() {
    if (!enabled) return;
    wifiActive   = true;
    wifiPeriodMs = 600;  // fast
}

void StatusLed::onModeChanged(draw_mode) {
    if (!enabled) return;
    startBlinkN(150, 100, 2);
}

void StatusLed::onOtaStart() {
    if (!enabled) return;
    otaActive   = true;
    otaPeriodMs = 1000;
}

void StatusLed::onOtaEnd() {
    if (!enabled) return;
    otaActive = false;
    if (!wifiActive) {
        ledcWrite(LEDC_CHANNEL, 0);
    }
}

void StatusLed::onFatalError() {
    if (!enabled) return;
    fatalActive = true;
    ledcWrite(LEDC_CHANNEL, 255);
}

#endif

#if defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)

#include "modeselector.h"
#include <Adafruit_LIS3DH.h>

#define LIS3DH_ADDRESS 0x19

static Adafruit_LIS3DH accel;
static bool ready = false;

ModeSelector::ModeSelector() {};

ModeSelector::~ModeSelector() {};

bool ModeSelector::init() {
    if (!accel.begin(LIS3DH_ADDRESS)) {
        Log::instance()->error("LIS3DH connection failed\n");

        return false;
    }

    accel.setRange(LIS3DH_RANGE_2_G);
    ready = true;
    Log::instance()->info("LIS3DH initialized\n");

    return true;
};

draw_mode ModeSelector::getMode() {
    if (!ready) {
        return DRAW_MODE_NONE;
    }

    sensors_event_t event;
    if (accel.getEvent(&event)) {
        float angle = atan2f(event.acceleration.x, event.acceleration.y);
        // Log::instance()->trace(
        //     "Current orientation: %d° (%.2f rad), accel x %.2f y %.2f z %.2f\n",
        //     static_cast<int16_t>(angle * 180 / M_PI),
        //     angle,
        //     event.acceleration.x,
        //     event.acceleration.y,
        //     event.acceleration.z
        // );
        previousRead = modeFromAngle(angle);
    }

    return previousRead;
};

#endif

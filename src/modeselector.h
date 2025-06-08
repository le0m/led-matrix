#ifndef _MODESELECTOR_H
#define _MODESELECTOR_H

#include "log.h"
#include <stdint.h>
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

#define SDA_PIN 21 // GPIO21
#define SCL_PIN 19 // GPIO19

// Running mode
typedef enum {
    DRAW_MODE_NONE  = 0,
    DRAW_MODE_IMAGE = 1,
    DRAW_MODE_LIFE  = 2,
    DRAW_MODE_TBD   = 3, // TODO: the other orientation, TBD on what to do
    DRAW_MODE_QR    = 4,
} draw_mode;

class ModeSelector {
    private:
        MPU6050 mpu;
        // MPU control/status vars
        bool dmpReady = false;  // set true if DMP init was successful
        uint8_t deviceStatus;   // return status after each device operation (0 = success, !0 = error)
        uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
        uint8_t fifoBuffer[64]; // FIFO storage buffer
        // orientation/motion vars
        Quaternion q;           // [w, x, y, z]         quaternion container
        VectorFloat gravity;    // [x, y, z]            gravity vector
        float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll in radians

    public:
        ModeSelector();
        ~ModeSelector();
        bool init();
        draw_mode getMode();
};

#endif

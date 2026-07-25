#if !defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3)

#include "modeselector.h"
#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps612.h"

/**
 * Original code taken from https://github.com/jrowberg/i2cdevlib/blob/170bad1f4a97b074cd33238a5523c7382a3d8f1e/Arduino/MPU6050/examples/MPU6050_DMP6_using_DMP_V6.12/MPU6050_DMP6_using_DMP_V6.12.ino
 */

#define SDA_PIN 21
#define SCL_PIN 19

static MPU6050 mpu;
// MPU control/status vars
static bool dmpReady = false;  // set true if DMP init was successful
static uint8_t deviceStatus;   // return status after each device operation (0 = success, !0 = error)
static uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
static uint8_t fifoBuffer[64]; // FIFO storage buffer
// orientation/motion vars
static Quaternion q;           // [w, x, y, z]         quaternion container
static VectorFloat gravity;    // [x, y, z]            gravity vector
static float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll in radians

ModeSelector::ModeSelector() {};

ModeSelector::~ModeSelector() {};

bool ModeSelector::init() {
    Wire.setPins(SDA_PIN, SCL_PIN);
    Wire.begin();
    Wire.setClock(400000);
    mpu.initialize();
    if (!mpu.testConnection()) {
        Log::instance()->error("MPU6050 connection failed\n");

        return false;
    }

    deviceStatus = mpu.dmpInitialize();
    if (deviceStatus != 0) {
        Log::instance()->error("DMP Initialization failed (code %d)\n", deviceStatus);

        return false;
    }

    mpu.setXGyroOffset(51);
    mpu.setYGyroOffset(8);
    mpu.setZGyroOffset(21);
    mpu.setXAccelOffset(1150);
    mpu.setYAccelOffset(-50);
    mpu.setZAccelOffset(1060);
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.setDMPEnabled(true);
    dmpReady = true;
    packetSize = mpu.dmpGetFIFOPacketSize();
    Log::instance()->info("MPU6050 initialized and calibrated\n");

    return true;
};

draw_mode ModeSelector::getMode() {
    if (!dmpReady) {
        return DRAW_MODE_NONE;
    }

    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        // Log::instance()->trace(
        //     "Current orientation: pitch %d° (%.2f rad), roll %d° (%.2f rad)\n",
        //     static_cast<int16_t>(ypr[1] * 180 / M_PI),
        //     ypr[1],
        //     static_cast<int16_t>(ypr[2] * 180 / M_PI),
        //     ypr[2]
        // );
        previousRead = modeFromAngle(ypr[1]);
    }

    return previousRead;
};

#endif

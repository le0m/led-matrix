#include "modeselector.h"

/**
 * Original code taken from https://github.com/jrowberg/i2cdevlib/blob/170bad1f4a97b074cd33238a5523c7382a3d8f1e/Arduino/MPU6050/examples/MPU6050_DMP6_using_DMP_V6.12/MPU6050_DMP6_using_DMP_V6.12.ino
 */

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
    Log::instance()->info("MP6050 initialized and calibrated\n");

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
        if (-M_PI_4 <= ypr[1] && ypr[1] <= M_PI_4) { // up
            return DRAW_MODE_IMAGE;
        } else if (M_PI_4 < ypr[1] && ypr[1] <= M_3PI_4) { // right
            return DRAW_MODE_LIFE;
        } else if (-M_3PI_4 <= ypr[1] && ypr[1] < -M_PI_4) { // left
            return DRAW_MODE_TBD;
        } else { // down
            return DRAW_MODE_QR;
        }
    }

    return DRAW_MODE_NONE;
};

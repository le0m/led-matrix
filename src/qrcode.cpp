#include "qrcode.h"

QRCode::QRCode(uint8_t w, uint8_t h) {
    width = w;
    height = h;
};

QRCode::~QRCode() {};

bool QRCode::setText(String str) {
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(2)];
    if (!qrcodegen_encodeText(
        str.c_str(),
        tempBuffer,
        qr0,
        qrcodegen_Ecc_QUARTILE,
        qrcodegen_VERSION_MIN,
        2,
        qrcodegen_Mask_AUTO,
        true
    )) {
        return false;
    }

    text = str;
    changed = true;

    return true;
};

void QRCode::initServer(AsyncWebServer* server) {};

void QRCode::render(MatrixPanel_I2S_DMA* display) {
    if (!changed) {
        return;
    }

    int size = qrcodegen_getSize(qr0);
    uint8_t startX = (width - size) / 2;
    uint8_t startY = (height - size) / 2;
    display->fillRect(startX - QR_BORDER_PX, startY - QR_BORDER_PX, size + QR_BORDER_PX * 2, size + QR_BORDER_PX * 2, display->color565(255, 255, 255));
    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            if (qrcodegen_getModule(qr0, x, y)) {
                display->drawPixelRGB888(x + startX, y + startY, 0, 0, 0);
            }
        }
    }

    changed = false;
};

void QRCode::print() {
    Log::instance()->info("Text: %s\n", text);
};

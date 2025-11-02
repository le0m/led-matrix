#ifndef _QRCODE_H
#define _QRCODE_H

#include "../modules/log.h"
#include "renderer.h"
#include "qrcodegen.h"
#define QR_BORDER_PX 4
#define QR_VERSION 2
#define QR_TEXT_MAX_LENGTH QR_VERSION * 4 + 17

class QRCode : public Renderer {
    private:
        uint8_t qr0[qrcodegen_BUFFER_LEN_FOR_VERSION(2)];
        char text[QR_TEXT_MAX_LENGTH];
        bool changed = true;

    public:
        QRCode(uint8_t, uint8_t);
        virtual ~QRCode();
        virtual void initServer(AsyncWebServer*);
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
        virtual bool setText(const char*);
        virtual bool open();
        virtual bool close();
};

#endif

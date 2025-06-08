#ifndef _QRCODE_H
#define _QRCODE_H

#include "log.h"
#include "renderer.h"
#include "qrcodegen.h"
#define QR_BORDER_PX 4

class QRCode : public Renderer {
    private:
        uint8_t qr0[qrcodegen_BUFFER_LEN_FOR_VERSION(2)];
        String text;
        bool changed = true;

    public:
        QRCode(uint8_t, uint8_t);
        virtual ~QRCode();
        virtual void initServer(AsyncWebServer*);
        virtual void render(MatrixPanel_I2S_DMA*);
        virtual void print();
        virtual bool setText(String);
};

#endif

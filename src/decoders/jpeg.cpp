#include "jpeg.h"

DecoderJPEG::DecoderJPEG() {};

DecoderJPEG::~DecoderJPEG() {
    close();
};

bool DecoderJPEG::fileExists() {
    return Filesystem::pathExists(JPEG_FILE_PATH);
};

bool DecoderJPEG::deleteFile() {
    return Filesystem::deleteFile(JPEG_FILE_PATH);
};

bool DecoderJPEG::writeFile(uint8_t *data, size_t len) {
    return Filesystem::writeBytes(JPEG_FILE_PATH, data, len);
};

bool DecoderJPEG::sendFile(AsyncWebServerRequest *request) {
    if (!fileExists()) {
        return false;
    }

    Log::instance()->info("Sending current JPEG\n");
    request->send(LittleFS, JPEG_FILE_PATH, JPEG_FILE_TYPE);

    return true;
};

bool DecoderJPEG::open() {
    if (isOpen_) {
        return true;
    }


    lastRender = 0;
    isOpen_ = true;

    return jpeg.open(JPEG_FILE_PATH, openFile, closeFile, readFile, seekFile, draw);
};

void DecoderJPEG::close() {
    if (!isOpen_) {
        return;
    }

    isOpen_ = false;
    delay(50); // wait for possible renderFrame() execution to finish
    jpeg.close();
    lastRender = 0;
};

void DecoderJPEG::renderFrame(MatrixPanel_I2S_DMA  *display) {
    // Render once
    if (!isOpen_ || lastRender > 0) {
        return;
    }

    lastRender = millis();
    jpeg.setUserPointer(static_cast<void*>(display));
    if (!jpeg.decode(0, 0, 0)) {
        Log::instance()->error("Error decoding JPEG: %d\n", jpeg.getLastError());
    }
};

void* DecoderJPEG::openFile(const char *path, int32_t *size) {
    File *file = new File();
    *file = LittleFS.open(path);
    if (file) {
        *size = file->size();

        return static_cast<void*>(file);
    }

    return NULL;
};

void DecoderJPEG::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) {
        f->close();
    }
};

int32_t DecoderJPEG::readFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen) {
        iBytesRead = pFile->iSize - pFile->iPos - 1; // <-- ugly work-around
    }
    if (iBytesRead <= 0) {
        return 0;
    }

    iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();

    return iBytesRead;
};

int32_t DecoderJPEG::seekFile(JPEGFILE *pFile, int32_t iPosition) {
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();

    return pFile->iPos;
};

int DecoderJPEG::draw(JPEGDRAW *pDraw) {
    MatrixPanel_I2S_DMA *display = static_cast<MatrixPanel_I2S_DMA*>(pDraw->pUser);
    uint16_t *s = pDraw->pPixels;
    int lastX = pDraw->x + pDraw->iWidth;
    int lastY = pDraw->y + pDraw->iHeight;
    for (int y = pDraw->y; y < lastY; y++) {
        for (int x = pDraw->x; x < lastX; x++) {
            display->drawPixel(x, y, *s++);
        }
    }

    return 1;
};

#include "image.h"

Image::Image() {};

Image::~Image() {
    jpeg.close();
};

bool Image::open(const char *path) {
    if (isOpen) {
        return true;
    }

    filePath = path;
    isOpen = true;

    return jpeg.open(filePath, openFile, closeFile, readFile, seekFile, draw);
};

void Image::close() {
    if (!isOpen) {
        return;
    }

    isOpen = false;
    delay(50); // wait for possible renderFrame() execution to finish
    jpeg.close();
    lastRender = 0;
};

void Image::renderFrame(MatrixPanel_I2S_DMA  *display) {
    // Re-render at most once a second
    if (!isOpen || millis() - lastRender < 1000) {
        return;
    }

    // Re-open image to rewind file position, otherwise calling decode() more than once corrupts the displayed image
    jpeg.open(filePath, openFile, closeFile, readFile, seekFile, draw);
    lastRender = millis();
    jpeg.setUserPointer(static_cast<void*>(display));
    if (!jpeg.decode(0, 0, 0)) {
        Log::instance()->error("Error decoding JPEG: %d\n", jpeg.getLastError());
    }

    jpeg.close();
};

void* Image::openFile(const char *path, int32_t *size) {
    File *file = new File();
    *file = LittleFS.open(path);
    if (file) {
        *size = file->size();

        return static_cast<void*>(file);
    }

    delete file;

    return NULL;
};

void Image::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) {
        f->close();
        delete f;
    }
};

int32_t Image::readFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen) {
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

int32_t Image::seekFile(JPEGFILE *pFile, int32_t iPosition) {
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();

    return pFile->iPos;
};

int Image::draw(JPEGDRAW *pDraw) {
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

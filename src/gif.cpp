#include "gif.h"

/**
 * Original code taken from https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA/blob/fb3499fb66fb4941af8a818eb9be0548fe4b7a60/examples/AnimatedGIFPanel_LittleFS/AnimatedGIFPanel_LittleFS.ino
 */

GIF::GIF() {};

GIF::~GIF() {
    gif.close();
};

bool GIF::open(const char *path) {
    if (isOpen) {
        return true;
    }

    isOpen = true;
    gif.begin(GIF_PALETTE_RGB565_LE);

    return gif.open(path, openFile, closeFile, readFile, seekFile, drawLine);
};

void GIF::close() {
    if (!isOpen) {
        return;
    }

    isOpen = false;
    delay(50); // wait for possible renderFrame() execution to finish
    gif.close();
    frameDelay = 0;
    lastGifFrameRender = 0;
};

void GIF::renderFrame(MatrixPanel_I2S_DMA  *display) {
    if (!isOpen || millis() - lastGifFrameRender < frameDelay) {
        return;
    }

    lastGifFrameRender = millis();
    switch (gif.playFrame(false, &frameDelay, static_cast<void*>(display))) {
        case 0:
            gif.reset();
            break;

        case -1:
            Log::instance()->error("Error decoding GIF frame: %d\n", gif.getLastError());
            break;
    }
};

void* GIF::openFile(const char *fname, int32_t *pSize) {
    File *file = new File();
    *file = LittleFS.open(fname);
    if (file) {
        *pSize = file->size();

        return static_cast<void*>(file);
    }

    delete file;

    return NULL;
};

void GIF::closeFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);
    if (f != NULL) {
        f->close();
        delete f;
    }
};

int32_t GIF::readFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
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

int32_t GIF::seekFile(GIFFILE *pFile, int32_t iPosition) {
    File *f = static_cast<File *>(pFile->fHandle);
    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();

    return pFile->iPos;
};

// Draw a line of image directly on the LED Matrix
void GIF::drawLine(GIFDRAW *pDraw) {
    MatrixPanel_I2S_DMA *display = static_cast<MatrixPanel_I2S_DMA*>(pDraw->pUser);
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth > display->width()) {
        iWidth = display->width();
    }

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line
    s = pDraw->pPixels;
    // restore to background color
    if (pDraw->ucDisposalMethod == 2){
        for (x = 0; x < iWidth; x++)
        {
            if (s[x] == pDraw->ucTransparent)
                s[x] = pDraw->ucBackground;
        }
        pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    // if transparency used
    if (pDraw->ucHasTransparency) {
        uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
        int x, iCount;
        pEnd = s + pDraw->iWidth;
        x = 0;
        iCount = 0; // count non-transparent pixels
        while (x < pDraw->iWidth) {
            c = ucTransparent - 1;
            d = usTemp;
            while (c != ucTransparent && s < pEnd) {
                c = *s++;
                // done, stop
                if (c == ucTransparent) {
                    s--; // back up to treat it like transparent
                } else {
                    // opaque
                    *d++ = usPalette[c];
                    iCount++;
                }
            } // while looking for opaque pixels
            // any opaque pixels?
            if (iCount) {
                for (int xOffset = 0; xOffset < iCount; xOffset++) {
                    display->drawPixel(x + xOffset, y, usTemp[xOffset]); // 565 Color Format
                }

                x += iCount;
                iCount = 0;
            }

            // no, look for a run of transparent pixels
            c = ucTransparent;
            while (c == ucTransparent && s < pEnd) {
                c = *s++;
                if (c == ucTransparent) {
                    iCount++;
                } else {
                    s--;
                }
            }
            if (iCount) {
                x += iCount; // skip these
                iCount = 0;
            }
        }
    } else {
        // does not have transparency
        s = pDraw->pPixels;
        // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
        for (x = 0; x < pDraw->iWidth; x++) {
            display->drawPixel(x, y, usPalette[*s++]); // color 565
        }
    }
};

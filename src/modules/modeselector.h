#ifndef _MODESELECTOR_H
#define _MODESELECTOR_H

#include <math.h>
#include <stdint.h>
#include "log.h"

#ifndef M_3PI_4
#define M_3PI_4 (3.0 * M_PI_4)
#endif

// Running mode
typedef enum {
    DRAW_MODE_NONE  = 0,
    DRAW_MODE_IMAGE = 1,
    DRAW_MODE_LIFE  = 2,
    DRAW_MODE_MAP   = 3,
    DRAW_MODE_QR    = 4,
} draw_mode;

class ModeSelector {
    private:
        draw_mode previousRead = DRAW_MODE_NONE;

        draw_mode modeFromAngle(float angle) {
            if (-M_PI_4 <= angle && angle <= M_PI_4) { // up
                return DRAW_MODE_IMAGE;
            } else if (M_PI_4 < angle && angle <= M_3PI_4) { // right
                return DRAW_MODE_LIFE;
            } else if (-M_3PI_4 <= angle && angle < -M_PI_4) { // left
                return DRAW_MODE_MAP;
            }

            return DRAW_MODE_QR; // down
        };

    public:
        ModeSelector();
        ~ModeSelector();
        bool init();
        draw_mode getMode();
};

#endif

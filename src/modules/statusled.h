#ifndef _STATUSLED_H
#define _STATUSLED_H

#include <stdint.h>
#include "modeselector.h"

class StatusLed {
    public:
        void init();
        void tick();
        void clear();
        void onBoot();
        void onWifiConnected();
        void onApStarted();
        void onWifiReconnecting();
        void onModeChanged(draw_mode mode);
        void onOtaStart();
        void onOtaEnd();
        void onFatalError();
        void setEnabled(bool);
};

#endif

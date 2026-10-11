#include "main.h"

HUB75_I2S_CFG::i2s_pins pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
HUB75_I2S_CFG ledConfig(
    WIDTH,                   // matrix width
    HEIGHT,                  // matrix height
    CHAIN,                   // chain length
    pins,                    // GPIO map
    HUB75_I2S_CFG::SHIFTREG, // matrix module chip
    false,                   // DMA double buffer (double RAM)
    I2C_CLK,                 // i2s clock speed
    LAT_BLANKING,            // how many clock cycles to blank before/after LAT change
    CLK_PHASE,               // i2s clock phase (positive/negative edge)
    60,                      // minimum refresh rate
    PIXEL_COLOR_DEPTH_BITS   // color depth
);
MatrixPanel_I2S_DMA *display = nullptr;

bool setupOk = false;
OTA ota(1000);
draw_mode drawMode = DRAW_MODE_NONE;
WiFiController wifi;
Life life(WIDTH, HEIGHT);
Media media(WIDTH, HEIGHT);
Map mapp(WIDTH, HEIGHT);
QRCode qr(WIDTH, HEIGHT);
AsyncWebServer server(80);
ModeSelector selector;
Config conf(updateConfig);
Renderer *currentMode = nullptr;
Monitor* monitor = nullptr;
StatusLed statusLed;
wl_status_t prevWifiStatus = WL_IDLE_STATUS;
wifi_mode_t prevWifiMode = WIFI_MODE_NULL;
bool otaWasUpdating = false;
ulong lastModePoll = 0;

void updateQrText() {
    const char* url = wifi.getUrl();
    if (url == nullptr) {
        Log::instance()->error("Error getting URL for QR code\n");

        return;
    }

    Log::instance()->info("QR code URL: %s\n", url);
    if (url[0] != '\0' && !qr.setText(url)) {
        Log::instance()->error("Error encoding IP to QR code\n");
    }
};

bool changeMode(draw_mode newMode) {
    if (newMode == drawMode) {
        return false;
    }

    if (currentMode != nullptr) {
        currentMode->close();
    }

    switch (newMode) {
        case DRAW_MODE_LIFE:
            currentMode = &life;
            break;
        case DRAW_MODE_QR:
            currentMode = &qr;
            break;
        case DRAW_MODE_IMAGE:
            currentMode = &media;
            break;
        case DRAW_MODE_MAP:
            currentMode = &mapp;
            break;

        default:
            currentMode = nullptr;
            break;
    }

    if (currentMode != nullptr) {
        currentMode->open();
    }

    drawMode = newMode;
    Log::instance()->info("Changed mode to %d\n", drawMode);
    if (display != nullptr) {
        display->clearScreen();
    }

    if (newMode == DRAW_MODE_QR) {
        updateQrText();
    }

    statusLed.onModeChanged(newMode);
    return true;
};

void updateConfig(JsonDocument& newConfig) {
    String newSsid = newConfig["wifi"]["ssid"].as<String>();
    String newPassword = newConfig["wifi"]["password"].as<String>();
    String currentSsid = conf.current["wifi"]["ssid"].as<String>();
    String currentPassword = conf.current["wifi"]["password"].as<String>();

    if (!newSsid.equals(currentSsid) || !newPassword.equals(currentPassword)) {
        Log::instance()->info("WiFi configuration changed, reconnecting\n");
        wifi.connect(newSsid.c_str(), newPassword.c_str());
    }
    if (display != nullptr && newConfig["panel"]["brightness"].as<uint8_t>() != conf.current["panel"]["brightness"].as<uint8_t>()) {
        Log::instance()->info("Panel brightness changed: %d\n", newConfig["panel"]["brightness"].as<uint8_t>());
        display->setBrightness(newConfig["panel"]["brightness"].as<uint8_t>());
    }
    if (newConfig["log"]["level"].as<uint8_t>() != conf.current["log"]["level"].as<uint8_t>()) {
        Log::instance()->setLevel(newConfig["log"]["level"].as<log_level>());
        Log::instance()->info("Log level changed: %s\n", Log::instance()->stringLevel().c_str());
    }

    mapp.setConfig(newConfig["map"].as<JsonVariantConst>());

    if (monitor != nullptr) {
        monitor->setEnabled(newConfig["monitor"]["enable"].as<bool>());
    }

    statusLed.setEnabled(newConfig["statusled"]["enable"].as<bool>());
};

void setup() {
    Log::instance()->begin(115200);
    statusLed.init();
    statusLed.onBoot();

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Log::instance()->error("LittleFS error\n");
        statusLed.onFatalError();

        return;
    }

    Filesystem::deleteFile(UPLOAD_TMP_PATH);

    // Read configuration
    conf.load();

    // Configure logs
    Log::instance()->setLevel(conf.current["log"]["level"].as<log_level>());
    statusLed.setEnabled((conf.current["statusled"]["enable"] | 1) != 0);

    // Initialize LED matrix
    display = setupLED(ledConfig);
    if (display == nullptr) {
        Log::instance()->error("Error setting up LED matrix\n");
        statusLed.onFatalError();

        return;
    }
    display->setBrightness(conf.current["panel"]["brightness"].as<uint8_t>());

    // Initialize orientation sensor
    if (!selector.init()) {
        Log::instance()->error("Mode selector unavailable, staying in default mode\n");
    }

    // Start media as default mode
    currentMode = &media;
    currentMode->open();
    drawMode = DRAW_MODE_IMAGE;

    // Initialize WiFi
    String ssid = conf.current["wifi"]["ssid"].as<String>();
    String password = conf.current["wifi"]["password"].as<String>();
    if (!wifi.connect(ssid.c_str(), password.c_str())) {
        statusLed.onFatalError();

        return;
    }

    if (wifi.getMode() & WIFI_MODE_AP) {
        statusLed.onApStarted();
    } else {
        statusLed.onWifiConnected();
        prevWifiStatus = WL_CONNECTED;
    }
    prevWifiMode = wifi.getMode();

    updateQrText();

    // Initialize log WebSocket
    Log::instance()->initServer(&server);

    // Initialize OTA
    ota.initServer(&server);

    // Initialize modules
    monitor = new Monitor(&wifi);
    monitor->initServer(&server);
    monitor->setEnabled(conf.current["monitor"]["enable"].as<bool>());
    media.initServer(&server);
    mapp.initServer(&server);
    mapp.setConfig(conf.current["map"].as<JsonVariantConst>());
    life.setFPS(FPS);
    conf.initServer(&server);

    // Setup server
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(404);
    });
    server.on("/status", HTTP_GET, [&](AsyncWebServerRequest *request) {
        Log::instance()->info("Sending current status\n");
        JsonDocument status;
        status["heap"]["size"] = ESP.getHeapSize();
        status["heap"]["free"] = ESP.getFreeHeap();
        status["heap"]["minFree"] = ESP.getMinFreeHeap();
        status["heap"]["maxFreeBlock"] = ESP.getMaxAllocHeap();
        status["flash"] = ESP.getFlashChipSize();
        status["psram"]["found"] = psramFound();
        status["psram"]["size"] = ESP.getPsramSize();
        status["psram"]["free"] = ESP.getFreePsram();
        status["psram"]["minFree"] = ESP.getMinFreePsram();
        status["psram"]["maxFreeBlock"] = ESP.getMaxAllocPsram();
        status["firmware"]["size"] = ESP.getFreeSketchSpace(); // see: https://github.com/espressif/arduino-esp32/issues/3131
        status["firmware"]["used"] = ESP.getSketchSize();
        status["firmware"]["version"] = VERSION; // defined at build time, see pre_version.py
        status["filesystem"]["size"] = LittleFS.totalBytes();
        status["filesystem"]["used"] = LittleFS.usedBytes();
        status["chip"]["model"] = ESP.getChipModel();
        status["chip"]["revision"] = ESP.getChipRevision();
        status["chip"]["cores"] = ESP.getChipCores();
        status["chip"]["clock"] = ESP.getCpuFreqMHz();
        status["sdk"] = ESP.getSdkVersion();
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        serializeJson(status, *response);
        response->setCode(200);
        request->send(response);
    });
    server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");

    server.begin();
    // Filesystem::tree("/", 0);
    setupOk = true;
};

void loop() {
    if (!setupOk) {
        return;
    }

    ota.loop();
    statusLed.tick();

    bool nowUpdating = ota.isUpdating();
    if (nowUpdating && !otaWasUpdating) {
        statusLed.onOtaStart();
    } else if (!nowUpdating && otaWasUpdating) {
        statusLed.onOtaEnd();
    }
    otaWasUpdating = nowUpdating;

    if (millis() - lastModePoll >= 1000) {
        lastModePoll = millis();

        wifi_mode_t wifiMode = wifi.getMode();
        if (wifiMode != prevWifiMode) {
            if (wifiMode & WIFI_MODE_AP) {
                statusLed.onApStarted();
            }
            if (!(wifiMode & WIFI_MODE_STA)) {
                prevWifiStatus = WL_IDLE_STATUS;
            }
            prevWifiMode = wifiMode;
        }
        if (wifiMode & WIFI_MODE_STA) {
            wl_status_t wifiStatus = WiFi.status();
            if (wifiStatus != WL_CONNECTED && prevWifiStatus == WL_CONNECTED) {
                statusLed.onWifiReconnecting();
            } else if (wifiStatus == WL_CONNECTED && prevWifiStatus != WL_CONNECTED) {
                statusLed.onWifiConnected();
            }
            prevWifiStatus = wifiStatus;
        }

        if (changeMode(selector.getMode())) {
            return;
        }
    }

    if (currentMode != nullptr) {
        currentMode->render(display);
    }
};

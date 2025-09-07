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
    true,                    // i2s clock phase (positive/negative edge)
    60,                      // minimum refresh rate
    PIXEL_COLOR_DEPTH_BITS   // color depth
);
MatrixPanel_I2S_DMA *display = nullptr;

OTA ota(1000);
draw_mode drawMode = DRAW_MODE_NONE;
WiFiController wifi;
Life life(WIDTH, HEIGHT);
Image image(WIDTH, HEIGHT);
QRCode qr(WIDTH, HEIGHT);
AsyncWebServer server(80);
ModeSelector selector;
Config conf(updateConfig);

void updateQrText() {
    String url = wifi.getUrl();
    Log::instance()->info("QR code URL: %s\n", url.c_str());
    if (url != "" && !qr.setText(url)) {
        Log::instance()->error("Error encoding IP to QR code\n");
    }
};

void changeMode(draw_mode newMode) {
    if (newMode == drawMode) {
        return;
    }

    drawMode = newMode;
    Log::instance()->info("Changed mode to %d\n", drawMode);
    display->clearScreen();
    if (newMode == DRAW_MODE_QR) {
        updateQrText();
    }
};

void updateConfig(JsonDocument newConfig) {
    if (newConfig["wifi"]["ssid"].as<String>() != conf.current["wifi"]["ssid"].as<String>() || newConfig["wifi"]["password"].as<String>() != conf.current["wifi"]["password"].as<String>()) {
        Log::instance()->info("WiFi configuration changed, reconnecting\n");
        wifi.connect(newConfig["wifi"]["ssid"].as<String>(), newConfig["wifi"]["password"].as<String>());
    }
    if (newConfig["panel"]["brightness"].as<uint8_t>() != conf.current["panel"]["brightness"].as<uint8_t>()) {
        Log::instance()->info("Panel brightness changed: %d\n", newConfig["panel"]["brightness"].as<uint8_t>());
        display->setBrightness(newConfig["panel"]["brightness"].as<uint8_t>());
    }
    if (newConfig["log"]["level"].as<uint8_t>() != conf.current["log"]["level"].as<uint8_t>()) {
        Log::instance()->setLevel(newConfig["log"]["level"].as<log_level>());
        Log::instance()->info("Log level changed: %s\n", Log::instance()->stringLevel());
    }
};

void setup() {
    Log::instance()->begin(115200);

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Log::instance()->error("LittleFS error\n");

        return;
    }

    // Read configuration
    conf.load();

    // Configure logs
    Log::instance()->setLevel(conf.current["log"]["level"].as<log_level>());
#ifdef LOG_QUEUE
    Log::instance()->setQueueSize(50);
#endif

    // Initialize LED matrix
    display = setupLED(ledConfig);
    display->setBrightness(conf.current["panel"]["brightness"].as<uint8_t>());

    // Initialize gyroscope
    if (!selector.init()) {
        return;
    }

    // Initialize WiFi
    if (!wifi.connect(conf.current["wifi"]["ssid"].as<String>(), conf.current["wifi"]["password"].as<String>())) {
        return;
    }
    updateQrText();

    // Initialize log WebSocket
    Log::instance()->initServer(&server);

    // Initialize OTA
    ota.initServer(&server);

    // Initialize modules
    image.initServer(&server);
    image.loadMedia();
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
};

ulong lastModePoll = 0;

void loop() {
    ota.loop();

    if (lastModePoll + 1000 < millis()) {
        lastModePoll = millis();
        changeMode(selector.getMode());
    }
    switch (drawMode) {
        case DRAW_MODE_LIFE:
            life.render(display);
            break;

        case DRAW_MODE_IMAGE:
            image.render(display);
            break;

        case DRAW_MODE_QR:
            qr.render(display);
            break;
    }
};

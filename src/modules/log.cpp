#include "log.h"

Log::Log(): level(LOG_LEVEL_INFO) {};
Log::Log(log_level l) : level(l) {};

Log::~Log() {
    if (websocket != nullptr) {
        websocket->closeAll();
        delete websocket;
    }
    if (handler != nullptr) {
        delete handler;
    }
};

Log* Log::instance() {
    static Log singleton;

    return &singleton;
};

void Log::setLevel(log_level l) {
    level = l;
    info("Log level: %d\n", level);
};

std::string Log::stringLevel() {
    switch (level) {
        case LOG_LEVEL_ERROR:
            return "error";
        case LOG_LEVEL_WARNING:
            return "warning";
        case LOG_LEVEL_INFO:
            return "info";
        case LOG_LEVEL_DEBUG:
            return "debug";
        case LOG_LEVEL_TRACE:
            return "trace";
        default:
            return "unknown";
    }
};

void Log::begin(unsigned long baud) {
    Serial.begin(baud);
    info("Log level: %d\n", level);
};

void Log::initServer(AsyncWebServer *server) {
    handler = new AsyncWebSocketMessageHandler();
    websocket = new AsyncWebSocket("/ws", handler->eventHandler());
    server->addHandler(websocket);
};

void Log::broadcast(std::string message) {
    if (handler == nullptr || websocket == nullptr) {
        return;
    }

    websocket->cleanupClients();
    if (websocket->count() > 0) {
        websocket->textAll(message.c_str());
    }
};

std::string Log::format(const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    std::vector<char> msg(len + 1);
    vsnprintf(msg.data(), len + 1, fmt, args);

    return std::string(msg.data());
};

void Log::error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);
    Serial.print(message.c_str());
    broadcast(message);
};

void Log::warning(const char *fmt, ...) {
    if (level < LOG_LEVEL_WARNING) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);
    Serial.println(message.c_str());
    broadcast(message);
};

void Log::info(const char *fmt, ...) {
    if (level < LOG_LEVEL_INFO) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);
    Serial.println(message.c_str());
    broadcast(message);
};

void Log::debug(const char *fmt, ...) {
    if (level < LOG_LEVEL_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);
    Serial.println(message.c_str());
    broadcast(message);
};

void Log::trace(const char *fmt, ...) {
    if (level < LOG_LEVEL_TRACE) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    std::string message = format(fmt, args);
    va_end(args);
    Serial.println(message.c_str());
    broadcast(message);
};

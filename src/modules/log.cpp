#include "log.h"

#ifdef LOG_QUEUE
Log::Log(): level(LOG_LEVEL_INFO), queueSize(50) {};
Log::Log(log_level l) : level(l), queueSize(50) {};

Log::Log(log_level l, uint8_t s) : level(l), queueSize(s) {};
#else
Log::Log(): level(LOG_LEVEL_INFO) {};
Log::Log(log_level l) : level(l) {};
#endif

Log::~Log() {
    if (websocket != nullptr) {
        websocket->closeAll();
        free(websocket);
    }
    if (handler != nullptr) {
        free(handler);
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

#ifdef LOG_QUEUE
void Log::setQueueSize(uint8_t s) {
    queueSize = s;
};
#endif

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
#ifdef LOG_QUEUE
    queue.insert(queue.begin(), message);
    while (queue.size() >= queueSize) {
        queue.pop_back();
    }
#endif

    if (handler == nullptr || websocket == nullptr) {
        return;
    }

    websocket->cleanupClients();
    if (websocket->count() > 0) {
#ifdef LOG_QUEUE
        while (queue.size() > 0) {
            websocket->textAll(queue.back().c_str());
            queue.pop_back();
        }
#else
        websocket->textAll(message.c_str());
#endif

        return;
    }
};

std::string Log::format(const char *fmt, va_list args) {
    std::vector<char> msg;
    int len = vsnprintf(&msg[0], 0, fmt, args);
    msg.resize(len + 1);
    vsnprintf(&msg[0], len, fmt, args);

    return std::string(&msg[0]);
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

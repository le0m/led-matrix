#ifndef _LOG_H
#define _LOG_H

#include <ESPAsyncWebServer.h>

typedef enum {
    LOG_LEVEL_ERROR   = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO    = 2,
    LOG_LEVEL_DEBUG   = 3,
    LOG_LEVEL_TRACE   = 4,
} log_level;

class Log {
    private:
        log_level level = LOG_LEVEL_INFO;
        bool serverInitialized = false;
        AsyncWebSocketMessageHandler* handler = nullptr;
        AsyncWebSocket* websocket = nullptr;
        void broadcast(std::string);
        std::string format(const char*, va_list);

    public:
        Log();
        Log(log_level);
        ~Log();
        static Log* instance();
        void setLevel(log_level);
        std::string stringLevel();
        void begin(unsigned long);
        void initServer(AsyncWebServer*);
        void error(const char*, ...);
        void warning(const char *fmt, ...);
        void info(const char *fmt, ...);
        void debug(const char *fmt, ...);
        void trace(const char *fmt, ...);
};

#endif

#ifndef LOG_H
#define LOG_H

#include <stdlib.h>

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
} Log_Level;

void log_output_fmt(Log_Level level, const char *fmt, ...);

#define LOG_INFO(fmt, ...) log_output_fmt(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__);
#define LOG_DEBUG(fmt, ...) log_output_fmt(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__);
#define LOG_WARNING(fmt, ...) log_output_fmt(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) log_output_fmt(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);
#define LOG_FATAL(fmt, ...)                                  \
    do {                                                     \
        log_output_fmt(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__); \
        exit(1);                                             \
    } while(0)

#endif
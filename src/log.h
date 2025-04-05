#ifndef LOG_H
#define LOG_H

typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} Log_Level;

void log_output_fmt(Log_Level level, const char *fmt, ...);

#define LOG_INFO(fmt, ...) log_output_fmt(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__);
#define LOG_WARNING(fmt, ...) log_output_fmt(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) log_output_fmt(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);

#endif
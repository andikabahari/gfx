#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "platform.h"

void log_output_fmt(Log_Level level, const char *fmt, ...)
{
    size_t max_length = 4 * 1024; // 4 KiB
    char buffer[max_length];

    // Initialize the buffer to empty
    buffer[0] = '\0';

    switch (level) {
        case LOG_LEVEL_INFO:
            strcat_s(buffer, max_length, "[INFO] ");
            break;
            
        case LOG_LEVEL_DEBUG:
            strcat_s(buffer, max_length, "[DEBUG] ");
            break;
            
        case LOG_LEVEL_WARNING:
            strcat_s(buffer, max_length, "[WARNING] ");
            break;
            
        case LOG_LEVEL_ERROR:
            strcat_s(buffer, max_length, "[ERROR] ");
            break;

        case LOG_LEVEL_FATAL:
            strcat_s(buffer, max_length, "[FATAL] ");
            break;

        default:
            return;
    }

    va_list args;
    va_start(args, fmt);
    size_t buffer_length = strlen(buffer);
    vsnprintf_s(
        buffer + buffer_length,
        max_length - buffer_length,
        max_length - buffer_length - 1,
        fmt, args);
    va_end(args);

    platform_log_output(level, buffer);
}
#include "log.h"

#include "common.h"
#include "platform.h"

void log_output_fmt(Log_Level level, const char *fmt, ...)
{
    size_t buffer_length = 4 * 1024; // 4 KiB
    char buffer[buffer_length];

    // Initialize the buffer to empty
    buffer[0] = '\0';

    switch (level) {
        case LOG_LEVEL_INFO:
            strcat_s(buffer, buffer_length, "[INFO] ");
            break;
        case LOG_LEVEL_WARNING:
            strcat_s(buffer, buffer_length, "[WARNING] ");
            break;
        case LOG_LEVEL_ERROR:
            strcat_s(buffer, buffer_length, "[ERROR] ");
            break;
        default:
            return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + strlen(buffer), buffer_length - strlen(buffer), fmt, args);
    va_end(args);

    platform_log_output(level, buffer);
}
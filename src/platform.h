#ifndef PLATFORM_H
#define PLATFORM_H

#include "log.h"
#include "vulkan_types.h"

#if defined(_WIN32) || defined(_WIN64)
#   define PLATFORM_WINDOWS 1
#   define PLATFORM_NAME "Windows"
#elif defined(__linux__) || defined(__gnu_linux__)
#   define PLATFORM_LINUX 1
#   define PLATFORM_NAME "Linux"
#else
#   define PLATFORM_UNKNOWN 1
#   define PLATFORM_NAME "Unsupported platform"
#endif

typedef struct {
    void *handle;
} Platform_Window;

void platform_window_init(Platform_Window *window, const char *title, int x, int y, int width, int height);
void platform_window_destroy(Platform_Window *window);
void platform_window_handle_message(Platform_Window *window);
void platform_window_create_vulkan_surface(Platform_Window *window, Vulkan_Context *context);

void platform_log_output(Log_Level level, const char *msg);

#endif
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "platform.h"

int main(void)
{    
    LOG_INFO("%s\n", "Starting application");

    Platform_Window window;
    platform_window_init(&window, "App window", 100, 100, 1280, 720);
    platform_window_destroy(&window);
    
    return 0;
}
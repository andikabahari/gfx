#include "log.h"
#include "platform.h"
#include "array.h"

int main(void)
{    
    LOG_INFO("Starting application\n");

    Platform_Window window;
    platform_window_init(&window, "App window", 100, 100, 1280, 720);
    platform_window_destroy(&window);
    
    return 0;
}
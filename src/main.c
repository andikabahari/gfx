#include "log.h"
#include "platform.h"
#include "array.h"
#include "event.h"
#include "input.h"
#include "vulkan.h"

bool is_running = true;

bool handle_key_pressed(int event_type, void *listener, Event_Context ctx)
{
    if (event_type != EVENT_KEY_PRESSED) return false;

    u16 key = ctx.data.u16[0];
    switch (key) {
        case KEY_ESC:
            is_running = false;
            LOG_INFO("Exiting application\n");
            break;
        default:
            LOG_INFO("Key pressed: %c\n", key);
            break;
    }

    return true;
}

bool handle_key_released(int event_type, void *listener, Event_Context ctx)
{
    if (event_type != EVENT_KEY_RELEASED) return false;

    u16 key = ctx.data.u16[0];
    LOG_INFO("Key released: %c\n", key);

    return true;
}

int main(void)
{    
    LOG_INFO("Starting application\n");

    LOG_INFO("Initializing Vulkan\n");
    vulkan_init();
    
    LOG_INFO("Initializing input system\n");
    input_init();

    LOG_INFO("Initializing event list\n");
    event_init();
    event_register(EVENT_KEY_PRESSED, NULL, handle_key_pressed);
    event_register(EVENT_KEY_RELEASED, NULL, handle_key_released);

    LOG_INFO("Initializing window\n");
    Platform_Window window;
    platform_window_init(&window, "App window", 100, 100, 1280, 720);

    while (is_running) {
        platform_window_handle_message(&window);

        input_update(NULL);
    }
    
    // Clean up
    {
        platform_window_destroy(&window);

        event_unregister(EVENT_KEY_PRESSED, NULL, handle_key_pressed);
        event_unregister(EVENT_KEY_RELEASED, NULL, handle_key_released);
        event_destroy();

        input_destroy();

        vulkan_destroy();
    }
    return 0;
}
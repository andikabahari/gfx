#ifndef VULKAN_H
#define VULKAN_H

#include "platform.h"

void vulkan_init(Platform_Window *window, u32 width, u32 height);
void vulkan_destroy();

void vulkan_draw_frame();

void vulkan_wait_idle();

// TODO: this is temporary
#define VULKAN_CHECK(expr) assert((expr) == VK_SUCCESS)

#endif
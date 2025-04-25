#ifndef VULKAN_H
#define VULKAN_H

#include "platform.h"

void vulkan_init(Platform_Window *window);
void vulkan_destroy();

// TODO: this is temporary
#define VULKAN_CHECK(expr) assert((expr) == VK_SUCCESS)

#endif
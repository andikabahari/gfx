#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>

#include "common.h"

typedef struct {
    VkInstance                instance;
    VkAllocationCallbacks    *allocator;
    VkDebugUtilsMessengerEXT  debug_messenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physical_device;
} Vulkan_Context;

typedef struct {
    u32 graphics;
    u32 present;
} Vulkan_Queue_Family_Index;

#endif
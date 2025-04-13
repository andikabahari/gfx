#include "vulkan.h"

#include <vulkan/vulkan.h>

#include "types.h"
#include "log.h"

static VkInstance instance;
static VkAllocationCallbacks *allocator = NULL;

void vulkan_init()
{
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Example Vulkan application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    if (vkCreateInstance(&create_info, allocator, &instance) != VK_SUCCESS) {
        LOG_FATAL("Failed to create Vulkan instance\n");
    }
}

void vulkan_destroy()
{
    // 
}

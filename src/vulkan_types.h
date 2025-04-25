#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>

#include "common.h"

typedef struct {
    u32 graphics_queue_family_index;
    u32 present_queue_family_index;
    u32 compute_queue_family_index;
    u32 transfer_queue_family_index;
} Vulkan_Queue_Family_Support_Details;

typedef struct {
    VkSurfaceCapabilitiesKHR  capabilities;
    VkSurfaceFormatKHR       *formats;
    u32                       format_count;
    VkPresentModeKHR         *present_modes;
    u32                       present_mode_count;
} Vulkan_Swapchain_Support_Details;

typedef struct {
    VkInstance                instance;
    VkSurfaceKHR              surface;
    VkAllocationCallbacks    *allocator;
#ifdef DEBUG_MODE
    VkDebugUtilsMessengerEXT  debug_messenger;
#endif
    
    VkPhysicalDevice                    physical_device;
    VkDevice                            logical_device;
    Vulkan_Swapchain_Support_Details    swapchain_support;
    Vulkan_Queue_Family_Support_Details queue_family_support;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkSwapchainKHR  swapchain;
    VkImage        *swapchain_images;
    u32             swapchain_image_count;
    VkFormat        swapchain_image_format;
    VkExtent2D      swapchain_extent;

    u32 framebuffer_width;
    u32 framebuffer_height;
} Vulkan_Context;

#endif
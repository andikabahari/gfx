#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include <vulkan/vulkan.h>

#include "common.h"

typedef struct {
    u32 graphics_queue_family_index;
    u32 present_queue_family_index;
    u32 compute_queue_family_index;
    u32 transfer_queue_family_index;
} Vulkan_Queue_Family_Indices;

typedef struct {
    VkSurfaceCapabilitiesKHR  capabilities;
    u32                       format_count;
    VkSurfaceFormatKHR       *formats;
    u32                       present_mode_count;
    VkPresentModeKHR         *present_modes;
} Vulkan_Swapchain_Support_Details;

typedef struct {
    VkInstance                instance;
    VkSurfaceKHR              surface;
    VkAllocationCallbacks    *allocator;
#ifdef DEBUG_MODE
    VkDebugUtilsMessengerEXT  debug_messenger;
#endif
    
    VkPhysicalDevice                 physical_device;
    VkDevice                         logical_device;
    Vulkan_Swapchain_Support_Details swapchain_support;
    Vulkan_Queue_Family_Indices      supported_queue_families;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    VkSwapchainKHR  swapchain;
    u32             swapchain_image_count;
    VkImage        *swapchain_images;
    VkFormat        swapchain_image_format;
    VkImageView    *swapchain_image_views;
    VkExtent2D      swapchain_extent;
    VkFramebuffer  *swapchain_framebuffers;

    u32 framebuffer_width;
    u32 framebuffer_height;

    VkRenderPass     renderpass;
    VkPipelineLayout pipeline_layout;
    VkPipeline       graphics_pipeline;

    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;
} Vulkan_Context;

#endif
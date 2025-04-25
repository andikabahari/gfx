#include "vulkan.h"

#include "vulkan_types.h"

#include <assert.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "array.h"
#include "platform.h"
#include "memory.h"

static Vulkan_Context context = {0};

static const char **required_extension_names;

#ifdef DEBUG_MODE
static const char *validation_layer_names[] = {
    "VK_LAYER_KHRONOS_validation"
};
static const u32 validation_layer_name_count = 1;
#endif

static bool check_validation_layer_support()
{
#ifdef DEBUG_MODE
    u32 available_layer_count = 0;
    VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL));

    VkLayerProperties available_layers[available_layer_count];
    VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (u32 i = 0; i < validation_layer_name_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strcmp(validation_layer_names[i], available_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            LOG_WARNING("Validation layer is not available: %s\n", validation_layer_names[i]);
            return false;
        }
    }

    return true;
#else
    return true;
#endif
}

#ifdef DEBUG_MODE
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR("%s\n", callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARNING("%s\n", callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO("%s\n", callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_DEBUG("%s\n", callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}
#endif

static void setup_debug_messenger()
{
#ifdef DEBUG_MODE
    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debug_callback,
    };

    PFN_vkCreateDebugUtilsMessengerEXT callback =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
    VULKAN_CHECK(callback(context.instance, &create_info, context.allocator, &context.debug_messenger));
#endif
}

static void get_required_extenion_names(const char ***extension_names)
{
    array_push(*extension_names, &VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef DEBUG_MODE
    // Other things potentially will be added in the future...
    array_push(*extension_names, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#if PLATFORM_WINDOWS
    array_push(*extension_names, &"VK_KHR_win32_surface");
#elif PLATFORM_LINUX
    array_push(*extension_names, &"VK_KHR_xcb_surface");
#endif
}

static void create_instance()
{
    if (!check_validation_layer_support()) {
        LOG_FATAL("Validation layers requested, but not available\n");
    }

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Example Vulkan application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    required_extension_names = array_create(const char *);
    get_required_extenion_names(&required_extension_names);

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = array_length(required_extension_names),
        .ppEnabledExtensionNames = required_extension_names,
    };

    if (vkCreateInstance(&create_info, context.allocator, &context.instance) != VK_SUCCESS) {
        LOG_FATAL("Failed to create Vulkan instance\n");
    }
}

static void get_physical_device_queue_family_support(VkPhysicalDevice device, Vulkan_Queue_Family_Support_Details *details)
{
    details->graphics_queue_family_index = -1;
    details->present_queue_family_index = -1;
    details->compute_queue_family_index = -1;
    details->transfer_queue_family_index = -1;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 current_transfer_score = 0;
        VkQueueFlags flags = queue_families[i].queueFlags;

        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            details->graphics_queue_family_index = i;
            ++current_transfer_score;
        }

        VkBool32 present_support = VK_FALSE;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &present_support));
        if (present_support) {
            details->present_queue_family_index = i;
        }

        if (flags & VK_QUEUE_COMPUTE_BIT) {
            details->compute_queue_family_index = i;
            ++current_transfer_score;
        }

        if (flags & VK_QUEUE_TRANSFER_BIT) {
            if (current_transfer_score <= min_transfer_score) {
                min_transfer_score = current_transfer_score;
                details->transfer_queue_family_index = i;
            }
        }
    }
}

static bool check_physical_device_queue_family_support(Vulkan_Queue_Family_Support_Details details)
{
    return details.graphics_queue_family_index != -1 && details.present_queue_family_index != -1; 
}

static void get_physical_device_swapchain_support(VkPhysicalDevice device, Vulkan_Swapchain_Support_Details *details)
{
    VULKAN_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details->capabilities));

    VULKAN_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &details->format_count, NULL));
    if (details->format_count > 0) {
        if (!details->formats) {
            details->formats = memory_alloc(sizeof(VkSurfaceFormatKHR) * details->format_count, MEMORY_TAG_VULKAN);
        }
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &details->format_count, details->formats));
    }

    VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &details->present_mode_count, NULL));
    if (details->present_mode_count > 0) {
        if (!details->present_modes) {
            details->present_modes = memory_alloc(sizeof(VkPresentModeKHR) * details->present_mode_count, MEMORY_TAG_VULKAN);
        }
        VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &details->present_mode_count, details->present_modes));
    }
}

static void free_swapchain_support(Vulkan_Swapchain_Support_Details *details)
{
    if (details->formats) {
        memory_free(details->formats, sizeof(VkSurfaceFormatKHR) * details->format_count, MEMORY_TAG_VULKAN);
        details->formats = NULL;
        details->format_count = 0;
    }

    if (details->present_modes) {
        memory_free(details->present_modes, sizeof(VkPresentModeKHR) * details->present_mode_count, MEMORY_TAG_VULKAN);
        details->present_modes = NULL;
        details->present_mode_count = 0;
    }
}

static const char *physical_device_extension_names[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const u32 physical_device_extension_count = 1;

static bool check_physical_device_extension_support(VkPhysicalDevice device)
{
    u32 available_extension_count;
    VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, NULL));

    if (available_extension_count > 0) {
        VkExtensionProperties available_extensions[available_extension_count];
        VULKAN_CHECK(vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, available_extensions));

        for (u32 i = 0; i < physical_device_extension_count; ++i) {
            bool found = false;
            for (u32 j = 0; j < available_extension_count; ++j) {
                if (strcmp(physical_device_extension_names[i], available_extensions[i].extensionName)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                LOG_WARNING("Required extension not found: '%s'\n", physical_device_extension_names[i]);
                return false;
            }
        }
    }

    return true;
}

static u32 rate_physical_device_suitability(VkPhysicalDevice device)
{
    u32 score = 0;
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    score += properties.limits.maxImageDimension2D; // Maximum possible size of textures affects graphics quality

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    if (!features.geometryShader) return 0; // Application can't function without geometry shaders

    Vulkan_Queue_Family_Support_Details queue_family_support;
    get_physical_device_queue_family_support(device, &queue_family_support);
    if (!check_physical_device_queue_family_support(queue_family_support)) return 0;

    if (!check_physical_device_extension_support(device)) return 0;

    Vulkan_Swapchain_Support_Details swapchain_support = {0};
    get_physical_device_swapchain_support(device, &swapchain_support);
    bool swapchain_support_adequate =
        swapchain_support.format_count > 0 && swapchain_support.present_mode_count > 0;
    free_swapchain_support(&swapchain_support);
    if (!swapchain_support_adequate) return 0;

    return score;
}

static void pick_physical_device()
{
    u32 physical_device_count;
    VULKAN_CHECK(vkEnumeratePhysicalDevices(context.instance, &physical_device_count, NULL));
    if (physical_device_count == 0) {
        LOG_FATAL("Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VULKAN_CHECK(vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices));

    struct {
        u32  index;
        u32  score;
        bool found;
    } best_picked = {-1, 0, false};

    for (u32 i = 0; i < physical_device_count; ++i) {
        int score = rate_physical_device_suitability(physical_devices[i]);
        if (score > 0 && score > best_picked.score) {
            best_picked.index = i;
            best_picked.score = score;
            best_picked.found = true;
        }
    }

    if (!best_picked.found) {
        LOG_FATAL("Failed to find a suitable GPU\n");
    }

    context.physical_device = physical_devices[best_picked.index];
    get_physical_device_queue_family_support(context.physical_device, &context.queue_family_support);
    get_physical_device_swapchain_support(context.physical_device, &context.swapchain_support);
}

static void create_logical_device()
{
    bool shared_present_queue =
        context.queue_family_support.graphics_queue_family_index ==
            context.queue_family_support.present_queue_family_index;
    bool shared_transfer_queue =
        context.queue_family_support.graphics_queue_family_index ==
            context.queue_family_support.transfer_queue_family_index;
    
    u32 index_count = 1;
    if (!shared_present_queue) index_count++;
    if (!shared_transfer_queue) index_count++;

    u32 indices[index_count];
    u32 index = 0;
    indices[index++] = context.queue_family_support.graphics_queue_family_index;
    if (!shared_present_queue)
        indices[index++] = context.queue_family_support.present_queue_family_index;
    if (!shared_transfer_queue)
        indices[index++] = context.queue_family_support.transfer_queue_family_index;

    VkDeviceQueueCreateInfo queue_create_infos[index_count];
    for (u32 i = 0; i < index_count; ++i) {
        queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[i].queueFamilyIndex = indices[i];
        queue_create_infos[i].queueCount = 1;
        float queue_priority = 1.0f;
        queue_create_infos[i].pQueuePriorities = &queue_priority;
        queue_create_infos[i].flags = 0;
        queue_create_infos[i].pNext = 0;
    }

    // TODO: I don't know what it is (yet), I'll take a look on it later
    VkPhysicalDeviceFeatures device_features = {0};

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = index_count,
        .pQueueCreateInfos = queue_create_infos,
        .pEnabledFeatures = &device_features,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = physical_device_extension_names,

        // Deprecated and ignored
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,  
    };

    VULKAN_CHECK(vkCreateDevice(context.physical_device, &device_create_info, NULL, &context.logical_device));

    vkGetDeviceQueue(
        context.logical_device,
        context.queue_family_support.graphics_queue_family_index, 0, &context.graphics_queue);
    vkGetDeviceQueue(
        context.logical_device,
        context.queue_family_support.present_queue_family_index, 0, &context.present_queue);
    vkGetDeviceQueue(
        context.logical_device,
        context.queue_family_support.transfer_queue_family_index, 0, &context.transfer_queue);
}

static void create_swapchain()
{
    VkSurfaceFormatKHR surface_format;
    bool found = false;
    for (u32 i = 1; i < context.swapchain_support.format_count; ++i) {
        VkSurfaceFormatKHR available_format = context.swapchain_support.formats[i];
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = available_format;
            found = true;
            break;
        }
    }
    if (!found) surface_format = context.swapchain_support.formats[0];

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < context.swapchain_support.present_mode_count; ++i) {
        VkPresentModeKHR available_present_mode = context.swapchain_support.present_modes[i];
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = available_present_mode;
            break;
        }
    }

    VkExtent2D extent = {
        .width = context.framebuffer_width,
        .height = context.framebuffer_height};
    if (context.swapchain_support.capabilities.currentExtent.height != UINT32_MAX) {
        extent = context.swapchain_support.capabilities.currentExtent;
    } else {
        VkExtent2D min_extent = context.swapchain_support.capabilities.minImageExtent;
        VkExtent2D max_extent = context.swapchain_support.capabilities.maxImageExtent;
        extent.width = CLAMP(extent.width, min_extent.width, max_extent.width);
        extent.height = CLAMP(extent.height, min_extent.height, max_extent.height);
    }

    u32 image_count = context.swapchain_support.capabilities.minImageCount + 1;
    if (context.swapchain_support.capabilities.maxImageCount > 0 && image_count > context.swapchain_support.capabilities.maxImageCount) {
        image_count = context.swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = context.surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };
    
    if (context.queue_family_support.graphics_queue_family_index !=
        context.queue_family_support.present_queue_family_index) {
        u32 queue_family_indices[] = {
            context.queue_family_support.graphics_queue_family_index,
            context.queue_family_support.present_queue_family_index};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = 0;
    }

    create_info.preTransform = context.swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VULKAN_CHECK(vkCreateSwapchainKHR(context.logical_device, &create_info, context.allocator, &context.swapchain));

    context.swapchain_image_count = 0;
    VULKAN_CHECK(vkGetSwapchainImagesKHR(context.logical_device, context.swapchain, &context.swapchain_image_count, NULL));
    if (context.swapchain_images == NULL) {
        context.swapchain_images = (VkImage *)memory_alloc(sizeof(VkImage) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    }
    VULKAN_CHECK(vkGetSwapchainImagesKHR(context.logical_device, context.swapchain, &context.swapchain_image_count, context.swapchain_images));

    if (context.swapchain_image_views == NULL) {
        context.swapchain_image_views = (VkImageView *)memory_alloc(sizeof(VkImageView) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    }
    for (u32 i = 0; i < context.swapchain_image_count; ++i) {
        VkImageViewCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = context.swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = context.swapchain_image_format;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        VULKAN_CHECK(vkCreateImageView(context.logical_device, &create_info, context.allocator, &context.swapchain_image_views[i]));
    }

    context.swapchain_image_format = surface_format.format;

    context.swapchain_extent = extent;
}

void vulkan_init(Platform_Window *window)
{
    context.allocator = NULL;

    create_instance();
    setup_debug_messenger();
    platform_window_create_vulkan_surface(window, &context);
    pick_physical_device();
    create_logical_device();
    create_swapchain();
}

void vulkan_destroy()
{
    vkDestroySwapchainKHR(context.logical_device, context.swapchain, context.allocator);
    vkDestroyDevice(context.logical_device, context.allocator);

#ifdef DEBUG_MODE
    PFN_vkDestroyDebugUtilsMessengerEXT callback =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
    callback(context.instance, context.debug_messenger, context.allocator);
#endif

    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);

    free_swapchain_support(&context.swapchain_support);

    memory_free(context.swapchain_images, sizeof(VkImage) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    context.swapchain_image_count = 0;

    array_destroy(required_extension_names);
}

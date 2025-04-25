#include "vulkan.h"

#include "vulkan_types.h"

#include <assert.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "array.h"
#include "platform.h"
#include "memory.h"

static Vulkan_Context context;

#ifdef DEBUG_MODE
const char *validation_layer_names[] = {
    "VK_LAYER_KHRONOS_validation"
};
const u32 validation_layer_name_count = 1;
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

    const char **extension_names = array_create(const char *);
    get_required_extenion_names(&extension_names);

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = array_length(extension_names),
        .ppEnabledExtensionNames = extension_names,
    };

    if (vkCreateInstance(&create_info, context.allocator, &context.instance) != VK_SUCCESS) {
        LOG_FATAL("Failed to create Vulkan instance\n");
    }
}

void get_physical_device_queue_family_support(VkPhysicalDevice device, Vulkan_Queue_Family_Support_Details *details)
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

bool check_physical_device_queue_family_support(Vulkan_Queue_Family_Support_Details details)
{
    return details.graphics_queue_family_index != -1 && details.present_queue_family_index != -1; 
}

void get_physical_device_swapchain_support(VkPhysicalDevice device, Vulkan_Swapchain_Support_Details *details)
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

const char *physical_device_extension_names[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
u32 physical_device_extension_count = 1;

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

static int rate_physical_device_suitability(VkPhysicalDevice device)
{
    int score = 0;
    
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    score += properties.limits.maxImageDimension2D; // Maximum possible size of textures affects graphics quality

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    if (!features.geometryShader) return 0; // Application can't function without geometry shaders

    get_physical_device_queue_family_support(device, &context.queue_family_support);
    if (!check_physical_device_queue_family_support(context.queue_family_support)) return 0;

    if (!check_physical_device_extension_support(device)) return 0;

    get_physical_device_swapchain_support(device, &context.swapchain_support);
    bool swapchain_support_adequate =
        context.swapchain_support.format_count > 0 && context.swapchain_support.present_mode_count > 0;
    if (!swapchain_support_adequate) return 0;

    return score;
}

static void pick_physical_device()
{
    u32 physical_device_count = 0;
    VULKAN_CHECK(vkEnumeratePhysicalDevices(context.instance, &physical_device_count, NULL));
    if (physical_device_count == 0) {
        LOG_FATAL("Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VULKAN_CHECK(vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices));

    struct {
        u32  index;
        int  score;
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
}

static void create_logical_device()
{
    bool present_shares_graphics_queue =
        context.queue_family_support.graphics_queue_family_index ==
            context.queue_family_support.present_queue_family_index;
    bool transfer_shares_graphics_queue =
        context.queue_family_support.graphics_queue_family_index ==
            context.queue_family_support.transfer_queue_family_index;
    
    u32 index_count = 1;
    if (!present_shares_graphics_queue) index_count++;
    if (!transfer_shares_graphics_queue) index_count++;

    u32 indices[index_count];
    u32 index = 0;
    indices[index++] = context.queue_family_support.graphics_queue_family_index;
    if (!present_shares_graphics_queue)
        indices[index++] = context.queue_family_support.present_queue_family_index;
    if (!transfer_shares_graphics_queue)
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

void create_swapchain()
{
    // TODO: swapchain
}

void vulkan_init(Platform_Window *window)
{
    context.allocator = NULL;
    context.physical_device = NULL;

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

    memory_free(
        context.swapchain_support.formats, sizeof(*context.swapchain_support.formats),
        MEMORY_TAG_VULKAN);
    context.swapchain_support.format_count = 0;

    memory_free(
        context.swapchain_support.present_modes, sizeof(*context.swapchain_support.present_modes),
        MEMORY_TAG_VULKAN);
    context.swapchain_support.present_mode_count = 0;
}

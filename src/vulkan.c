#include "vulkan.h"

#include "vulkan_types.h"

#include <assert.h>
#include <string.h>

#include "common.h"
#include "log.h"
#include "array.h"
#include "platform.h"

#ifdef DEBUG_MODE
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif

// TODO: this is temporary
#define VK_CHECK(expr) assert((expr) == VK_SUCCESS)

static Vulkan_Context context;

static void check_validation_layer_support()
{
    if (!enable_validation_layers) return;

    const char **validation_layer_names = array_create(const char *);
    array_push(validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    u32 validation_layer_name_count = array_length(validation_layer_names);

    u32 available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, NULL));
    VkLayerProperties *available_layers = array_reserve(VkLayerProperties, available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers));

    for (u32 i = 0; i < validation_layer_name_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < available_layer_count; ++j) {
            if (strcmp(validation_layer_names[i], available_layers[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            LOG_FATAL("Validation layer is not available: %s\n", validation_layer_names[i]);
            return;
        }
    }
}

static void get_required_extenions(const char ***extension_names)
{
    array_push(*extension_names, &VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef DEBUG_MODE
    // Platform-specific extensions
#if PLATFORM_WINDOWS
    array_push(*extension_names, &"VK_KHR_win32_surface");
#elif PLATFORM_LINUX
    array_push(*extension_names, &"VK_KHR_xcb_surface");
#endif

    // Other things potentially will be added in the future...
    array_push(*extension_names, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
}

static void create_instance()
{
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Example Vulkan application",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    const char **extension_names = array_create(const char *);
    get_required_extenions(&extension_names);

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

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR(callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARNING(callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            LOG_INFO(callback_data->pMessage);
            break;

        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_DEBUG(callback_data->pMessage);
            break;
    }
    return VK_FALSE;
}

static void setup_debug_messenger()
{
    if (!enable_validation_layers) return;

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
    VK_CHECK(callback(context.instance, &create_info, context.allocator, &context.debug_messenger));
}

Vulkan_Queue_Family_Index find_queue_family(VkPhysicalDevice device)
{
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    Vulkan_Queue_Family_Index queue_family_index = {
        .graphics = -1,
        .present = -1,
    };

    for (u32 i = 0; i < queue_family_count; ++i) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_family_index.graphics = i;
        }

        VkBool32 present_support = VK_FALSE;
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &present_support));
        if (present_support) {
            queue_family_index.present = i;
        }
    }

    return queue_family_index;
}

bool valid_queue_family(Vulkan_Queue_Family_Index index)
{
    return index.graphics != -1 && index.present != -1; 
}

static int rate_device_suitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    int score = 0;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!features.geometryShader) return 0;

    return score;
}

static void pick_physical_device()
{
    u32 device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &device_count, NULL));
    if (device_count == 0) {
        LOG_FATAL("Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice physical_devices[device_count];
    VK_CHECK(vkEnumeratePhysicalDevices(context.instance, &device_count, physical_devices));

    struct {
        u32  index;
        int  score;
        bool found;
    } best_picked = {-1, 0, false};

    for (u32 i = 0; i < device_count; ++i) {
        Vulkan_Queue_Family_Index queue_family_index = find_queue_family(physical_devices[i]);
        if (!valid_queue_family(queue_family_index)) break;

        int score = rate_device_suitability(physical_devices[i]);
        if (score > best_picked.score) {
            best_picked.index = i;
            best_picked.score = score;
            best_picked.found = true;
        }
    }

    if (!best_picked.found) {
        LOG_FATAL("Failed to faind a suitable GPU\n");
    }

    context.physical_device = physical_devices[best_picked.index];
}

static void create_logical_device()
{
    Vulkan_Queue_Family_Index queue_family_index = find_queue_family(context.physical_device);
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_index.graphics,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    VkPhysicalDeviceFeatures physical_device_features = {0};

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_create_info,
        .queueCreateInfoCount = 1,
        .pEnabledFeatures = &physical_device_features,
        .enabledExtensionCount = 0,

        // Deprecated and ignored
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = 0,  
    };

    VK_CHECK(vkCreateDevice(context.physical_device, &device_create_info, NULL, &context.logical_device));

    vkGetDeviceQueue(context.logical_device, queue_family_index.graphics, 0, &context.graphics_queue);
    vkGetDeviceQueue(context.logical_device, queue_family_index.present, 0, &context.present_queue);
}

void vulkan_init(Platform_Window *window)
{
    context.allocator = NULL;
    context.physical_device = NULL;

    check_validation_layer_support();
    create_instance();
    setup_debug_messenger();
    platform_window_create_vulkan_surface(window, &context);
    pick_physical_device();
    create_logical_device();
}

// TODO: cleanup heap-allocated variables
void vulkan_destroy()
{
    if (enable_validation_layers) {
        PFN_vkDestroyDebugUtilsMessengerEXT callback =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkDestroyDebugUtilsMessengerEXT");
        callback(context.instance, context.debug_messenger, context.allocator);
    }

    vkDestroyInstance(context.instance, context.allocator);
}

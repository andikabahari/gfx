#include "vulkan.h"

#include <vulkan/vulkan.h>
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

static struct {
    VkInstance instance;
    VkAllocationCallbacks *allocator;
    VkDebugUtilsMessengerEXT debug_messenger;
} vk;

static void check_validation_layer_support()
{
    if (!enable_validation_layers) return;

    const char **validation_layer_names = array_create(const char *);
    array_push(validation_layer_names, &"VK_LAYER_KHRONOS_validation");
    u32 validation_layer_names_count = array_length(validation_layer_names);

    u32 available_layers_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layers_count, NULL));
    VkLayerProperties *available_layers = array_reserve(VkLayerProperties, available_layers_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layers_count, available_layers));

    for (u32 i = 0; i < validation_layer_names_count; ++i) {
        bool found = false;
        for (u32 j = 0; j < available_layers_count; ++j) {
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

    if (vkCreateInstance(&create_info, vk.allocator, &vk.instance) != VK_SUCCESS) {
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
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(callback(vk.instance, &create_info, vk.allocator, &vk.debug_messenger));
}

void vulkan_init()
{
    vk.allocator = NULL;

    check_validation_layer_support();
    create_instance();
    setup_debug_messenger();
}

void vulkan_destroy()
{
    if (enable_validation_layers) {
        PFN_vkDestroyDebugUtilsMessengerEXT callback =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkDestroyDebugUtilsMessengerEXT");
        callback(vk.instance, vk.debug_messenger, vk.allocator);
    }

    vkDestroyInstance(vk.instance, vk.allocator);
}

#include "vulkan.h"

#include "vulkan_types.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

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
    VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;

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

    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Example Vulkan application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    required_extension_names = array_create(const char *);
    get_required_extenion_names(&required_extension_names);

    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = array_length(required_extension_names);
    create_info.ppEnabledExtensionNames = required_extension_names;

    VULKAN_CHECK(vkCreateInstance(&create_info, context.allocator, &context.instance));
}

static void get_physical_device_queue_family_support(
    VkPhysicalDevice device, Vulkan_Queue_Family_Indices *supported_queue_families)
{
    supported_queue_families->graphics_queue_family_index = -1;
    supported_queue_families->present_queue_family_index = -1;
    supported_queue_families->compute_queue_family_index = -1;
    supported_queue_families->transfer_queue_family_index = -1;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

    VkQueueFamilyProperties queue_families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

    u8 min_transfer_score = 255;
    for (u32 i = 0; i < queue_family_count; ++i) {
        u8 current_transfer_score = 0;
        VkQueueFlags flags = queue_families[i].queueFlags;

        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            supported_queue_families->graphics_queue_family_index = i;
            ++current_transfer_score;
        }

        VkBool32 present_support = VK_FALSE;
        VULKAN_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &present_support));
        if (present_support) {
            supported_queue_families->present_queue_family_index = i;
        }

        if (flags & VK_QUEUE_COMPUTE_BIT) {
            supported_queue_families->compute_queue_family_index = i;
            ++current_transfer_score;
        }

        if (flags & VK_QUEUE_TRANSFER_BIT) {
            if (current_transfer_score <= min_transfer_score) {
                min_transfer_score = current_transfer_score;
                supported_queue_families->transfer_queue_family_index = i;
            }
        }
    }
}

static bool check_queue_family_support(Vulkan_Queue_Family_Indices supported_queue_families)
{
    return supported_queue_families.graphics_queue_family_index != -1
        && supported_queue_families.present_queue_family_index != -1
        && supported_queue_families.compute_queue_family_index != -1
        && supported_queue_families.transfer_queue_family_index != -1;
}

static void get_physical_device_swapchain_support(
    VkPhysicalDevice device, Vulkan_Swapchain_Support_Details *details)
{
    VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details->capabilities));

    VULKAN_CHECK(
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &details->format_count, NULL));
    if (details->format_count > 0) {
        if (!details->formats) {
            details->formats =
                memory_alloc(sizeof(VkSurfaceFormatKHR) * details->format_count, MEMORY_TAG_VULKAN);
        }
        VULKAN_CHECK(
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                device,
                context.surface,
                &details->format_count,
                details->formats));
    }

    VULKAN_CHECK(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            context.surface,
            &details->present_mode_count,
            NULL));
    if (details->present_mode_count > 0) {
        if (!details->present_modes) {
            details->present_modes =
                memory_alloc(sizeof(VkPresentModeKHR) * details->present_mode_count, MEMORY_TAG_VULKAN);
        }
        VULKAN_CHECK(
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                context.surface,
                &details->present_mode_count,
                details->present_modes));
    }
}

static void free_swapchain_support(Vulkan_Swapchain_Support_Details *details)
{
    if (details->formats) {
        memory_free(
            details->formats,
            sizeof(VkSurfaceFormatKHR) * details->format_count,
            MEMORY_TAG_VULKAN);
        details->formats = NULL;
        details->format_count = 0;
    }

    if (details->present_modes) {
        memory_free(
            details->present_modes,
            sizeof(VkPresentModeKHR) * details->present_mode_count,
            MEMORY_TAG_VULKAN);
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
    VULKAN_CHECK(
        vkEnumerateDeviceExtensionProperties(device, NULL, &available_extension_count, NULL));

    if (available_extension_count > 0) {
        VkExtensionProperties available_extensions[available_extension_count];
        VULKAN_CHECK(
            vkEnumerateDeviceExtensionProperties(
                device,
                NULL,
                &available_extension_count,
                available_extensions));

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

    Vulkan_Queue_Family_Indices supported_queue_families;
    get_physical_device_queue_family_support(device, &supported_queue_families);
    if (!check_queue_family_support(supported_queue_families)) return 0;

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
    VULKAN_CHECK(
        vkEnumeratePhysicalDevices(context.instance, &physical_device_count, NULL));
    if (physical_device_count == 0) {
        LOG_FATAL("Failed to find GPUs with Vulkan support\n");
    }

    VkPhysicalDevice physical_devices[physical_device_count];
    VULKAN_CHECK(
        vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices));

    struct {
        u32 index;
        u32 score;
    } best_picked = {-1, 0};

    for (u32 i = 0; i < physical_device_count; ++i) {
        u32 score = rate_physical_device_suitability(physical_devices[i]);
        if (score > best_picked.score) {
            best_picked.index = i;
            best_picked.score = score;
        }
    }

    if (best_picked.score == 0) {
        LOG_FATAL("Failed to find a suitable GPU\n");
    }

    context.physical_device = physical_devices[best_picked.index];
    get_physical_device_queue_family_support(context.physical_device, &context.supported_queue_families);
    get_physical_device_swapchain_support(context.physical_device, &context.swapchain_support);
}

static void create_logical_device()
{
    bool shared_present_queue =
        context.supported_queue_families.graphics_queue_family_index ==
            context.supported_queue_families.present_queue_family_index;
    bool shared_transfer_queue =
        context.supported_queue_families.graphics_queue_family_index ==
            context.supported_queue_families.transfer_queue_family_index;
    
    u32 index_count = 1;
    if (!shared_present_queue) index_count++;
    if (!shared_transfer_queue) index_count++;

    u32 indices[index_count];
    u32 index = 0;
    indices[index++] = context.supported_queue_families.graphics_queue_family_index;
    if (!shared_present_queue)
        indices[index++] = context.supported_queue_families.present_queue_family_index;
    if (!shared_transfer_queue)
        indices[index++] = context.supported_queue_families.transfer_queue_family_index;

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

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = index_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledExtensionCount = physical_device_extension_count;
    device_create_info.ppEnabledExtensionNames = physical_device_extension_names;

    // Deprecated and ignored
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;

    VULKAN_CHECK(
        vkCreateDevice(
            context.physical_device,
            &device_create_info,
            context.allocator,
            &context.logical_device));

    vkGetDeviceQueue(
        context.logical_device,
        context.supported_queue_families.graphics_queue_family_index,
        0,
        &context.graphics_queue);
    vkGetDeviceQueue(
        context.logical_device,
        context.supported_queue_families.present_queue_family_index,
        0,
        &context.present_queue);
    vkGetDeviceQueue(
        context.logical_device,
        context.supported_queue_families.transfer_queue_family_index,
        0,
        &context.transfer_queue);
}

static void create_swapchain()
{
    VkSurfaceFormatKHR surface_format;
    bool found = false;
    for (u32 i = 0; i < context.swapchain_support.format_count; ++i) {
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
        .height = context.framebuffer_height,
    };
    if (context.swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {
        extent = context.swapchain_support.capabilities.currentExtent;
    } else {
        VkExtent2D min_extent = context.swapchain_support.capabilities.minImageExtent;
        VkExtent2D max_extent = context.swapchain_support.capabilities.maxImageExtent;
        extent.width = CLAMP(extent.width, min_extent.width, max_extent.width);
        extent.height = CLAMP(extent.height, min_extent.height, max_extent.height);
    }

    u32 image_count = context.swapchain_support.capabilities.minImageCount + 1;
    if (context.swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > context.swapchain_support.capabilities.maxImageCount) {
        image_count = context.swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context.surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    if (context.supported_queue_families.graphics_queue_family_index !=
        context.supported_queue_families.present_queue_family_index) {
        u32 queue_family_indices[] = {
            context.supported_queue_families.graphics_queue_family_index,
            context.supported_queue_families.present_queue_family_index,
        };
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

    VULKAN_CHECK(
        vkCreateSwapchainKHR(
            context.logical_device,
            &create_info,
            context.allocator,
            &context.swapchain));

    VULKAN_CHECK(
        vkGetSwapchainImagesKHR(
            context.logical_device,
            context.swapchain,
            &context.swapchain_image_count,
            NULL));
    if (context.swapchain_images == NULL) {
        context.swapchain_images =
            (VkImage *)memory_alloc(sizeof(VkImage) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    }
    VULKAN_CHECK(
        vkGetSwapchainImagesKHR(
            context.logical_device,
            context.swapchain,
            &context.swapchain_image_count,
            context.swapchain_images));

    if (context.swapchain_image_views == NULL) {
        context.swapchain_image_views =
            (VkImageView *)memory_alloc(sizeof(VkImageView) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
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

        VULKAN_CHECK(
            vkCreateImageView(
                context.logical_device,
                &create_info,
                context.allocator,
                &context.swapchain_image_views[i]));
    }

    context.swapchain_image_format = surface_format.format;

    context.swapchain_extent = extent;
}

static void create_renderpass()
{
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = context.swapchain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    color_attachment.flags = 0;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_desc = {0};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency subpass_dependency = {0};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderpass_create_info = {0};
    renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.attachmentCount = 1;
    renderpass_create_info.pAttachments = &color_attachment;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass_desc;
    renderpass_create_info.dependencyCount = 1;
    renderpass_create_info.pDependencies = &subpass_dependency;

    VULKAN_CHECK(
        vkCreateRenderPass(
            context.logical_device,
            &renderpass_create_info,
            context.allocator,
            &context.renderpass));
}

static char *read_file(const char *filename, size_t *size)
{
    FILE *file = NULL;
    fopen_s(&file, filename, "rb+");
    if (!file) {
        LOG_FATAL("Failed to open file\n");
        return NULL;
    }

    // Move to the end of the file to get its size
    fseek(file, 0, SEEK_END);
    *size = ftell(file);  // get the file size
    fseek(file, 0, SEEK_SET);  // move back to the beginning of the file

    char *buffer = (char *)memory_alloc(*size * sizeof(char), MEMORY_TAG_STRING);
    if (!buffer) {
        fclose(file);
        LOG_FATAL("Failed to allocate memory\n");
        return NULL;
    }

    // Read the file content into the buffer
    fread(buffer, 1, *size, file);
    fclose(file);

    return buffer;
}

static void create_graphics_pipeline()
{
    size_t vert_shader_size;
    char *vert_shader_code = read_file("shaders/vert.spv", &vert_shader_size);
    VkShaderModuleCreateInfo vert_shader_module_create_info = {0};
    vert_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_shader_module_create_info.codeSize = vert_shader_size;
    vert_shader_module_create_info.pCode = (u32 *)vert_shader_code;
    VkShaderModule vert_shader_module;
    VULKAN_CHECK(
        vkCreateShaderModule(
            context.logical_device,
            &vert_shader_module_create_info,
            context.allocator,
            &vert_shader_module));

    size_t frag_shader_size;
    char *frag_shader_code = read_file("shaders/frag.spv", &frag_shader_size);
    VkShaderModuleCreateInfo frag_shader_module_create_info = {0};
    frag_shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_shader_module_create_info.codeSize = frag_shader_size;
    frag_shader_module_create_info.pCode = (u32 *)frag_shader_code;
    VkShaderModule frag_shader_module;
    VULKAN_CHECK(
        vkCreateShaderModule(
            context.logical_device,
            &frag_shader_module_create_info,
            context.allocator,
            &frag_shader_module));

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info,
        frag_shader_stage_info,
    };

    // NOTE: we hard code the vertex data directly in the vertex shader,
    // so we don't have to fill the struct for now.
    VkPipelineVertexInputStateCreateInfo vertext_input_create_info = {0};
    vertext_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertext_input_create_info.vertexBindingDescriptionCount = 0;
    vertext_input_create_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {0};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_create_info = {0};
    viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_create_info.viewportCount = 1;
    viewport_create_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {0};
    rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_create_info.depthClampEnable = VK_FALSE;
    rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_create_info.lineWidth = 1.0f;
    rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer_create_info.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_create_info = {0};
    multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_create_info.sampleShadingEnable = VK_FALSE;
    multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_create_info = {0};
    color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_create_info.logicOpEnable = VK_FALSE;
    color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_create_info.attachmentCount = 1;
    color_blend_create_info.pAttachments = &color_blend_attachment;
    color_blend_create_info.blendConstants[0] = 0.0f;
    color_blend_create_info.blendConstants[1] = 0.0f;
    color_blend_create_info.blendConstants[2] = 0.0f;
    color_blend_create_info.blendConstants[3] = 0.0f;
    
    u32 dynamic_state_count = 2;
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 0;
    pipeline_layout_create_info.pushConstantRangeCount = 0;

    VULKAN_CHECK(
        vkCreatePipelineLayout(
            context.logical_device,
            &pipeline_layout_create_info,
            context.allocator,
            &context.pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info = {0};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertext_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_info;
    pipeline_create_info.pViewportState = &viewport_create_info;
    pipeline_create_info.pRasterizationState = &rasterizer_create_info;
    pipeline_create_info.pMultisampleState = &multisampling_create_info;
    pipeline_create_info.pDepthStencilState = NULL; // optional
    pipeline_create_info.pColorBlendState = &color_blend_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = context.pipeline_layout;
    pipeline_create_info.renderPass = context.renderpass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE; // optional

    VULKAN_CHECK(
        vkCreateGraphicsPipelines(
            context.logical_device,
            VK_NULL_HANDLE,
            1,
            &pipeline_create_info,
            context.allocator,
            &context.graphics_pipeline));

    vkDestroyShaderModule(context.logical_device, frag_shader_module, context.allocator);
    memory_free(frag_shader_code, frag_shader_size, MEMORY_TAG_STRING);

    vkDestroyShaderModule(context.logical_device, vert_shader_module, context.allocator);
    memory_free(vert_shader_code, vert_shader_size, MEMORY_TAG_STRING);
}

static void create_framebuffers()
{
    // context.swapchain_framebuffers = array_reserve(VkFramebuffer, context.swapchain_image_count);
    
    context.swapchain_framebuffers =
        (VkFramebuffer *)memory_alloc(sizeof(VkFramebuffer) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    for (u32 i = 0; i < context.swapchain_image_count; ++i) {
        // TODO: we might need depth attachment later
        u32 attachment_count = 1;
        VkImageView attachments[] = {context.swapchain_image_views[i]};
        
        VkFramebufferCreateInfo create_info = {0};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = context.renderpass;
        create_info.attachmentCount = attachment_count;
        create_info.pAttachments = attachments;
        create_info.width = context.swapchain_extent.width;
        create_info.height = context.swapchain_extent.height;
        create_info.layers = 1;

        VULKAN_CHECK(
            vkCreateFramebuffer(
                context.logical_device,
                &create_info,
                context.allocator,
                &context.swapchain_framebuffers[i]));
    }
}

static void create_command_pool()
{
    VkCommandPoolCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = context.supported_queue_families.graphics_queue_family_index;

    VULKAN_CHECK(
        vkCreateCommandPool(
            context.logical_device,
            &create_info,
            context.allocator,
            &context.command_pool));
}

static void create_command_buffer()
{
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = context.command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VULKAN_CHECK(vkAllocateCommandBuffers(context.logical_device, &alloc_info, &context.command_buffer));
}

static void record_command_buffer(VkCommandBuffer command_buffer, u32 image_index)
{
    VkCommandBufferBeginInfo cmd_begin_info = {0};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin_info.flags = 0; // optional
    cmd_begin_info.pInheritanceInfo = NULL; // optional, only relevant for secondary command buffers
    VULKAN_CHECK(vkBeginCommandBuffer(context.command_buffer, &cmd_begin_info));

    VkRenderPassBeginInfo render_begin_info = {0};
    render_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_begin_info.renderPass = context.renderpass;
    render_begin_info.framebuffer = context.swapchain_framebuffers[image_index];
    render_begin_info.renderArea.offset.x = 0;
    render_begin_info.renderArea.offset.y = 0;
    render_begin_info.renderArea.extent.width = context.swapchain_extent.width;
    render_begin_info.renderArea.extent.height = context.swapchain_extent.height;
    render_begin_info.pNext = NULL;
    u32 clear_value_count = 1;
    VkClearValue clear_values[1] = {0};
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 1.0f;
    render_begin_info.clearValueCount = clear_value_count;
    render_begin_info.pClearValues = clear_values;

    // // TODO: fix this!
    // // Render pass does not work and it will produce "Integer division by zero error."
    // vkCmdBeginRenderPass(context.command_buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // // Render pass
    // {
    //     vkCmdBindPipeline(
    //         context.command_buffer,
    //         VK_PIPELINE_BIND_POINT_GRAPHICS,
    //         context.graphics_pipeline);
    
    //     VkViewport viewport = {0};
    //     viewport.x = 0.0f;
    //     viewport.y = 0.0f;
    //     viewport.width = (float)context.swapchain_extent.width;
    //     viewport.height = (float)context.swapchain_extent.height;
    //     viewport.minDepth = 0.0f;
    //     viewport.maxDepth = 1.0f;
    //     vkCmdSetViewport(context.command_buffer, 0, 1, &viewport);
    
    //     VkRect2D scissor = {0};
    //     scissor.offset.x = 0;
    //     scissor.offset.y = 0;
    //     scissor.extent = context.swapchain_extent;
    //     vkCmdSetScissor(context.command_buffer, 0, 1, &scissor);
    
    //     vkCmdDraw(context.command_buffer, 3, 1, 0, 0);
    // }

    // vkCmdEndRenderPass(context.command_buffer);

    VULKAN_CHECK(vkEndCommandBuffer(context.command_buffer));
}

static void create_sync_objects()
{
    VkSemaphoreCreateInfo semaphore_create_info = {0};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VULKAN_CHECK(
        vkCreateSemaphore(
            context.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.image_available_semaphore));

    VULKAN_CHECK(
        vkCreateSemaphore(
            context.logical_device,
            &semaphore_create_info,
            context.allocator,
            &context.render_finished_semaphore));

    VkFenceCreateInfo fence_create_info = {0};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VULKAN_CHECK(
        vkCreateFence(
            context.logical_device,
            &fence_create_info,
            context.allocator,
            &context.in_flight_fence));
}

void vulkan_init(Platform_Window *window, u32 width, u32 height)
{
    context.framebuffer_width = width;
    context.framebuffer_height = height;

    create_instance();
    setup_debug_messenger();
    platform_window_create_vulkan_surface(window, &context);
    pick_physical_device();
    create_logical_device();
    create_swapchain();
    create_renderpass();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_pool();
    create_command_buffer();
    create_sync_objects();
}

void vulkan_destroy()
{
    vkDestroySemaphore(context.logical_device, context.image_available_semaphore, context.allocator);
    vkDestroySemaphore(context.logical_device, context.render_finished_semaphore, context.allocator);
    vkDestroyFence(context.logical_device, context.in_flight_fence, context.allocator);

    vkDestroyCommandPool(context.logical_device, context.command_pool, context.allocator);

    for (u32 i = 0; i < context.swapchain_image_count; ++i) {
        vkDestroyFramebuffer(context.logical_device, context.swapchain_framebuffers[i], context.allocator);
    }

    vkDestroyPipeline(context.logical_device, context.graphics_pipeline, context.allocator);
    vkDestroyPipelineLayout(context.logical_device, context.pipeline_layout, context.allocator);

    vkDestroyRenderPass(context.logical_device, context.renderpass, context.allocator);

    for (u32 i = 0; i < context.swapchain_image_count; ++i) {
        vkDestroyImageView(context.logical_device, context.swapchain_image_views[i], context.allocator);
    }
    memory_free(
        context.swapchain_images, sizeof(VkImage) * context.swapchain_image_count, MEMORY_TAG_VULKAN);
    context.swapchain_image_count = 0;
    vkDestroySwapchainKHR(context.logical_device, context.swapchain, context.allocator);
    
    free_swapchain_support(&context.swapchain_support);
    vkDestroyDevice(context.logical_device, context.allocator);

#ifdef DEBUG_MODE
    PFN_vkDestroyDebugUtilsMessengerEXT callback =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            context.instance, "vkDestroyDebugUtilsMessengerEXT");
    callback(context.instance, context.debug_messenger, context.allocator);
#endif

    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);

    array_destroy(required_extension_names);
}

void vulkan_wait_idle()
{
    vkDeviceWaitIdle(context.logical_device);
}

void vulkan_draw_frame()
{
    vkWaitForFences(context.logical_device, 1, &context.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(context.logical_device, 1, &context.in_flight_fence);

    u32 image_index;
    VULKAN_CHECK(
        vkAcquireNextImageKHR(
            context.logical_device,
            context.swapchain,
            UINT64_MAX,
            context.image_available_semaphore,
            VK_NULL_HANDLE,
            &image_index));

    VULKAN_CHECK(vkResetCommandBuffer(context.command_buffer, 0));
    record_command_buffer(context.command_buffer, image_index);

    VkSemaphore wait_semaphores[] = {context.image_available_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signal_semaphores[] = {context.render_finished_semaphore};

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &context.command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    VULKAN_CHECK(vkQueueSubmit(context.graphics_queue, 1, &submit_info, context.in_flight_fence));

    VkSwapchainKHR swap_chains[] = {context.swapchain};

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = NULL; // optional
    VULKAN_CHECK(vkQueuePresentKHR(context.present_queue, &present_info));
}
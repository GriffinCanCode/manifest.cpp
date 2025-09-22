#include "VkCore.hpp"
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdlib>

// Include proper Vulkan headers
#include <vulkan/vulkan.hpp>

// Include GLFW for surface creation
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Manifest {
namespace Render {
namespace Vulkan {

// Import modern compatibility types  
using namespace Core::Modern;

bool Instance::create(const std::vector<const char*>& extensions,
                     const std::vector<const char*>& layers) {
    try {
        vk::ApplicationInfo app_info{};
        app_info.pApplicationName = "Manifest";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Manifest Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo create_info{};
        create_info.flags = vk::InstanceCreateFlags{MOLTENVK_INSTANCE_FLAGS}; // Required for MoltenVK
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
        create_info.ppEnabledLayerNames = layers.data();

        instance_ = vk::createInstanceUnique(create_info).value;
        
        if (!layers.empty()) {
            setup_debug_messenger();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create Vulkan instance: " << e.what() << std::endl;
        return false;
    }
}

bool Instance::setup_debug_messenger() {
    try {
        vk::DebugUtilsMessengerCreateInfoEXT create_info{};
        create_info.messageSeverity = 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        create_info.messageType = 
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        // Debug callback temporarily disabled due to C++/C API signature mismatch
        create_info.pfnUserCallback = nullptr;

        debug_messenger_ = instance_->createDebugUtilsMessengerEXTUnique(create_info).value;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to setup debug messenger: " << e.what() << std::endl;
        return false;
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Instance::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    (void)type; (void)user_data; // Suppress warnings
    
    const char* severity_str = "UNKNOWN";
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity_str = "ERROR";
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity_str = "WARNING";
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severity_str = "INFO";
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severity_str = "VERBOSE";
    }

    std::cerr << "[VULKAN " << severity_str << "] " << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

bool Surface::create(vk::Instance instance, void* window_handle) {
    try {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, static_cast<GLFWwindow*>(window_handle), 
                                   nullptr, &surface) != VK_SUCCESS) {
            return false;
        }
        surface_ = vk::UniqueSurfaceKHR{surface, instance};
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create surface: " << e.what() << std::endl;
        return false;
    }
}

bool PhysicalDeviceInfo::is_suitable(const DeviceRequirements& requirements) const {
    // Check queue families
    if (requirements.graphics_queue_required && !graphics_queue_family.has_value()) {
        return false;
    }
    if (requirements.present_queue_required && !present_queue_family.has_value()) {
        return false;
    }
    if (requirements.compute_queue_required && !compute_queue_family.has_value()) {
        return false;
    }
    if (requirements.transfer_queue_required && !transfer_queue_family.has_value()) {
        return false;
    }

    // Check required extensions
    std::set<std::string> available_extensions;
    for (const auto& ext : extensions) {
        available_extensions.insert(ext.extensionName);
    }

    for (const auto& required : requirements.required_extensions) {
        if (available_extensions.find(required) == available_extensions.end()) {
            return false;
        }
    }

    return true;
}

uint32_t PhysicalDeviceInfo::rate_device() const {
    uint32_t score = 0;

    // Discrete GPUs have significant performance advantage
    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += properties.limits.maxImageDimension2D;

    return score;
}

bool PhysicalDevice::select(vk::Instance instance, vk::SurfaceKHR surface,
                           const DeviceRequirements& requirements) {
    try {
        auto devices = instance.enumeratePhysicalDevices().value;
        if (devices.empty()) {
            std::cerr << "No Vulkan-capable devices found" << std::endl;
            return false;
        }

        std::vector<std::pair<uint32_t, PhysicalDeviceInfo>> candidates;

        for (const auto& device : devices) {
            PhysicalDeviceInfo info{};
            info.device = device;
            info.properties = device.getProperties();
            info.features = device.getFeatures();
            info.memory_properties = device.getMemoryProperties();
            info.queue_families = device.getQueueFamilyProperties();
            info.extensions = device.enumerateDeviceExtensionProperties().value;

            // Find queue families
            auto indices = find_queue_families(device, surface);
            info.graphics_queue_family = indices.graphics;
            info.present_queue_family = indices.present;
            info.compute_queue_family = indices.compute;
            info.transfer_queue_family = indices.transfer;

            if (info.is_suitable(requirements)) {
                candidates.emplace_back(info.rate_device(), std::move(info));
            }
        }

        if (candidates.empty()) {
            std::cerr << "No suitable devices found" << std::endl;
            return false;
        }

        // Sort by score (highest first)
        std::sort(candidates.begin(), candidates.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });

        device_ = candidates[0].second.device;
        info_ = std::move(candidates[0].second);

        std::cout << "Selected device: " << info_.properties.deviceName << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to select physical device: " << e.what() << std::endl;
        return false;
    }
}

SwapchainSupportDetails query_swapchain_support(vk::PhysicalDevice device, 
                                                vk::SurfaceKHR surface) {
    SwapchainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface).value;
    details.formats = device.getSurfaceFormatsKHR(surface).value;
    details.present_modes = device.getSurfacePresentModesKHR(surface).value;
    return details;
}

QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, 
                                       vk::SurfaceKHR surface) {
    QueueFamilyIndices indices;
    auto queue_families = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queue_families.size(); i++) {
        const auto& queue_family = queue_families[i];

        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics = i;
        }

        if (queue_family.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.compute = i;
        }

        if (queue_family.queueFlags & vk::QueueFlagBits::eTransfer) {
            indices.transfer = i;
        }

        if (device.getSurfaceSupportKHR(i, surface).value) {
            indices.present = i;
        }

        if (indices.is_complete()) {
            break;
        }
    }

    return indices;
}

}  // namespace Vulkan
}  // namespace Render 
}  // namespace Manifest

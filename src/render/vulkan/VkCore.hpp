#pragma once

#include "../../core/types/Modern.hpp"
#include "VkCompat.hpp"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace Manifest {
namespace Render {
namespace Vulkan {

// Import modern compatibility types
using namespace Core::Modern;

// RAII wrapper for VkInstance
class Instance {
    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debug_messenger_;

public:
    Instance() = default;
    
    bool create(const std::vector<const char*>& extensions = {},
                const std::vector<const char*>& layers = {});
    
    vk::Instance get() const { return instance_.get(); }
    operator bool() const { return static_cast<bool>(instance_); }
    
private:
    bool setup_debug_messenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* user_data);
};

// RAII wrapper for VkSurfaceKHR
class Surface {
    vk::UniqueSurfaceKHR surface_;

public:
    Surface() = default;
    
    bool create(vk::Instance instance, void* window_handle);
    
    vk::SurfaceKHR get() const { return surface_.get(); }
    operator bool() const { return static_cast<bool>(surface_); }
};

// Physical device selection criteria
struct DeviceRequirements {
    bool discrete_gpu_preferred{true};
    bool graphics_queue_required{true};
    bool present_queue_required{true};
    bool compute_queue_required{false};
    bool transfer_queue_required{false};
    std::vector<const char*> required_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char*> optional_extensions{};
};

// Physical device info
struct PhysicalDeviceInfo {
    vk::PhysicalDevice device;
    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceFeatures features;
    vk::PhysicalDeviceMemoryProperties memory_properties;
    std::vector<vk::QueueFamilyProperties> queue_families;
    std::vector<vk::ExtensionProperties> extensions;
    
    std::optional<uint32_t> graphics_queue_family;
    std::optional<uint32_t> present_queue_family;
    std::optional<uint32_t> compute_queue_family;
    std::optional<uint32_t> transfer_queue_family;
    
    bool is_suitable(const DeviceRequirements& requirements) const;
    uint32_t rate_device() const;
};

// Physical device selector
class PhysicalDevice {
    vk::PhysicalDevice device_;
    PhysicalDeviceInfo info_;

public:
    bool select(vk::Instance instance, vk::SurfaceKHR surface, 
                const DeviceRequirements& requirements = {});
    
    vk::PhysicalDevice get() const { return device_; }
    const PhysicalDeviceInfo& info() const { return info_; }
    operator bool() const { return static_cast<bool>(device_); }
};

// Queue family indices
struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
    
    bool is_complete() const {
        return graphics.has_value() && present.has_value();
    }
    
    std::vector<uint32_t> unique_indices() const {
        std::vector<uint32_t> indices;
        auto add_unique = [&](std::optional<uint32_t> idx) {
            if (idx && std::find(indices.begin(), indices.end(), *idx) == indices.end()) {
                indices.push_back(*idx);
            }
        };
        add_unique(graphics);
        add_unique(present);
        add_unique(compute);
        add_unique(transfer);
        return indices;
    }
};

// Swapchain support details
struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
    
    bool adequate() const {
        return !formats.empty() && !present_modes.empty();
    }
};

SwapchainSupportDetails query_swapchain_support(vk::PhysicalDevice device, 
                                                vk::SurfaceKHR surface);

QueueFamilyIndices find_queue_families(vk::PhysicalDevice device, 
                                       vk::SurfaceKHR surface);

}  // namespace Vulkan
}  // namespace Render
}  // namespace Manifest

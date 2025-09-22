#pragma once

// Vulkan compatibility header for C++23 and cross-platform support
// Provides necessary Vulkan definitions, extensions, and platform integrations

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <type_traits>
#include <string>

// All Vulkan types are now available through vulkan.hpp

// Include Vulkan C++ headers  
#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 0
#include <vulkan/vulkan.hpp>

namespace Manifest {
namespace Render {
namespace Vulkan {

// Platform-specific configuration for MoltenVK on macOS
#ifdef __APPLE__
    constexpr VkInstanceCreateFlags MOLTENVK_INSTANCE_FLAGS = 0x00000001U; // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
    constexpr bool MOLTENVK_AVAILABLE = true;
#else
    constexpr VkInstanceCreateFlags MOLTENVK_INSTANCE_FLAGS = 0;
    constexpr bool MOLTENVK_AVAILABLE = false;
#endif

// Debug and validation configuration
#ifdef VK_ENABLE_VALIDATION
    constexpr bool VALIDATION_ENABLED = true;
#else
    constexpr bool VALIDATION_ENABLED = false;
#endif

// Vulkan validation layer
constexpr const char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

// Required instance extensions (string literals for compatibility)
constexpr std::array<const char*, 4> REQUIRED_INSTANCE_EXTENSIONS = {
    "VK_KHR_surface",
#ifdef __APPLE__
    "VK_MVK_macos_surface",
    "VK_KHR_portability_enumeration",
#endif
#ifdef _WIN32
    "VK_KHR_win32_surface",
#endif
#ifdef __linux__
    "VK_KHR_xlib_surface",
#endif
    "VK_EXT_debug_utils"
};

// Required device extensions  
constexpr std::array<const char*, 2> REQUIRED_DEVICE_EXTENSIONS = {
    "VK_KHR_swapchain",
#ifdef __APPLE__
    "VK_KHR_portability_subset"
#endif
};

// Optional device extensions for enhanced features
constexpr std::array<const char*, 3> OPTIONAL_DEVICE_EXTENSIONS = {
    "VK_EXT_descriptor_indexing",
    "VK_KHR_dynamic_rendering",  
    "VK_EXT_memory_budget"
};

// Vulkan API version requirements
constexpr std::uint32_t MIN_VULKAN_API_VERSION = 4194306U;  // VK_API_VERSION_1_2 = VK_MAKE_API_VERSION(0, 1, 2, 0)
constexpr std::uint32_t PREFERRED_VULKAN_API_VERSION = 4194307U; // VK_API_VERSION_1_3 = VK_MAKE_API_VERSION(0, 1, 3, 0)

// Performance and memory configuration
constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr VkDeviceSize DEFAULT_STAGING_BUFFER_SIZE = 64 * 1024 * 1024; // 64MB
constexpr VkDeviceSize DEFAULT_UNIFORM_BUFFER_SIZE = 16 * 1024 * 1024;  // 16MB

// Vulkan object type constants are now available through vulkan.hpp

// Debug naming support helper with C++23 compatible syntax
template<typename T>
constexpr VkObjectType get_vulkan_object_type() {
    if constexpr (std::is_same<T, VkBuffer>::value) return VK_OBJECT_TYPE_BUFFER;
    else if constexpr (std::is_same<T, VkImage>::value) return VK_OBJECT_TYPE_IMAGE;
    else if constexpr (std::is_same<T, VkDeviceMemory>::value) return VK_OBJECT_TYPE_DEVICE_MEMORY;
    else if constexpr (std::is_same<T, VkPipeline>::value) return VK_OBJECT_TYPE_PIPELINE;
    else if constexpr (std::is_same<T, VkRenderPass>::value) return VK_OBJECT_TYPE_RENDER_PASS;
    else if constexpr (std::is_same<T, VkCommandBuffer>::value) return VK_OBJECT_TYPE_COMMAND_BUFFER;
    else if constexpr (std::is_same<T, VkShaderModule>::value) return VK_OBJECT_TYPE_SHADER_MODULE;
    else return VK_OBJECT_TYPE_UNKNOWN;
}

// C++23 compatible error handling
enum class VulkanError {
    Success,
    InitializationFailed,
    DeviceNotSuitable,
    ExtensionNotSupported,
    LayerNotSupported,
    OutOfMemory,
    DeviceLost,
    SurfaceCreationFailed
};

// Environment setup helper for MoltenVK
inline void setup_moltenvk_environment() {
#ifdef __APPLE__
    // Configure MoltenVK for optimal performance
    // These will be set in the implementation file where proper headers are available
    // setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
    // setenv("MVK_CONFIG_SHADER_CONVERSION_FLIP_VERTEX_Y", "1", 1);
    // setenv("MVK_CONFIG_TRACE_VULKAN_CALLS", "0", 1);
    // setenv("MVK_CONFIG_FORCE_LOW_POWER_GPU", "0", 1);
#endif
}

// Helper to get available validation layers
inline std::vector<std::string> get_available_validation_layers() {
    std::vector<std::string> available_layers;
    
    // This will be implemented in the source file where Vulkan headers are available
    // For now, return a basic set
    available_layers.emplace_back(VALIDATION_LAYER_NAME);
    
    return available_layers;
}

// Helper to check if validation layers are supported
inline bool validation_layers_supported() {
    auto available_layers = get_available_validation_layers();
    return std::find(available_layers.begin(), available_layers.end(), 
                     VALIDATION_LAYER_NAME) != available_layers.end();
}

} // namespace Vulkan
} // namespace Render
} // namespace Manifest

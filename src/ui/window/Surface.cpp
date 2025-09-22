#include "Surface.hpp"
#include "backends/GLFW.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Manifest {
namespace UI {
namespace Window {

namespace Integration {

SurfaceResult<void*> create_surface_for_renderer(
    const Window& window, API api, void* context) {
    
    switch (api) {
        case API::Vulkan: {
            auto result = window.create_vulkan_surface(context);
            if (!result) {
                return SurfaceError::CreationFailed;
            }
            return result.value();
        }
            
        case API::OpenGL: {
            auto result = window.make_opengl_context_current();
            if (!result) {
                return SurfaceError::CreationFailed;
            }
            return static_cast<void*>(const_cast<Window*>(&window));
        }
        
        default:
            return SurfaceError::BackendUnavailable;
    }
}

std::vector<const char*> get_required_vulkan_extensions() {
    std::vector<const char*> extensions;
    
    // Get GLFW required extensions
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    if (glfw_extensions) {
        for (uint32_t i = 0; i < glfw_extension_count; ++i) {
            extensions.push_back(glfw_extensions[i]);
        }
    }
    
    return extensions;
}

bool is_api_supported(const Window& window, API api) {
    switch (api) {
        case API::Vulkan: {
            // Try to create a dummy surface to test support
            auto surface_result = window.create_vulkan_surface(nullptr);
            return surface_result.has_value();
        }
        
        case API::OpenGL: {
            // Try to make context current
            auto result = window.make_opengl_context_current();
            return result.has_value();
        }
        
        default:
            return false;
    }
}

}  // namespace Integration

}  // namespace Window
}  // namespace UI
}  // namespace Manifest

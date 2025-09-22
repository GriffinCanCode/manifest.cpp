#include "common/Renderer.hpp"
#include "opengl/OpenGL.hpp"
#include "../ui/window/Surface.hpp"

// Only include Vulkan if available
#ifdef VULKAN_AVAILABLE
#include "vulkan/VulkanRenderer.hpp"
#endif

namespace Manifest::Render {

std::unique_ptr<Renderer> create_renderer(API api) {
    switch (api) {
        case API::OpenGL:
            return std::make_unique<OpenGL::Core>();
        case API::Vulkan:
#ifdef VULKAN_AVAILABLE
            return std::make_unique<Vulkan::VulkanRenderer>();
#else
            // Vulkan not available, fall back to OpenGL
            return std::make_unique<OpenGL::Core>();
#endif
        default:
            return nullptr;
    }
}

}  // namespace Manifest::Render

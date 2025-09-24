#pragma once

#include "Window.hpp"
#include "../../render/common/Types.hpp"

namespace Manifest {
namespace UI {
namespace Window {

using namespace Render;

// Surface creation errors
enum class SurfaceError : std::uint8_t {
    WindowInvalid,
    BackendUnavailable, 
    CreationFailed,
    ExtensionMissing
};

template <typename T>
using SurfaceResult = Core::Modern::Result<T, SurfaceError>;

// Abstract surface factory for renderer integration
class SurfaceFactory {
public:
    virtual ~SurfaceFactory() = default;

    // OpenGL context management only
    [[nodiscard]] virtual SurfaceResult<void> 
    make_opengl_current(const Window& window) const = 0;
    
    [[nodiscard]] virtual SurfaceResult<void> 
    release_opengl_current() const = 0;

    // Surface properties
    [[nodiscard]] virtual Vec2i get_framebuffer_size(const Window& window) const = 0;
    [[nodiscard]] virtual Vec2f get_content_scale(const Window& window) const = 0;
    [[nodiscard]] virtual bool supports_vulkan() const = 0;
    [[nodiscard]] virtual bool supports_opengl() const = 0;
};

// Renderer integration utilities
namespace Integration {

// Create surface for specific renderer API
[[nodiscard]] SurfaceResult<void*> create_surface_for_renderer(
    const Window& window, API api, void* context = nullptr);

// Get required Vulkan extensions for windowing
[[nodiscard]] std::vector<const char*> get_required_vulkan_extensions();

// Check if API is supported by current window backend
[[nodiscard]] bool is_api_supported(const Window& window, API api);

}  // namespace Integration

}  // namespace Window
}  // namespace UI
}  // namespace Manifest

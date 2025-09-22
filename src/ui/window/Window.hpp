#pragma once

#include "../../core/types/Modern.hpp"
#include "../../core/types/Types.hpp" 
#include "../../core/math/Vector.hpp"
#include <functional>
#include <string_view>
#include <cstdint>

namespace Manifest {
namespace UI {
namespace Window {

using namespace Core::Types;
using namespace Core::Math;
using namespace Core::Modern;

// Window error types
enum class WindowError : std::uint8_t {
    InitializationFailed,
    InvalidHandle,
    InvalidState,
    BackendUnavailable,
    InvalidSize,
    InvalidPosition,
    CreationFailed
};

template <typename T>
using Result = Core::Modern::Result<T, WindowError>;

// Strong typing for window handles
struct WindowTag {};
using WindowHandle = StrongId<WindowTag, std::uint32_t>;

// Window state and properties
enum class WindowState : std::uint8_t {
    Hidden,
    Visible, 
    Minimized,
    Maximized,
    Fullscreen,
    Closed
};

enum class CursorMode : std::uint8_t {
    Normal,
    Hidden,
    Disabled,
    Captured
};

struct WindowDesc {
    Vec2i size{1280, 720};
    Vec2i position{100, 100};
    std::string_view title{"Manifest"};
    bool resizable{true};
    bool decorated{true};
    bool transparent{false};
    bool always_on_top{false};
    WindowState initial_state{WindowState::Visible};
    std::string_view debug_name{};
};

struct WindowProperties {
    Vec2i size{};
    Vec2i framebuffer_size{};
    Vec2i position{};
    Vec2f content_scale{1.0f, 1.0f};
    WindowState state{};
    bool focused{};
    bool hovered{};
    bool should_close{};
};

// Pure abstract window interface for maximum testability
class Window {
public:
    virtual ~Window() = default;

    // Core lifecycle
    virtual Result<void> initialize(const WindowDesc& desc) = 0;
    virtual void shutdown() = 0;
    virtual void poll_events() = 0;
    virtual void swap_buffers() = 0;

    // State queries  
    [[nodiscard]] virtual const WindowProperties& properties() const = 0;
    [[nodiscard]] virtual bool should_close() const = 0;
    [[nodiscard]] virtual void* native_handle() const = 0;

    // State modifications
    virtual Result<void> set_title(std::string_view title) = 0;
    virtual Result<void> set_size(Vec2i size) = 0;
    virtual Result<void> set_position(Vec2i position) = 0;
    virtual Result<void> set_state(WindowState state) = 0;
    virtual Result<void> set_cursor_mode(CursorMode mode) = 0;
    virtual Result<void> close() = 0;

    // Surface creation for renderers
    virtual Result<void*> create_vulkan_surface(void* instance) const = 0;
    virtual Result<void> make_opengl_context_current() const = 0;
};

}  // namespace Window
}  // namespace UI  
}  // namespace Manifest

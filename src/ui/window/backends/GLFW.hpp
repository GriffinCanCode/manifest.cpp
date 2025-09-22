#pragma once

#include "../Window.hpp"
#include "../Events.hpp"
#include "../Factory.hpp"
#include "../Surface.hpp"

struct GLFWwindow;

namespace Manifest {
namespace UI {
namespace Window {
namespace Backends {

// GLFW window implementation
class GLFWWindow : public Window {
    GLFWwindow* window_{nullptr};
    WindowProperties properties_;
    std::unique_ptr<EventSystem> event_system_;
    bool initialized_{false};

public:
    explicit GLFWWindow(std::unique_ptr<EventSystem> event_system);
    ~GLFWWindow() override { shutdown(); }

    // Core lifecycle
    Result<void> initialize(const WindowDesc& desc) override;
    void shutdown() override;
    void poll_events() override;
    void swap_buffers() override;

    // State queries
    [[nodiscard]] const WindowProperties& properties() const override { return properties_; }
    [[nodiscard]] bool should_close() const override;
    [[nodiscard]] void* native_handle() const override { return window_; }

    // State modifications
    Result<void> set_title(std::string_view title) override;
    Result<void> set_size(Vec2i size) override;
    Result<void> set_position(Vec2i position) override;
    Result<void> set_state(WindowState state) override;
    Result<void> set_cursor_mode(CursorMode mode) override;
    Result<void> close() override;

    // Surface creation
    Result<void*> create_vulkan_surface(void* instance) const override;
    Result<void> make_opengl_context_current() const override;

private:
    void update_properties();
    void setup_callbacks();
    static Vec2i to_vec2i(int x, int y) { return Vec2i{x, y}; }
    static Vec2d to_vec2d(double x, double y) { return Vec2d{x, y}; }
};

// GLFW event system implementation
class GLFWEventSystem : public EventSystem {
    EventCallback callback_;

public:
    void set_callback(EventCallback callback) override { callback_ = std::move(callback); }
    void dispatch(const Event& event) override;
    void clear_callbacks() override { callback_ = nullptr; }
    
    // Internal dispatch methods used by GLFW callbacks
    void dispatch_window_close();
    void dispatch_window_resize(Vec2i size);
    void dispatch_window_move(Vec2i position);
    void dispatch_window_focus(bool focused);
    void dispatch_window_iconify(bool iconified);
    void dispatch_window_maximize(bool maximized);
    void dispatch_key(KeyCode key, std::uint32_t scancode, Action action, Modifier mods);
    void dispatch_char(std::uint32_t codepoint);
    void dispatch_mouse_button(MouseButton button, Action action, Modifier mods, Vec2d position);
    void dispatch_mouse_move(Vec2d position, Vec2d delta);
    void dispatch_scroll(Vec2d offset, Vec2d position);
    void dispatch_drop(Core::Modern::span<const char*> paths);
};

// GLFW surface factory
class GLFWSurfaceFactory : public SurfaceFactory {
public:
    [[nodiscard]] SurfaceResult<void*> 
    create_vulkan_surface(const Window& window, void* instance) const override;
    
    [[nodiscard]] SurfaceResult<void> 
    make_opengl_current(const Window& window) const override;
    
    [[nodiscard]] SurfaceResult<void> 
    release_opengl_current() const override;

    [[nodiscard]] Vec2i get_framebuffer_size(const Window& window) const override;
    [[nodiscard]] Vec2f get_content_scale(const Window& window) const override;
    [[nodiscard]] bool supports_vulkan() const override;
    [[nodiscard]] bool supports_opengl() const override;
};

// GLFW factory implementation
class GLFWFactory : public WindowFactory {
    bool initialized_{false};
    std::unique_ptr<GLFWSurfaceFactory> surface_factory_;

public:
    GLFWFactory();
    ~GLFWFactory() override { shutdown(); }
    
    [[nodiscard]] Result<std::unique_ptr<Window>> 
    create_window(const WindowDesc& desc) const override;
    
    [[nodiscard]] Result<std::unique_ptr<EventSystem>> 
    create_event_system() const override;
    
    [[nodiscard]] Backend backend() const override { return Backend::GLFW; }
    [[nodiscard]] bool is_available() const override;
    
    Result<void> initialize() override;
    void shutdown() override;

private:
    static void error_callback(int error, const char* description);
};

}  // namespace Backends
}  // namespace Window
}  // namespace UI
}  // namespace Manifest

#include "GLFW.hpp"
#include <iostream>
#include <unordered_map>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Manifest {
namespace UI {
namespace Window {
namespace Backends {

// GLFW initialization guard
namespace {
bool g_glfw_initialized = false;
int g_glfw_window_count = 0;
std::unordered_map<GLFWwindow*, GLFWEventSystem*> g_event_systems;

// Key code conversion helpers
KeyCode glfw_to_key_code(int key) {
    // Simplified mapping - in real implementation would need full mapping
    return static_cast<KeyCode>(key);
}

MouseButton glfw_to_mouse_button(int button) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        default: return MouseButton::Left;
    }
}

Action glfw_to_action(int action) {
    switch (action) {
        case GLFW_PRESS: return Action::Press;
        case GLFW_RELEASE: return Action::Release;
        case GLFW_REPEAT: return Action::Repeat;
        default: return Action::Press;
    }
}

Modifier glfw_to_mods(int mods) {
    Modifier result = Modifier::None;
    if (mods & GLFW_MOD_SHIFT) result = static_cast<Modifier>(static_cast<int>(result) | static_cast<int>(Modifier::Shift));
    if (mods & GLFW_MOD_CONTROL) result = static_cast<Modifier>(static_cast<int>(result) | static_cast<int>(Modifier::Ctrl));
    if (mods & GLFW_MOD_ALT) result = static_cast<Modifier>(static_cast<int>(result) | static_cast<int>(Modifier::Alt));
    if (mods & GLFW_MOD_SUPER) result = static_cast<Modifier>(static_cast<int>(result) | static_cast<int>(Modifier::Super));
    return result;
}
}

// GLFWWindow Implementation
GLFWWindow::GLFWWindow(std::unique_ptr<EventSystem> event_system)
    : event_system_(std::move(event_system)) {
}

Result<void> GLFWWindow::initialize(const WindowDesc& desc) {
    if (initialized_) {
        return WindowError::InvalidState;
    }

    // Window hints
    glfwWindowHint(GLFW_RESIZABLE, desc.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, desc.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, desc.transparent ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, desc.always_on_top ? GLFW_TRUE : GLFW_FALSE);
    
    // Default to OpenGL context for compatibility
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window_ = glfwCreateWindow(desc.size.x(), desc.size.y(), 
                              std::string(desc.title).c_str(), 
                              nullptr, nullptr);
    
    if (!window_) {
        return WindowError::CreationFailed;
    }

    // Set position
    glfwSetWindowPos(window_, desc.position.x(), desc.position.y());
    
    // Set initial visibility
    if (desc.initial_state == WindowState::Hidden) {
        glfwHideWindow(window_);
    }

    setup_callbacks();
    update_properties();
    
    initialized_ = true;
    g_glfw_window_count++;
    
    return {};
}

void GLFWWindow::shutdown() {
    if (!initialized_) return;
    
    if (window_) {
        g_event_systems.erase(window_);
        glfwDestroyWindow(window_);
        window_ = nullptr;
        g_glfw_window_count--;
    }
    
    initialized_ = false;
}

void GLFWWindow::poll_events() {
    // Events are polled globally by GLFW
}

void GLFWWindow::swap_buffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

bool GLFWWindow::should_close() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

Result<void> GLFWWindow::set_title(std::string_view title) {
    if (!window_) return WindowError::InvalidState;
    
    glfwSetWindowTitle(window_, std::string(title).c_str());
    return {};
}

Result<void> GLFWWindow::set_size(Vec2i size) {
    if (!window_) return WindowError::InvalidState;
    
    glfwSetWindowSize(window_, size.x(), size.y());
    update_properties();
    return {};
}

Result<void> GLFWWindow::set_position(Vec2i position) {
    if (!window_) return WindowError::InvalidState;
    
    glfwSetWindowPos(window_, position.x(), position.y());
    update_properties();
    return {};
}

Result<void> GLFWWindow::set_state(WindowState state) {
    if (!window_) return WindowError::InvalidState;
    
    switch (state) {
        case WindowState::Hidden:
            glfwHideWindow(window_);
            break;
        case WindowState::Visible:
            glfwShowWindow(window_);
            break;
        case WindowState::Minimized:
            glfwIconifyWindow(window_);
            break;
        case WindowState::Maximized:
            glfwMaximizeWindow(window_);
            break;
        case WindowState::Fullscreen: {
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            break;
        }
        case WindowState::Closed:
            return close();
    }
    
    update_properties();
    return {};
}

Result<void> GLFWWindow::set_cursor_mode(CursorMode mode) {
    if (!window_) return WindowError::InvalidState;
    
    int glfw_mode = GLFW_CURSOR_NORMAL;
    switch (mode) {
        case CursorMode::Normal: glfw_mode = GLFW_CURSOR_NORMAL; break;
        case CursorMode::Hidden: glfw_mode = GLFW_CURSOR_HIDDEN; break;
        case CursorMode::Disabled: glfw_mode = GLFW_CURSOR_DISABLED; break;
        case CursorMode::Captured: glfw_mode = GLFW_CURSOR_CAPTURED; break;
    }
    
    glfwSetInputMode(window_, GLFW_CURSOR, glfw_mode);
    return {};
}

Result<void> GLFWWindow::close() {
    if (!window_) return WindowError::InvalidState;
    
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
    return {};
}

Result<void*> GLFWWindow::create_vulkan_surface(void* instance) const {
    if (!window_) return WindowError::InvalidState;
    
#ifdef VULKAN_AVAILABLE
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(instance), 
                                             window_, nullptr, &surface);
    if (result == VK_SUCCESS) {
        return static_cast<void*>(surface);
    }
#endif
    
    return WindowError::CreationFailed;
}

Result<void> GLFWWindow::make_opengl_context_current() const {
    if (!window_) return WindowError::InvalidState;
    
    glfwMakeContextCurrent(window_);
    return {};
}

void GLFWWindow::update_properties() {
    if (!window_) return;
    
    int width, height;
    glfwGetWindowSize(window_, &width, &height);
    properties_.size = to_vec2i(width, height);
    
    glfwGetFramebufferSize(window_, &width, &height);
    properties_.framebuffer_size = to_vec2i(width, height);
    
    int x, y;
    glfwGetWindowPos(window_, &x, &y);
    properties_.position = to_vec2i(x, y);
    
    float xscale, yscale;
    glfwGetWindowContentScale(window_, &xscale, &yscale);
    properties_.content_scale = Vec2f{xscale, yscale};
    
    properties_.focused = glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE;
    properties_.hovered = glfwGetWindowAttrib(window_, GLFW_HOVERED) == GLFW_TRUE;
    properties_.should_close = glfwWindowShouldClose(window_) == GLFW_TRUE;
    
    // Determine window state
    if (glfwGetWindowAttrib(window_, GLFW_VISIBLE) == GLFW_FALSE) {
        properties_.state = WindowState::Hidden;
    } else if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) == GLFW_TRUE) {
        properties_.state = WindowState::Minimized;
    } else if (glfwGetWindowAttrib(window_, GLFW_MAXIMIZED) == GLFW_TRUE) {
        properties_.state = WindowState::Maximized;
    } else if (glfwGetWindowMonitor(window_) != nullptr) {
        properties_.state = WindowState::Fullscreen;
    } else {
        properties_.state = WindowState::Visible;
    }
}

void GLFWWindow::setup_callbacks() {
    if (!window_ || !event_system_) return;
    
    // Store event system reference for callbacks
    g_event_systems[window_] = static_cast<GLFWEventSystem*>(event_system_.get());
    
    glfwSetWindowUserPointer(window_, this);
    
    glfwSetWindowCloseCallback(window_, [](GLFWwindow* window) {
        if (auto* event_system = g_event_systems[window]) {
            event_system->dispatch_window_close();
        }
    });
    
    glfwSetWindowSizeCallback(window_, [](GLFWwindow* window, int width, int height) {
        if (auto* event_system = g_event_systems[window]) {
            event_system->dispatch_window_resize(Vec2i{width, height});
        }
    });
}

// GLFWEventSystem Implementation
void GLFWEventSystem::dispatch(const Event& event) {
    if (callback_) {
        callback_(event);
    }
}

void GLFWEventSystem::dispatch_window_close() {
    dispatch(WindowCloseEvent{});
}

void GLFWEventSystem::dispatch_window_resize(Vec2i size) {
    dispatch(WindowResizeEvent{size});
}

void GLFWEventSystem::dispatch_window_move(Vec2i position) {
    dispatch(WindowMoveEvent{position});
}

void GLFWEventSystem::dispatch_window_focus(bool focused) {
    dispatch(WindowFocusEvent{focused});
}

void GLFWEventSystem::dispatch_window_iconify(bool iconified) {
    dispatch(WindowIconifyEvent{iconified});
}

void GLFWEventSystem::dispatch_window_maximize(bool maximized) {
    dispatch(WindowMaximizeEvent{maximized});
}

void GLFWEventSystem::dispatch_key(KeyCode key, std::uint32_t scancode, Action action, Modifier mods) {
    dispatch(KeyEvent{key, scancode, action, mods});
}

void GLFWEventSystem::dispatch_char(std::uint32_t codepoint) {
    dispatch(CharEvent{codepoint});
}

void GLFWEventSystem::dispatch_mouse_button(MouseButton button, Action action, Modifier mods, Vec2d position) {
    dispatch(MouseButtonEvent{button, action, mods, position});
}

void GLFWEventSystem::dispatch_mouse_move(Vec2d position, Vec2d delta) {
    dispatch(MouseMoveEvent{position, delta});
}

void GLFWEventSystem::dispatch_scroll(Vec2d offset, Vec2d position) {
    dispatch(ScrollEvent{offset, position});
}

void GLFWEventSystem::dispatch_drop(Core::Modern::span<const char*> paths) {
    dispatch(DropEvent{paths});
}

// GLFWFactory Implementation
GLFWFactory::GLFWFactory() = default;

Result<void> GLFWFactory::initialize() {
    if (initialized_) return {};
    
    glfwSetErrorCallback(error_callback);
    
    if (!glfwInit()) {
        return WindowError::InitializationFailed;
    }
    
    g_glfw_initialized = true;
    initialized_ = true;
    surface_factory_ = std::make_unique<GLFWSurfaceFactory>();
    
    return {};
}

void GLFWFactory::shutdown() {
    if (!initialized_) return;
    
    surface_factory_.reset();
    
    if (g_glfw_initialized && g_glfw_window_count == 0) {
        glfwTerminate();
        g_glfw_initialized = false;
    }
    
    initialized_ = false;
}

bool GLFWFactory::is_available() const {
    return true;  // GLFW should always be available
}

Result<std::unique_ptr<Window>> GLFWFactory::create_window(const WindowDesc& desc) const {
    if (!initialized_) return WindowError::InvalidState;
    
    auto event_system_result = create_event_system();
    if (!event_system_result) {
        return event_system_result.error();
    }
    
    auto window = std::make_unique<GLFWWindow>(std::move(*event_system_result));
    auto init_result = window->initialize(desc);
    
    if (!init_result) {
        return init_result.error();
    }
    
    return std::unique_ptr<Window>(std::move(window));
}

Result<std::unique_ptr<EventSystem>> GLFWFactory::create_event_system() const {
    if (!initialized_) return WindowError::InvalidState;
    
    return std::unique_ptr<EventSystem>(std::make_unique<GLFWEventSystem>());
}

void GLFWFactory::error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// GLFWSurfaceFactory Implementation  
SurfaceResult<void*> GLFWSurfaceFactory::create_vulkan_surface(
    const Window& window, void* instance) const {
    auto result = window.create_vulkan_surface(instance);
    if (!result) {
        return SurfaceError::CreationFailed;
    }
    return result.value();
}

SurfaceResult<void> GLFWSurfaceFactory::make_opengl_current(const Window& window) const {
    auto result = window.make_opengl_context_current();
    if (!result) {
        return SurfaceError::CreationFailed;
    }
    return {};
}

SurfaceResult<void> GLFWSurfaceFactory::release_opengl_current() const {
    glfwMakeContextCurrent(nullptr);
    return {};
}

Vec2i GLFWSurfaceFactory::get_framebuffer_size(const Window& window) const {
    return window.properties().framebuffer_size;
}

Vec2f GLFWSurfaceFactory::get_content_scale(const Window& window) const {
    return window.properties().content_scale;
}

bool GLFWSurfaceFactory::supports_vulkan() const {
    return glfwVulkanSupported() == GLFW_TRUE;
}

bool GLFWSurfaceFactory::supports_opengl() const {
    return true;  // GLFW always supports OpenGL
}

}  // namespace Backends
}  // namespace Window
}  // namespace UI
}  // namespace Manifest

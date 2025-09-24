#include "Mock.hpp"
#include <algorithm>

namespace Manifest {
namespace UI {
namespace Window {
namespace Backends {

// MockWindow Implementation
Result<void> MockWindow::initialize(const WindowDesc& desc) {
    if (initialized_) {
        return WindowError::InvalidState;
    }
    
    record_call("initialize");
    
    desc_ = desc;
    properties_.size = desc.size;
    properties_.framebuffer_size = desc.size;
    properties_.position = desc.position;
    properties_.content_scale = Vec2f{1.0f, 1.0f};
    properties_.state = desc.initial_state;
    properties_.focused = (desc.initial_state == WindowState::Visible);
    properties_.hovered = false;
    properties_.should_close = false;
    
    title_ = std::string(desc.title);
    
    initialized_ = true;
    return {};
}

void MockWindow::shutdown() {
    if (!initialized_) return;
    
    record_call("shutdown");
    initialized_ = false;
    should_close_ = true;
}

void MockWindow::poll_events() {
    record_call("poll_events");
}

void MockWindow::swap_buffers() {
    record_call("swap_buffers");
}

Result<void> MockWindow::set_title(std::string_view title) {
    record_call("set_title");
    title_ = std::string(title);
    return {};
}

Result<void> MockWindow::set_size(Vec2i size) {
    record_call("set_size");
    properties_.size = size;
    properties_.framebuffer_size = size;
    return {};
}

Result<void> MockWindow::set_position(Vec2i position) {
    record_call("set_position");
    properties_.position = position;
    return {};
}

Result<void> MockWindow::set_state(WindowState state) {
    record_call("set_state");
    properties_.state = state;
    
    if (state == WindowState::Closed) {
        should_close_ = true;
    }
    
    return {};
}

Result<void> MockWindow::set_cursor_mode(CursorMode mode) {
    record_call("set_cursor_mode");
    cursor_mode_ = mode;
    return {};
}

Result<void> MockWindow::close() {
    record_call("close");
    should_close_ = true;
    properties_.should_close = true;
    return {};
}

// Vulkan surface creation removed - OpenGL only

Result<void> MockWindow::make_opengl_context_current() const {
    record_call("make_opengl_context_current");
    return {};
}

void MockWindow::simulate_resize(Vec2i size) {
    properties_.size = size;
    properties_.framebuffer_size = size;
}

void MockWindow::simulate_move(Vec2i position) {
    properties_.position = position;
}

void MockWindow::simulate_focus_change(bool focused) {
    properties_.focused = focused;
}

void MockWindow::record_call(std::string_view method) const {
    method_calls_.emplace_back(method);
}

// MockEventSystem Implementation
void MockEventSystem::dispatch(const Event& event) {
    if (callback_) {
        callback_(event);
    }
}

void MockEventSystem::process_queued_events() {
    while (!event_queue_.empty()) {
        dispatch(event_queue_.front());
        event_queue_.pop();
    }
}

// MockSurfaceFactory Implementation - Vulkan surface creation removed

SurfaceResult<void> MockSurfaceFactory::make_opengl_current(const Window& window) const {
    if (!opengl_supported_) {
        return SurfaceError::BackendUnavailable;
    }
    
    return {};
}

SurfaceResult<void> MockSurfaceFactory::release_opengl_current() const {
    return {};
}

Vec2i MockSurfaceFactory::get_framebuffer_size(const Window& window) const {
    return window.properties().framebuffer_size;
}

Vec2f MockSurfaceFactory::get_content_scale(const Window& window) const {
    return window.properties().content_scale;
}

// MockFactory Implementation  
Result<std::unique_ptr<Window>> MockFactory::create_window(const WindowDesc& desc) const {
    if (!initialized_ || !available_) {
        return WindowError::InitializationFailed;
    }
    
    auto window = std::make_unique<MockWindow>();
    auto init_result = window->initialize(desc);
    
    if (!init_result) {
        return init_result.error();
    }
    
    return std::unique_ptr<Window>(std::move(window));
}

Result<std::unique_ptr<EventSystem>> MockFactory::create_event_system() const {
    if (!initialized_ || !available_) {
        return WindowError::InitializationFailed;
    }
    
    return std::unique_ptr<EventSystem>(std::make_unique<MockEventSystem>());
}

Result<void> MockFactory::initialize() {
    if (initialized_) return {};
    
    initialized_ = true;
    return {};
}

void MockFactory::shutdown() {
    initialized_ = false;
}

}  // namespace Backends
}  // namespace Window
}  // namespace UI
}  // namespace Manifest

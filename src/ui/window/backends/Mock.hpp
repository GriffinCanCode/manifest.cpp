#pragma once

#include "../Window.hpp"
#include "../Events.hpp"
#include "../Factory.hpp"
#include "../Surface.hpp"
#include <vector>
#include <queue>

namespace Manifest {
namespace UI {
namespace Window {
namespace Backends {

// Mock implementations for testing
class MockWindow : public Window {
    WindowProperties properties_;
    WindowDesc desc_;
    bool initialized_{false};
    bool should_close_{false};
    std::string title_;
    CursorMode cursor_mode_{CursorMode::Normal};
    
    // Testing support
    mutable std::vector<std::string> method_calls_;
    
public:
    MockWindow() = default;
    ~MockWindow() override { shutdown(); }

    // Core lifecycle
    Result<void> initialize(const WindowDesc& desc) override;
    void shutdown() override;
    void poll_events() override;
    void swap_buffers() override;

    // State queries
    [[nodiscard]] const WindowProperties& properties() const override { return properties_; }
    [[nodiscard]] bool should_close() const override { return should_close_; }
    [[nodiscard]] void* native_handle() const override { return nullptr; }

    // State modifications
    Result<void> set_title(std::string_view title) override;
    Result<void> set_size(Vec2i size) override;
    Result<void> set_position(Vec2i position) override;
    Result<void> set_state(WindowState state) override;
    Result<void> set_cursor_mode(CursorMode mode) override;
    Result<void> close() override;

    // Surface creation (mock) - OpenGL only
    Result<void> make_opengl_context_current() const override;
    
    // Testing utilities
    [[nodiscard]] const std::vector<std::string>& method_calls() const { return method_calls_; }
    void clear_method_calls() { method_calls_.clear(); }
    void simulate_close() { should_close_ = true; }
    void simulate_resize(Vec2i size);
    void simulate_move(Vec2i position);
    void simulate_focus_change(bool focused);
    
private:
    void record_call(std::string_view method) const;
};

class MockEventSystem : public EventSystem {
    EventCallback callback_;
    std::queue<Event> event_queue_;
    
public:
    void set_callback(EventCallback callback) override { callback_ = std::move(callback); }
    void dispatch(const Event& event) override;
    void clear_callbacks() override { callback_ = nullptr; }
    
    // Testing utilities
    void queue_event(const Event& event) { event_queue_.push(event); }
    void process_queued_events();
    [[nodiscard]] std::size_t queued_event_count() const { return event_queue_.size(); }
    void clear_queue() { event_queue_ = {}; }
};

class MockSurfaceFactory : public SurfaceFactory {
    bool vulkan_supported_{false};  // Vulkan not supported anymore
    bool opengl_supported_{true};
    
public:
    [[nodiscard]] SurfaceResult<void> 
    make_opengl_current(const Window& window) const override;
    
    [[nodiscard]] SurfaceResult<void> 
    release_opengl_current() const override;

    [[nodiscard]] Vec2i get_framebuffer_size(const Window& window) const override;
    [[nodiscard]] Vec2f get_content_scale(const Window& window) const override;
    [[nodiscard]] bool supports_vulkan() const override { return vulkan_supported_; }
    [[nodiscard]] bool supports_opengl() const override { return opengl_supported_; }
    
    // Testing controls
    void set_vulkan_support(bool supported) { vulkan_supported_ = supported; }
    void set_opengl_support(bool supported) { opengl_supported_ = supported; }
};

class MockFactory : public WindowFactory {
    bool available_{true};
    bool initialized_{false};
    
public:
    MockFactory() = default;
    ~MockFactory() override { shutdown(); }
    
    [[nodiscard]] Result<std::unique_ptr<Window>> 
    create_window(const WindowDesc& desc) const override;
    
    [[nodiscard]] Result<std::unique_ptr<EventSystem>> 
    create_event_system() const override;
    
    [[nodiscard]] Backend backend() const override { return Backend::Mock; }
    [[nodiscard]] bool is_available() const override { return available_; }
    
    Result<void> initialize() override;
    void shutdown() override;
    
    // Testing controls
    void set_available(bool available) { available_ = available; }
};

}  // namespace Backends
}  // namespace Window
}  // namespace UI
}  // namespace Manifest

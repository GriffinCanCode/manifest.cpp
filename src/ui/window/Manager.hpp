#pragma once

#include "Window.hpp"
#include "Events.hpp"
#include "Factory.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace Manifest {
namespace UI {
namespace Window {

// Window manager for coordinating multiple windows
class Manager {
    std::unordered_map<WindowHandle, std::unique_ptr<Window>> windows_;
    std::unordered_map<WindowHandle, std::unique_ptr<EventSystem>> event_systems_;
    std::unique_ptr<WindowFactory> factory_;
    
    WindowHandle next_handle_{WindowHandle{1}};
    WindowHandle primary_window_{};
    
    using WindowCallback = std::function<void(WindowHandle, const Event&)>;
    WindowCallback global_callback_;

public:
    ~Manager() { shutdown(); }

    // Initialization
    Result<void> initialize(const FactoryConfig& config = {});
    void shutdown();

    // Window lifecycle
    [[nodiscard]] Result<WindowHandle> create_window(const WindowDesc& desc);
    Result<void> destroy_window(WindowHandle handle);
    void destroy_all_windows();

    // Window access
    [[nodiscard]] Window* get_window(WindowHandle handle) const;
    [[nodiscard]] WindowHandle primary_window() const { return primary_window_; }
    [[nodiscard]] std::vector<WindowHandle> all_windows() const;
    [[nodiscard]] bool has_window(WindowHandle handle) const;
    [[nodiscard]] std::size_t window_count() const { return windows_.size(); }

    // Event handling
    void set_global_event_callback(WindowCallback callback);
    void poll_all_events();
    bool should_quit() const;

    // Utility
    void swap_all_buffers();
    void set_primary_window(WindowHandle handle);
    
    // Singleton access (optional - can also be used as regular class)
    static Manager& instance();

private:
    WindowHandle generate_handle();
    void setup_window_events(WindowHandle handle);
};

}  // namespace Window
}  // namespace UI
}  // namespace Manifest

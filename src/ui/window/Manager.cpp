#include "Manager.hpp"
#include <algorithm>

namespace Manifest {
namespace UI {
namespace Window {

Result<void> Manager::initialize(const FactoryConfig& config) {
    if (factory_) {
        shutdown();
    }
    
    auto factory_result = create_window_factory(config);
    if (!factory_result) {
        return factory_result.error();
    }
    
    factory_ = std::move(*factory_result);
    return {};
}

void Manager::shutdown() {
    destroy_all_windows();
    
    if (factory_) {
        factory_->shutdown();
        factory_.reset();
    }
    
    next_handle_ = WindowHandle{1};
    primary_window_ = {};
    global_callback_ = nullptr;
}

Result<WindowHandle> Manager::create_window(const WindowDesc& desc) {
    if (!factory_) {
        return WindowError::InvalidState;
    }
    
    // Create window
    auto window_result = factory_->create_window(desc);
    if (!window_result) {
        return window_result.error();
    }
    
    // Create event system
    auto event_system_result = factory_->create_event_system();
    if (!event_system_result) {
        return WindowError::InitializationFailed;
    }
    
    auto handle = generate_handle();
    windows_[handle] = std::move(*window_result);
    event_systems_[handle] = std::move(*event_system_result);
    
    // Set first window as primary
    if (windows_.size() == 1) {
        primary_window_ = handle;
    }
    
    // Setup event forwarding
    setup_window_events(handle);
    
    return handle;
}

Result<void> Manager::destroy_window(WindowHandle handle) {
    auto window_it = windows_.find(handle);
    auto event_it = event_systems_.find(handle);
    
    if (window_it == windows_.end() || event_it == event_systems_.end()) {
        return WindowError::InvalidHandle;
    }
    
    // Clean shutdown
    window_it->second->shutdown();
    event_it->second->clear_callbacks();
    
    windows_.erase(window_it);
    event_systems_.erase(event_it);
    
    // Update primary window if necessary
    if (primary_window_ == handle) {
        primary_window_ = windows_.empty() ? WindowHandle{} : windows_.begin()->first;
    }
    
    return {};
}

void Manager::destroy_all_windows() {
    // Shutdown all windows
    for (auto& [handle, window] : windows_) {
        window->shutdown();
    }
    
    // Clear all event systems
    for (auto& [handle, event_system] : event_systems_) {
        event_system->clear_callbacks();
    }
    
    windows_.clear();
    event_systems_.clear();
    primary_window_ = {};
}

Window* Manager::get_window(WindowHandle handle) const {
    auto it = windows_.find(handle);
    return it != windows_.end() ? it->second.get() : nullptr;
}

std::vector<WindowHandle> Manager::all_windows() const {
    std::vector<WindowHandle> handles;
    handles.reserve(windows_.size());
    
    for (const auto& [handle, window] : windows_) {
        handles.push_back(handle);
    }
    
    return handles;
}

bool Manager::has_window(WindowHandle handle) const {
    return windows_.find(handle) != windows_.end();
}

void Manager::set_global_event_callback(WindowCallback callback) {
    global_callback_ = std::move(callback);
}

void Manager::poll_all_events() {
    for (auto& [handle, window] : windows_) {
        window->poll_events();
    }
}

bool Manager::should_quit() const {
    return std::all_of(windows_.begin(), windows_.end(),
                       [](const auto& pair) { return pair.second->should_close(); });
}

void Manager::swap_all_buffers() {
    for (auto& [handle, window] : windows_) {
        window->swap_buffers();
    }
}

void Manager::set_primary_window(WindowHandle handle) {
    if (has_window(handle)) {
        primary_window_ = handle;
    }
}

Manager& Manager::instance() {
    static Manager manager;
    return manager;
}

WindowHandle Manager::generate_handle() {
    auto handle = next_handle_;
    next_handle_ = WindowHandle{next_handle_.value() + 1};
    return handle;
}

void Manager::setup_window_events(WindowHandle handle) {
    auto event_it = event_systems_.find(handle);
    if (event_it == event_systems_.end() || !global_callback_) {
        return;
    }
    
    event_it->second->set_callback([this, handle](const Event& event) {
        if (global_callback_) {
            global_callback_(handle, event);
        }
    });
}

}  // namespace Window
}  // namespace UI
}  // namespace Manifest

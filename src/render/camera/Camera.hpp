#pragma once

// Complete camera control system integration
// This header provides a unified interface to all camera functionality

#include "Controls.hpp"
#include "Behavior.hpp"
#include "State.hpp" 
#include "Cursor.hpp"
#include "Selection.hpp"
#include "../../core/graphics/Camera.hpp"

namespace Manifest::Render::CameraSystem {

// Complete camera system that integrates all components
class System {
    Core::Graphics::Camera camera_;
    Controls controls_;
    Behavior::Controller behavior_controller_;
    State::Manager state_manager_;
    Cursor::Manager cursor_manager_;
    
    // Entity selection management (using int as placeholder)
    Selection::Manager<int> selection_manager_;
    
    bool active_{true};
    
public:
    explicit System(std::unique_ptr<Cursor::IProvider> cursor_provider = nullptr);
    
    // Core camera access
    Core::Graphics::Camera& camera() noexcept { return camera_; }
    const Core::Graphics::Camera& camera() const noexcept { return camera_; }
    
    // Component access
    Controls& controls() noexcept { return controls_; }
    const Controls& controls() const noexcept { return controls_; }
    
    Behavior::Controller& behavior() noexcept { return behavior_controller_; }
    const Behavior::Controller& behavior() const noexcept { return behavior_controller_; }
    
    State::Manager& state() noexcept { return state_manager_; }
    const State::Manager& state() const noexcept { return state_manager_; }
    
    Cursor::Manager& cursor() noexcept { return cursor_manager_; }
    const Cursor::Manager& cursor() const noexcept { return cursor_manager_; }
    
    Selection::Manager<int>& selection() noexcept { return selection_manager_; }
    const Selection::Manager<int>& selection() const noexcept { return selection_manager_; }
    
    // Unified event handling
    Core::Modern::Result<void, std::string> handle_event(const UI::Window::Event& event) noexcept;
    
    // Update system
    void update(float delta_time) noexcept;
    
    // System control
    void set_active(bool active) noexcept { active_ = active; }
    bool is_active() const noexcept { return active_; }
    
    // Quick setup methods
    void setup_orbital_controls() noexcept;
    void setup_free_controls() noexcept;
    void setup_cinematic_controls() noexcept;
    
    // Integration helpers
    void sync_cursor_context() noexcept;
    void save_current_state(const std::string& name) noexcept;
    void restore_state(const std::string& name) noexcept;
};

// Factory functions for common configurations
namespace Factory {
    System create_rts_camera() noexcept;      // RTS-style orbital camera
    System create_fps_camera() noexcept;      // First-person free camera
    System create_city_camera() noexcept;     // City management camera
    System create_world_camera() noexcept;    // World overview camera
}

} // namespace Manifest::Render::CameraSystem

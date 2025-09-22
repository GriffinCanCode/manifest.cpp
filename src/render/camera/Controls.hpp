#pragma once

#include "../../core/types/Types.hpp"
#include "../../core/types/Modern.hpp"
#include "../../ui/window/Events.hpp"
#include "../../core/math/Vector.hpp"
#include "../../core/graphics/Camera.hpp"
#include <chrono>
#include <functional>
#include <unordered_map>

namespace Manifest::Render::CameraSystem {

using Core::Types::Quantity;
using Core::Modern::Result;
using Core::Math::Vec2f;
using Core::Math::Vec2i;
using UI::Window::Event;
using UI::Window::KeyEvent;
using UI::Window::MouseButtonEvent;
using UI::Window::MouseMoveEvent;
using UI::Window::ScrollEvent;
using UI::Window::WindowResizeEvent;
using UI::Window::KeyCode;
using UI::Window::MouseButton;
using UI::Window::Modifier;
// using UI::Window::Action;  // Avoided due to name collision

// Strong typing for control parameters
struct SpeedTag {};
struct SensitivityTag {};
struct ZoomTag {};
struct TimeTag {};

using Speed = Quantity<SpeedTag, float>;
using Sensitivity = Quantity<SensitivityTag, float>;
using ZoomFactor = Quantity<ZoomTag, float>;
using DeltaTime = Quantity<TimeTag, float>;

// Control states
enum class ControlMode : std::uint8_t {
    Orbital,    // Camera orbits around target (RTS-style)
    Free,       // Free-look camera (FPS-style)
    Cinematic,  // Automated camera movement
    Fixed       // No user control
};

// Input bindings
struct Binding {
    KeyCode key{KeyCode::Unknown};
    MouseButton button{MouseButton::Left};
    Modifier required_mods{Modifier::None};
    bool is_key{true};
    
    constexpr bool matches(const KeyEvent& event) const noexcept {
        return is_key && key == event.key && required_mods == event.mods;
    }
    
    constexpr bool matches(const MouseButtonEvent& event) const noexcept {
        return !is_key && button == event.button && required_mods == event.mods;
    }
};

// Control actions
enum class ControlAction : std::uint8_t {
    MoveForward, MoveBack, MoveLeft, MoveRight, MoveUp, MoveDown,
    LookLeft, LookRight, LookUp, LookDown,
    ZoomIn, ZoomOut,
    ResetView, ToggleMode
};

// Configuration for camera controls
struct Config {
    Speed move_speed{50.0f};
    Speed zoom_speed{2.0f};
    Sensitivity mouse_sensitivity{0.005f};
    Sensitivity scroll_sensitivity{0.1f};
    ZoomFactor min_zoom{0.1f};
    ZoomFactor max_zoom{1000.0f};
    bool edge_scrolling{true};
    bool invert_y{false};
    float edge_scroll_margin{10.0f};
    std::unordered_map<ControlAction, Binding> bindings;
    
    Config() {
        setup_default_bindings();
    }
    
private:
    void setup_default_bindings();
};

// Control state tracking
struct State {
    ControlMode mode{ControlMode::Orbital};
    Vec2f last_mouse_pos{0.0f, 0.0f};
    Vec2i viewport_size{1920, 1080};
    bool is_dragging{false};
    bool keys_pressed[256]{false};
    std::chrono::high_resolution_clock::time_point last_update{};
    
    DeltaTime compute_delta_time() noexcept {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<float>(now - last_update).count();
        last_update = now;
        return DeltaTime{delta};
    }
};

// Main controls interface
class Controls {
    Config config_;
    State state_;
    std::function<void(Vec2f)> on_edge_scroll_;
    
public:
    explicit Controls(Config config = {}) noexcept;
    
    // Event handling
    Result<void, std::string> handle_event(const Event& event, Core::Graphics::Camera& camera) noexcept;
    void update(Core::Graphics::Camera& camera) noexcept;
    
    // Configuration
    const Config& config() const noexcept { return config_; }
    void set_config(Config config) noexcept { config_ = std::move(config); }
    
    // State queries
    ControlMode mode() const noexcept { return state_.mode; }
    void set_mode(ControlMode mode) noexcept { state_.mode = mode; }
    
    Vec2i viewport_size() const noexcept { return state_.viewport_size; }
    void set_viewport_size(Vec2i size) noexcept { state_.viewport_size = size; }
    
    // Callbacks
    void set_edge_scroll_callback(std::function<void(Vec2f)> callback) noexcept {
        on_edge_scroll_ = std::move(callback);
    }
    
private:
    Result<void, std::string> handle_key_event(const KeyEvent& event, Core::Graphics::Camera& camera) noexcept;
    Result<void, std::string> handle_mouse_button_event(const MouseButtonEvent& event, Core::Graphics::Camera& camera) noexcept;
    Result<void, std::string> handle_mouse_move_event(const MouseMoveEvent& event, Core::Graphics::Camera& camera) noexcept;
    Result<void, std::string> handle_scroll_event(const ScrollEvent& event, Core::Graphics::Camera& camera) noexcept;
    Result<void, std::string> handle_window_resize_event(const WindowResizeEvent& event) noexcept;
    
    void process_continuous_input(Core::Graphics::Camera& camera, DeltaTime dt) noexcept;
    void check_edge_scrolling(Vec2f mouse_pos) noexcept;
    bool is_action_active(ControlAction action) const noexcept;
    
    Vec2f apply_movement_constraints(const Vec2f& delta) const noexcept;
    float apply_zoom_constraints(float zoom_delta) const noexcept;
};

// Factory for creating pre-configured controls
namespace Factory {
    Controls orbital() noexcept;     // RTS-style orbital camera
    Controls free() noexcept;        // FPS-style free camera  
    Controls cinematic() noexcept;   // Automated cinematic camera
}

} // namespace Manifest::Render::CameraSystem

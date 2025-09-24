#include "Controls.hpp"
#include <algorithm>
#include <cmath>

namespace Manifest::Render::CameraSystem {

void Controls::Config::setup_default_bindings() {
    // Movement keys (WASD)
    bindings[ControlAction::MoveForward] = {KeyCode::W, {}, Modifier::None, true};
    bindings[ControlAction::MoveBack] = {KeyCode::S, {}, Modifier::None, true};
    bindings[ControlAction::MoveLeft] = {KeyCode::A, {}, Modifier::None, true};
    bindings[ControlAction::MoveRight] = {KeyCode::D, {}, Modifier::None, true};
    bindings[ControlAction::MoveUp] = {KeyCode::Q, {}, Modifier::None, true};
    bindings[ControlAction::MoveDown] = {KeyCode::E, {}, Modifier::None, true};
    
    // Mouse controls
    bindings[ControlAction::ResetView] = {{}, MouseButton::Middle, Modifier::None, false};
    bindings[ControlAction::ToggleMode] = {KeyCode::Tab, {}, Modifier::None, true};
}

Controls::Controls::Controls(Config config) noexcept 
    : config_(std::move(config))
    , state_{}
{
    state_.last_update = std::chrono::high_resolution_clock::now();
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_event(const Event& event, Core::Graphics::Camera& camera) noexcept {
    // Simple event handling - for now just return success
    // TODO: Implement proper event handling
    return Core::Modern::Result<void, std::string>{};
}

void Controls::Controls::update(Core::Graphics::Camera& camera) noexcept {
    if (state_.mode == ControlMode::Fixed) return;
    
    auto dt = state_.compute_delta_time();
    // TODO: Process continuous input
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_key_event(const UI::Window::KeyEvent& event, Core::Graphics::Camera& camera) noexcept {
    // Track key state for continuous input
    if (static_cast<std::uint16_t>(event.key) < 256) {
        state_.keys_pressed[static_cast<std::uint16_t>(event.key)] = 
            (event.action == UI::Window::Action::Press || event.action == UI::Window::Action::Repeat);
    }
    
    // Handle discrete actions
    if (event.action == UI::Window::Action::Press) {
        for (const auto& [action, binding] : config_.bindings) {
            if (binding.matches(event)) {
                switch (action) {
                    case ControlAction::ResetView:
                        camera.look_at(Core::Math::Vec3f{10.0f, 10.0f, 10.0f}, Core::Math::Vec3f{0.0f, 0.0f, 0.0f});
                        break;
                    case ControlAction::ToggleMode:
                        state_.mode = (state_.mode == ControlMode::Orbital) 
                            ? ControlMode::Free : ControlMode::Orbital;
                        break;
                    default:
                        break; // Continuous actions handled in process_continuous_input
                }
            }
        }
    }
    
    return {};
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_mouse_button_event(const UI::Window::MouseButtonEvent& event, Core::Graphics::Camera& camera) noexcept {
    state_.is_dragging = (event.button == UI::Window::MouseButton::Left && event.action == UI::Window::Action::Press);
    
    // Check for bound actions
    for (const auto& [action, binding] : config_.bindings) {
        if (binding.matches(event) && event.action == UI::Window::Action::Press) {
            switch (action) {
                case ControlAction::ResetView:
                    camera.look_at(Core::Math::Vec3f{10.0f, 10.0f, 10.0f}, Core::Math::Vec3f{0.0f, 0.0f, 0.0f});
                    break;
                default:
                    break;
            }
        }
    }
    
    return {};
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_mouse_move_event(const UI::Window::MouseMoveEvent& event, Core::Graphics::Camera& camera) noexcept {
    Core::Math::Vec2f current_pos{static_cast<float>(event.position.x()), static_cast<float>(event.position.y())};
    Core::Math::Vec2f delta = current_pos - state_.last_mouse_pos;
    state_.last_mouse_pos = current_pos;
    
    // Mouse look (only when dragging in orbital mode, or always in free mode)
    bool should_look = (state_.mode == ControlMode::Free) || 
                       (state_.mode == ControlMode::Orbital && state_.is_dragging);
    
    if (should_look && (std::abs(delta.x()) > 0.001f || std::abs(delta.y()) > 0.001f)) {
        float sensitivity = config_.mouse_sensitivity.value();
        float yaw_delta = -delta.x() * sensitivity;
        float pitch_delta = config_.invert_y ? delta.y() * sensitivity : -delta.y() * sensitivity;
        
        if (state_.mode == ControlMode::Orbital) {
            camera.orbit_horizontal(yaw_delta);
            camera.orbit_vertical(pitch_delta);
        } else {
            camera.rotate_yaw(yaw_delta);
            camera.rotate_pitch(pitch_delta);
        }
    }
    
    // Edge scrolling
    if (config_.edge_scrolling) {
        check_edge_scrolling(current_pos);
    }
    
    return {};
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_scroll_event(const UI::Window::ScrollEvent& event, Core::Graphics::Camera& camera) noexcept {
    float zoom_delta = static_cast<float>(event.offset.y()) * config_.scroll_sensitivity.value();
    zoom_delta = apply_zoom_constraints(zoom_delta);
    
    if (std::abs(zoom_delta) > 0.001f) {
        if (state_.mode == ControlMode::Orbital) {
            camera.zoom(1.0f + zoom_delta);
        } else {
            // In free mode, move forward/backward
            camera.move_forward(zoom_delta * config_.move_speed.value());
        }
    }
    
    return {};
}

Core::Modern::Result<void, std::string> Controls::Controls::handle_window_resize_event(const UI::Window::WindowResizeEvent& event) noexcept {
    state_.viewport_size = event.size;
    return {};
}

void Controls::Controls::process_continuous_input(Core::Graphics::Camera& camera, DeltaTime dt) noexcept {
    Core::Math::Vec2f movement{0.0f, 0.0f};
    float vertical_movement = 0.0f;
    
    // Process movement keys
    for (const auto& [action, binding] : config_.bindings) {
        if (!binding.is_key || !is_action_active(action)) continue;
        
        float speed = config_.move_speed.value() * dt.value();
        
        switch (action) {
            case ControlAction::MoveForward:
                movement.y() += speed;
                break;
            case ControlAction::MoveBack:
                movement.y() -= speed;
                break;
            case ControlAction::MoveLeft:
                movement.x() -= speed;
                break;
            case ControlAction::MoveRight:
                movement.x() += speed;
                break;
            case ControlAction::MoveUp:
                vertical_movement += speed;
                break;
            case ControlAction::MoveDown:
                vertical_movement -= speed;
                break;
            default:
                break;
        }
    }
    
    // Apply movement constraints and execute
    movement = apply_movement_constraints(movement);
    
    if (std::abs(movement.x()) > 0.001f) {
        camera.move_right(movement.x());
    }
    if (std::abs(movement.y()) > 0.001f) {
        camera.move_forward(movement.y());
    }
    if (std::abs(vertical_movement) > 0.001f) {
        camera.move_up(vertical_movement);
    }
}

void Controls::Controls::check_edge_scrolling(Core::Math::Vec2f mouse_pos) noexcept {
    if (!on_edge_scroll_) return;
    
    Core::Math::Vec2f scroll_delta{0.0f, 0.0f};
    float margin = config_.edge_scroll_margin;
    
    // Check edges
    if (mouse_pos.x() < margin) {
        scroll_delta.x() = -(margin - mouse_pos.x()) / margin;
    } else if (mouse_pos.x() > state_.viewport_size.x() - margin) {
        scroll_delta.x() = (mouse_pos.x() - (state_.viewport_size.x() - margin)) / margin;
    }
    
    if (mouse_pos.y() < margin) {
        scroll_delta.y() = (margin - mouse_pos.y()) / margin;
    } else if (mouse_pos.y() > state_.viewport_size.y() - margin) {
        scroll_delta.y() = -(mouse_pos.y() - (state_.viewport_size.y() - margin)) / margin;
    }
    
    if (scroll_delta.length() > 0.001f) {
        on_edge_scroll_(scroll_delta * config_.move_speed.value());
    }
}

bool Controls::Controls::is_action_active(ControlAction action) const noexcept {
    auto it = config_.bindings.find(action);
    if (it == config_.bindings.end() || !it->second.is_key) {
        return false;
    }
    
    std::uint16_t key_code = static_cast<std::uint16_t>(it->second.key);
    return key_code < 256 && state_.keys_pressed[key_code];
}

Core::Math::Vec2f Controls::Controls::apply_movement_constraints(const Core::Math::Vec2f& delta) const noexcept {
    // Could add constraints like max speed, restricted zones, etc.
    return delta;
}

float Controls::Controls::apply_zoom_constraints(float zoom_delta) const noexcept {
    // Clamp zoom delta to reasonable bounds
    return std::clamp(zoom_delta, -config_.zoom_speed.value(), config_.zoom_speed.value());
}

// Factory implementations
namespace Controls {
namespace Factory {

Controls orbital() noexcept {
    Config config;
    config.move_speed = Speed{25.0f};
    config.mouse_sensitivity = Sensitivity{0.003f};
    config.edge_scrolling = true;
    
    Controls controls(config);
    controls.set_mode(ControlMode::Orbital);
    return controls;
}

Controls free() noexcept {
    Config config;
    config.move_speed = Speed{10.0f};
    config.mouse_sensitivity = Sensitivity{0.002f};
    config.edge_scrolling = false;
    
    Controls controls(config);
    controls.set_mode(ControlMode::Free);
    return controls;
}

Controls cinematic() noexcept {
    Config config;
    config.move_speed = Speed{5.0f};
    
    Controls controls(config);
    controls.set_mode(ControlMode::Cinematic);
    return controls;
}

} // namespace Factory
} // namespace Controls

} // namespace Manifest::Render::CameraSystem

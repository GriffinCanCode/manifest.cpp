#include "Camera.hpp"

namespace Manifest::Render::CameraSystem {

// System implementation
System::System(std::unique_ptr<Cursor::IProvider> cursor_provider)
    : controls_(Controls::Factory::orbital())
    , cursor_manager_(cursor_provider ? std::move(cursor_provider) : std::make_unique<Cursor::MockProvider>())
{
    // Set up default camera position
    camera_.look_at(Core::Math::Vec3f{10.0f, 10.0f, 10.0f}, Core::Math::Vec3f{0.0f, 0.0f, 0.0f});
    
    // Configure cursor context updates
    cursor_manager_.set_resolver([this](const Cursor::Context& context) -> Cursor::Type {
        if (selection_manager_.is_box_selecting()) {
            return Cursor::Type::Crosshair;
        }
        return Cursor::DefaultResolver::resolve(context);
    });
    
    // Link selection events to cursor updates
    selection_manager_.set_selection_changed_callback([this](const auto& selected_ids) {
        sync_cursor_context();
    });
}

Core::Modern::Result<void, std::string> System::handle_event(const UI::Window::Event& event) noexcept {
    if (!active_) {
        return Core::Modern::Result<void, std::string>::success();
    }
    
    // Handle events in priority order
    
    // 1. Selection system (might consume mouse events)
    std::visit([this](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, UI::Window::MouseButtonEvent>) {
            selection_manager_.handle_mouse_button(e);
        } else if constexpr (std::is_same_v<T, UI::Window::MouseMoveEvent>) {
            selection_manager_.handle_mouse_move(e);
        } else if constexpr (std::is_same_v<T, UI::Window::KeyEvent>) {
            selection_manager_.handle_key(e);
        }
    }, event);
    
    // 2. Camera controls (if not consumed by selection)
    if (!selection_manager_.is_box_selecting()) {
        auto controls_result = controls_.handle_event(event, camera_);
        if (!controls_result.is_success()) {
            return controls_result;
        }
    }
    
    // 3. Update cursor context after event processing
    sync_cursor_context();
    
    return Core::Modern::Result<void, std::string>::success();
}

void System::update(float delta_time) noexcept {
    if (!active_) return;
    
    // Update all systems
    controls_.update(camera_);
    behavior_controller_.update(camera_, delta_time);
    cursor_manager_.update(delta_time);
    
    // Sync cursor context regularly
    sync_cursor_context();
}

void System::setup_orbital_controls() noexcept {
    controls_ = Controls::Factory::orbital();
    controls_.set_mode(ControlMode::Orbital);
}

void System::setup_free_controls() noexcept {
    controls_ = Controls::Factory::free();
    controls_.set_mode(ControlMode::Free);
}

void System::setup_cinematic_controls() noexcept {
    controls_ = Controls::Factory::cinematic();
    controls_.set_mode(ControlMode::Cinematic);
}

void System::sync_cursor_context() noexcept {
    Cursor::Context context;
    
    // Get mouse position
    auto pos_result = cursor_manager_.position();
    if (pos_result.is_success()) {
        context.mouse_position = pos_result.value();
    }
    
    // Update context based on current state
    context.is_dragging = selection_manager_.is_box_selecting();
    context.is_selection_active = selection_manager_.has_selection();
    context.ctrl_pressed = false;  // Would get from input system
    context.shift_pressed = false; // Would get from input system
    context.alt_pressed = false;   // Would get from input system
    
    // Update cursor manager with new context
    cursor_manager_.update_context(context);
}

void System::save_current_state(const std::string& name) noexcept {
    state_manager_.save_state(name, camera_, controls_.mode());
}

void System::restore_state(const std::string& name) noexcept {
    state_manager_.restore_state(name, camera_);
}

// Factory implementations
namespace Factory {

System create_rts_camera() noexcept {
    System system;
    system.setup_orbital_controls();
    
    // Position for RTS view
    system.camera().look_at(
        Core::Math::Vec3f{0.0f, 50.0f, 50.0f},
        Core::Math::Vec3f{0.0f, 0.0f, 0.0f}
    );
    
    // Save initial state
    system.save_current_state("rts_default");
    
    return system;
}

System create_fps_camera() noexcept {
    System system;
    system.setup_free_controls();
    
    // Position for first-person view
    system.camera().look_at(
        Core::Math::Vec3f{0.0f, 2.0f, 0.0f},
        Core::Math::Vec3f{0.0f, 2.0f, -1.0f}
    );
    
    system.save_current_state("fps_default");
    
    return system;
}

System create_city_camera() noexcept {
    System system;
    system.setup_orbital_controls();
    
    // Close-up orbital view for city management
    system.camera().look_at(
        Core::Math::Vec3f{10.0f, 15.0f, 10.0f},
        Core::Math::Vec3f{0.0f, 0.0f, 0.0f}
    );
    
    system.save_current_state("city_default");
    
    return system;
}

System create_world_camera() noexcept {
    System system;
    system.setup_orbital_controls();
    
    // High-altitude world overview
    system.camera().look_at(
        Core::Math::Vec3f{0.0f, 200.0f, 100.0f},
        Core::Math::Vec3f{0.0f, 0.0f, 0.0f}
    );
    
    system.save_current_state("world_default");
    
    return system;
}

} // namespace Factory

} // namespace Manifest::Render::CameraSystem

#include "Cursor.hpp"
#include "ResultHelper.hpp"
#include <algorithm>

namespace Manifest::Render::CameraSystem {

// Manager implementation
Cursor::Manager::Manager(std::unique_ptr<IProvider> provider) noexcept
    : provider_(std::move(provider))
    , resolver_(DefaultResolver::resolve)
{
}

void Cursor::Manager::update_context(const Context& context) noexcept {
    context_ = context;
    
    if (!state_.is_locked) {
        Type new_type = resolve_cursor_type();
        if (new_type != state_.target_type) {
            state_.target_type = new_type;
            state_.transition_progress = 0.0f;
        }
    }
}

Result<void, std::string> Cursor::Manager::set_cursor(Type type) noexcept {
    if (!provider_) {
        return error<void, std::string>("No cursor provider available");
    }
    
    state_.target_type = type;
    state_.transition_progress = 0.0f;
    
    return apply_cursor_change();
}

Result<void, std::string> Cursor::Manager::show(bool visible) noexcept {
    if (!provider_) {
        return error<void, std::string>("No cursor provider available");
    }
    
    state_.is_visible = visible;
    return provider_->show_cursor(visible);
}

Result<void, std::string> Cursor::Manager::lock(bool locked) noexcept {
    state_.is_locked = locked;
    return success<void, std::string>();
}

Result<Vec2i, std::string> Cursor::Manager::position() const noexcept {
    if (!provider_) {
        return error<Vec2i, std::string>("No cursor provider available");
    }
    
    return provider_->get_position();
}

Result<void, std::string> Cursor::Manager::set_position(Vec2i pos) noexcept {
    if (!provider_) {
        return error<void, std::string>("No cursor provider available");
    }
    
    state_.position = pos;
    return provider_->set_position(pos);
}

void Cursor::Manager::update(float delta_time) noexcept {
    update_transition(delta_time);
    update_auto_hide(delta_time);
    
    if (state_.needs_update()) {
        apply_cursor_change();
    }
}

void Cursor::Manager::set_resolver(std::function<Type(const Context&)> resolver) noexcept {
    resolver_ = std::move(resolver);
}

void Cursor::Manager::set_auto_hide(bool enabled, float delay) noexcept {
    auto_hide_enabled_ = enabled;
    auto_hide_delay_ = delay;
    auto_hide_timer_ = 0.0f;
}

Cursor::Type Cursor::Manager::resolve_cursor_type() const noexcept {
    if (resolver_) {
        return resolver_(context_);
    }
    return Type::Default;
}

void Cursor::Manager::update_transition(float delta_time) noexcept {
    if (state_.current_type != state_.target_type) {
        state_.transition_progress += transition_speed_ * delta_time;
        
        if (state_.transition_progress >= 1.0f) {
            state_.transition_progress = 1.0f;
            state_.current_type = state_.target_type;
        }
    }
}

void Cursor::Manager::update_auto_hide(float delta_time) noexcept {
    if (!auto_hide_enabled_ || state_.is_locked) return;
    
    // Reset timer if mouse moved
    auto current_pos = position();
    if (current_pos.has_value() && current_pos.value() != state_.position) {
        auto_hide_timer_ = 0.0f;
        state_.position = current_pos.value();
        
        if (!state_.is_visible) {
            show(true);
        }
        return;
    }
    
    // Increment timer
    auto_hide_timer_ += delta_time;
    
    // Auto-hide after delay
    if (auto_hide_timer_ >= auto_hide_delay_ && state_.is_visible) {
        show(false);
    }
}

Result<void, std::string> Cursor::Manager::apply_cursor_change() noexcept {
    if (!provider_ || state_.current_type == state_.target_type) {
        return success<void, std::string>();
    }
    
    // Check if target type is supported
    if (!provider_->is_supported(state_.target_type)) {
        // Fallback to default cursor
        state_.target_type = Cursor::Type::Default;
    }
    
    auto result = provider_->set_cursor(state_.target_type);
    if (result.has_value()) {
        state_.current_type = state_.target_type;
        state_.transition_progress = 1.0f;
    }
    
    return result;
}

// DefaultResolver implementation  
namespace Cursor {
namespace DefaultResolver {

// We're now inside Cursor::DefaultResolver namespace

Type resolve(const Context& context) noexcept {
    // Priority order: UI > Units > Cities > Terrain
    
    if (context.is_over_ui) {
        return for_ui(context);
    }
    
    if (context.is_over_unit) {
        return for_unit(context);
    }
    
    if (context.is_over_city) {
        return for_city(context);
    }
    
    if (context.is_over_terrain) {
        return for_terrain(context);
    }
    
    return Type::Default;
}

Type for_terrain(const Context& context) noexcept {
    if (context.is_dragging) {
        return Type::Grabbing;
    }
    
    if (context.is_selection_active) {
        return Type::Crosshair;
    }
    
    if (context.ctrl_pressed) {
        return Type::Move;
    }
    
    return Type::Default;
}

Type for_ui(const Context& context) noexcept {
    if (!context.can_interact) {
        return Type::NotAllowed;
    }
    
    if (context.is_dragging) {
        return Type::Grabbing;
    }
    
    // Would examine specific UI element type in real implementation
    return Cursor::Type::Hand;
}

Type for_unit(const Context& context) noexcept {
    if (!context.can_interact) {
        return Type::NotAllowed;
    }
    
    if (context.interaction_distance > 100.0f) {
        return Type::Default;
    }
    
    if (context.shift_pressed) {
        return Type::Crosshair;  // Attack cursor
    }
    
    if (context.alt_pressed) {
        return Type::Hand;  // Inspect cursor
    }
    
    return Cursor::Type::Move;  // Move unit cursor
}

Type for_city(const Context& context) noexcept {
    if (!context.can_interact) {
        return Type::NotAllowed;
    }
    
    return Cursor::Type::Hand;  // Enter city management
}

} // namespace DefaultResolver
} // namespace Cursor

// MockProvider implementation
Result<void, std::string> Cursor::MockProvider::set_cursor(Cursor::Type type) noexcept {
    current_type_ = type;
    return success<void, std::string>();
}

Result<void, std::string> Cursor::MockProvider::show_cursor(bool visible) noexcept {
    visible_ = visible;
    return success<void, std::string>();
}

Result<Vec2i, std::string> Cursor::MockProvider::get_position() const noexcept {
    return success<Vec2i, std::string>(position_);
}

Result<void, std::string> Cursor::MockProvider::set_position(Vec2i pos) noexcept {
    position_ = pos;
    return success<void, std::string>();
}

bool Cursor::MockProvider::is_supported(Cursor::Type type) const noexcept {
    // Mock provider supports all cursor types
    return true;
}

// Utility functions
namespace Util {

std::string to_string(Cursor::Type type) noexcept {
    switch (type) {
        case Cursor::Type::Default: return "default";
        case Cursor::Type::Hand: return "hand";
        case Cursor::Type::Grab: return "grab";
        case Cursor::Type::Grabbing: return "grabbing";
        case Cursor::Type::Move: return "move";
        case Cursor::Type::Crosshair: return "crosshair";
        case Cursor::Type::Text: return "text";
        case Cursor::Type::Wait: return "wait";
        case Cursor::Type::NotAllowed: return "not-allowed";
        case Cursor::Type::ResizeN: return "resize-n";
        case Cursor::Type::ResizeS: return "resize-s";
        case Cursor::Type::ResizeE: return "resize-e";
        case Cursor::Type::ResizeW: return "resize-w";
        case Cursor::Type::ResizeNE: return "resize-ne";
        case Cursor::Type::ResizeNW: return "resize-nw";
        case Cursor::Type::ResizeSE: return "resize-se";
        case Cursor::Type::ResizeSW: return "resize-sw";
        default: return "unknown";
    }
}

Vec2i default_hotspot(Cursor::Type type) noexcept {
    switch (type) {
        case Cursor::Type::Default:
        case Cursor::Type::Hand:
        case Cursor::Type::Wait:
        case Cursor::Type::NotAllowed:
            return Vec2i{0, 0};  // Top-left
            
        case Cursor::Type::Crosshair:
            return Vec2i{16, 16};  // Center (assuming 32x32 cursor)
            
        case Cursor::Type::Text:
            return Vec2i{8, 16};  // Middle-left
            
        case Cursor::Type::Move:
        case Cursor::Type::Grab:
        case Cursor::Type::Grabbing:
            return Vec2i{16, 16};  // Center
            
        default:
            return Vec2i{0, 0};
    }
}

bool is_resize_cursor(Cursor::Type type) noexcept {
    return type >= Cursor::Type::ResizeN && type <= Cursor::Type::ResizeSW;
}

bool is_interactive_cursor(Cursor::Type type) noexcept {
    return type == Cursor::Type::Hand || 
           type == Cursor::Type::Grab || 
           type == Cursor::Type::Grabbing ||
           type == Cursor::Type::Move ||
           type == Cursor::Type::Crosshair;
}

} // namespace Util

} // namespace Manifest::Render::CameraSystem

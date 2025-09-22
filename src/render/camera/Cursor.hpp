#pragma once

#include "../../core/types/Types.hpp"
#include "../../core/types/Modern.hpp"
#include "../../core/math/Vector.hpp"
#include <functional>
#include <string>

namespace Manifest::Render::CameraSystem {

using Core::Types::StrongId;
using Core::Modern::Result;
using Core::Math::Vec2i;

// Strong typing for cursor management
struct CursorTag {};
using CursorId = StrongId<CursorTag, std::uint8_t>;

// Cursor types for different contexts
enum class Type : std::uint8_t {
    Default,        // Standard arrow cursor
    Hand,           // Pointing hand for clickable items
    Grab,           // Open hand for draggable items  
    Grabbing,       // Closed hand while dragging
    Move,           // Four-directional move cursor
    Crosshair,      // Precision targeting
    Text,           // Text input cursor
    Wait,           // Loading/processing
    NotAllowed,     // Invalid action
    ResizeN,        // North resize
    ResizeS,        // South resize
    ResizeE,        // East resize
    ResizeW,        // West resize
    ResizeNE,       // Northeast resize
    ResizeNW,       // Northwest resize
    ResizeSE,       // Southeast resize
    ResizeSW        // Southwest resize
};

// Context for determining cursor type
struct Context {
    Vec2i mouse_position{0, 0};
    bool is_dragging{false};
    bool is_over_ui{false};
    bool is_over_terrain{false};
    bool is_over_unit{false};
    bool is_over_city{false};
    bool is_selection_active{false};
    bool ctrl_pressed{false};
    bool shift_pressed{false};
    bool alt_pressed{false};
    
    // Custom context data
    std::string tooltip_text{};
    float interaction_distance{0.0f};
    bool can_interact{true};
};

// Cursor state for animations and effects
struct CursorState {
    Type current_type{Type::Default};
    Type target_type{Type::Default};
    float transition_progress{0.0f};
    bool is_visible{true};
    bool is_locked{false};
    Vec2i position{0, 0};
    Vec2i hot_spot{0, 0};  // Cursor hotspot offset
    
    bool needs_update() const noexcept {
        return current_type != target_type || transition_progress < 1.0f;
    }
};

// Interface for cursor providers (platform-specific)
class IProvider {
public:
    virtual ~IProvider() = default;
    virtual Result<void, std::string> set_cursor(Type type) noexcept = 0;
    virtual Result<void, std::string> show_cursor(bool visible) noexcept = 0;
    virtual Result<Vec2i, std::string> get_position() const noexcept = 0;
    virtual Result<void, std::string> set_position(Vec2i pos) noexcept = 0;
    virtual bool is_supported(Type type) const noexcept = 0;
};

// Context-sensitive cursor manager
class Manager {
    std::unique_ptr<IProvider> provider_;
    CursorState state_;
    Context context_;
    std::function<Type(const Context&)> resolver_;
    float transition_speed_{5.0f};
    bool auto_hide_enabled_{true};
    float auto_hide_delay_{2.0f};
    float auto_hide_timer_{0.0f};
    
public:
    explicit Manager(std::unique_ptr<IProvider> provider) noexcept;
    
    // Context management
    void update_context(const Context& context) noexcept;
    const Context& context() const noexcept { return context_; }
    
    // Manual cursor control
    Result<void, std::string> set_cursor(Type type) noexcept;
    Result<void, std::string> show(bool visible) noexcept;
    Result<void, std::string> lock(bool locked) noexcept;
    
    // Position management
    Result<Vec2i, std::string> position() const noexcept;
    Result<void, std::string> set_position(Vec2i pos) noexcept;
    
    // Update system
    void update(float delta_time) noexcept;
    
    // Configuration
    void set_resolver(std::function<Type(const Context&)> resolver) noexcept;
    void set_transition_speed(float speed) noexcept { transition_speed_ = speed; }
    void set_auto_hide(bool enabled, float delay = 2.0f) noexcept;
    
    // State queries
    Type current_type() const noexcept { return state_.current_type; }
    bool is_visible() const noexcept { return state_.is_visible; }
    bool is_locked() const noexcept { return state_.is_locked; }
    bool is_transitioning() const noexcept { return state_.needs_update(); }
    
private:
    Type resolve_cursor_type() const noexcept;
    void update_transition(float delta_time) noexcept;
    void update_auto_hide(float delta_time) noexcept;
    Result<void, std::string> apply_cursor_change() noexcept;
};

// Default cursor resolution logic
namespace DefaultResolver {
    Type resolve(const Context& context) noexcept;
    Type for_terrain(const Context& context) noexcept;
    Type for_ui(const Context& context) noexcept;
    Type for_unit(const Context& context) noexcept;
    Type for_city(const Context& context) noexcept;
}

// Mock provider for testing
class MockProvider final : public IProvider {
    Type current_type_{Type::Default};
    bool visible_{true};
    Vec2i position_{0, 0};
    
public:
    Result<void, std::string> set_cursor(Type type) noexcept override;
    Result<void, std::string> show_cursor(bool visible) noexcept override;
    Result<Vec2i, std::string> get_position() const noexcept override;
    Result<void, std::string> set_position(Vec2i pos) noexcept override;
    bool is_supported(Type type) const noexcept override;
    
    // Testing interface
    Type get_current_type() const noexcept { return current_type_; }
    bool is_cursor_visible() const noexcept { return visible_; }
};

// Utility functions
namespace Util {
    std::string to_string(Type type) noexcept;
    Vec2i default_hotspot(Type type) noexcept;
    bool is_resize_cursor(Type type) noexcept;
    bool is_interactive_cursor(Type type) noexcept;
}

} // namespace Manifest::Render::CameraSystem

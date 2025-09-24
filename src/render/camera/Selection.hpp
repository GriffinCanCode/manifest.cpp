#pragma once

#include "../../core/types/Types.hpp"
#include "../../core/types/Modern.hpp"
#include "../../core/math/Vector.hpp"
#include "../../ui/window/Events.hpp"
#include <vector>
#include <unordered_set>
#include <functional>
#include <chrono>

namespace Manifest::Render::CameraSystem {

using namespace Core::Types;
using Core::Modern::Result;
using Core::Math::Vec2f;
using Core::Math::Vec2i;
using Core::Math::Vec3f;

namespace Selection {

// Strong typing for selection system
struct SelectionTag {};
using SelectionId = StrongId<SelectionTag, std::uint32_t>;

// Selection modes
enum class Mode : std::uint8_t {
    Replace,    // Replace current selection
    Add,        // Add to current selection (Ctrl+click)
    Remove,     // Remove from current selection (Ctrl+Shift+click)
    Toggle      // Toggle selection state (Alt+click)
};

// Selection area types
enum class Area : std::uint8_t {
    Point,      // Single point selection
    Box,        // Rectangular selection box
    Circle,     // Circular selection area
    Lasso,      // Free-form lasso selection
    Line        // Line-based selection
};

// Selection box state
struct Box {
    Vec2i start{0, 0};          // Start position in screen coordinates
    Vec2i end{0, 0};            // Current end position
    Vec2i min{0, 0};            // Top-left corner
    Vec2i max{0, 0};            // Bottom-right corner
    bool is_active{false};      // Currently being drawn
    bool is_valid{false};       // Has minimum size
    float min_size{3.0f};       // Minimum box size in pixels
    
    void update(Vec2i current_pos) noexcept;
    Vec2i size() const noexcept { return max - min; }
    Vec2i center() const noexcept { return (min + max) / 2; }
    bool contains(Vec2i point) const noexcept;
    float area() const noexcept;
};

// Selectable entity interface
template<typename EntityType>
class ISelectable {
public:
    virtual ~ISelectable() = default;
    virtual SelectionId id() const noexcept = 0;
    virtual Vec3f world_position() const noexcept = 0;
    virtual Vec2i screen_position() const noexcept = 0;
    virtual float selection_radius() const noexcept = 0;
    virtual bool is_selectable() const noexcept = 0;
    virtual bool is_in_box(const Box& box) const noexcept = 0;
    virtual EntityType& entity() noexcept = 0;
    virtual const EntityType& entity() const noexcept = 0;
};

// Selection state tracking
struct State {
    Area current_area{Area::Point};
    Mode current_mode{Mode::Replace};
    Box selection_box{};
    Vec2f mouse_start{0.0f, 0.0f};
    Vec2f mouse_current{0.0f, 0.0f};
    bool is_selecting{false};
    bool is_dragging{false};
    std::chrono::high_resolution_clock::time_point start_time{};
    UI::Window::MouseButton button{UI::Window::MouseButton::Left};
    UI::Window::Modifier modifiers{UI::Window::Modifier::None};
    
    bool is_multiselect() const noexcept {
        return static_cast<int>(modifiers) & static_cast<int>(UI::Window::Modifier::Ctrl);
    }
    
    bool is_deselect() const noexcept {
        return (static_cast<int>(modifiers) & static_cast<int>(UI::Window::Modifier::Ctrl)) &&
               (static_cast<int>(modifiers) & static_cast<int>(UI::Window::Modifier::Shift));
    }
    
    bool is_toggle() const noexcept {
        return static_cast<int>(modifiers) & static_cast<int>(UI::Window::Modifier::Alt);
    }
};

// Selection manager for entities of type T
template<typename EntityType>
class Manager {
    std::unordered_set<SelectionId> selected_ids_;
    std::vector<std::unique_ptr<ISelectable<EntityType>>> selectables_;
    State state_{};
    
    // Configuration
    float double_click_time_{0.3f};
    float drag_threshold_{5.0f};
    bool enable_multiselect_{true};
    bool enable_box_select_{true};
    
    // Callbacks
    std::function<void(const std::vector<SelectionId>&)> on_selection_changed_;
    std::function<void(SelectionId, const EntityType&)> on_entity_selected_;
    std::function<void(SelectionId, const EntityType&)> on_entity_deselected_;
    std::function<bool(const EntityType&)> selection_filter_;
    
public:
    Manager() = default;
    
    // Entity management
    void register_selectable(std::unique_ptr<ISelectable<EntityType>> selectable);
    void unregister_selectable(SelectionId id);
    void clear_selectables();
    
    // Event handling
    Result<void, std::string> handle_mouse_button(const UI::Window::MouseButtonEvent& event);
    Result<void, std::string> handle_mouse_move(const UI::Window::MouseMoveEvent& event);
    Result<void, std::string> handle_key(const UI::Window::KeyEvent& event);
    
    // Selection operations
    Result<void, std::string> select(SelectionId id, Mode mode = Mode::Replace);
    Result<void, std::string> select_multiple(const std::vector<SelectionId>& ids, Mode mode = Mode::Replace);
    Result<void, std::string> select_in_box(const Box& box, Mode mode = Mode::Replace);
    Result<void, std::string> select_all();
    Result<void, std::string> deselect(SelectionId id);
    Result<void, std::string> deselect_all();
    Result<void, std::string> invert_selection();
    
    // Selection queries
    std::vector<SelectionId> selected_ids() const;
    std::vector<EntityType*> selected_entities();
    std::vector<const EntityType*> selected_entities() const;
    bool is_selected(SelectionId id) const noexcept;
    std::size_t selection_count() const noexcept { return selected_ids_.size(); }
    bool has_selection() const noexcept { return !selected_ids_.empty(); }
    
    // State queries
    const Box& selection_box() const noexcept { return state_.selection_box; }
    bool is_box_selecting() const noexcept { return state_.is_selecting && state_.current_area == Area::Box; }
    Area current_area() const noexcept { return state_.current_area; }
    Mode current_mode() const noexcept { return state_.current_mode; }
    
    // Configuration
    void set_double_click_time(float time) noexcept { double_click_time_ = time; }
    void set_drag_threshold(float threshold) noexcept { drag_threshold_ = threshold; }
    void enable_multiselect(bool enable) noexcept { enable_multiselect_ = enable; }
    void enable_box_select(bool enable) noexcept { enable_box_select_ = enable; }
    
    // Callbacks
    void set_selection_changed_callback(std::function<void(const std::vector<SelectionId>&)> callback);
    void set_entity_selected_callback(std::function<void(SelectionId, const EntityType&)> callback);
    void set_entity_deselected_callback(std::function<void(SelectionId, const EntityType&)> callback);
    void set_selection_filter(std::function<bool(const EntityType&)> filter);
    
private:
    void update_selection_mode() noexcept;
    std::vector<SelectionId> find_entities_in_box(const Box& box) const;
    ISelectable<EntityType>* find_selectable(SelectionId id) const;
    void notify_selection_changed();
    bool should_start_box_selection(Vec2f delta) const noexcept;
    Mode determine_mode() const noexcept;
};

// Concrete selectable implementations for common entity types
namespace Selectables {

// Example unit selectable
class Unit : public ISelectable<int> {  // Using int as placeholder entity type
    SelectionId id_;
    Vec3f world_pos_;
    Vec2i screen_pos_;
    float radius_;
    int unit_entity_;
    
public:
    Unit(SelectionId id, Vec3f world_pos, int entity);
    
    SelectionId id() const noexcept override { return id_; }
    Vec3f world_position() const noexcept override { return world_pos_; }
    Vec2i screen_position() const noexcept override { return screen_pos_; }
    float selection_radius() const noexcept override { return radius_; }
    bool is_selectable() const noexcept override { return true; }
    bool is_in_box(const Box& box) const noexcept override;
    int& entity() noexcept override { return unit_entity_; }
    const int& entity() const noexcept override { return unit_entity_; }
    
    void update_screen_position(Vec2i pos) noexcept { screen_pos_ = pos; }
};

} // namespace Selectables

// Utility functions for selection rendering
namespace Render {
    struct BoxStyle {
        Vec3f border_color{0.2f, 0.6f, 1.0f};
        Vec3f fill_color{0.2f, 0.6f, 1.0f};
        float border_width{2.0f};
        float fill_alpha{0.1f};
        bool dashed_border{true};
        float dash_length{5.0f};
    };
    
    void draw_selection_box(const Box& box, const BoxStyle& style = {});
    void draw_selection_highlight(Vec2i position, float radius, const Vec3f& color = {1.0f, 1.0f, 0.0f});
}

} // namespace Selection

} // namespace Manifest::Render::CameraSystem

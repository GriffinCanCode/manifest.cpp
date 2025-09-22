#include "Selection.hpp"
#include "ResultHelper.hpp"
#include <algorithm>
#include <cmath>

namespace Manifest::Render::CameraSystem {

// Box implementation
void Box::update(Vec2i current_pos) noexcept {
    end = current_pos;
    
    // Calculate min/max bounds
    min = Vec2i{std::min(start.x(), end.x()), std::min(start.y(), end.y())};
    max = Vec2i{std::max(start.x(), end.x()), std::max(start.y(), end.y())};
    
    // Check if box meets minimum size requirements
    Vec2i box_size = size();
    is_valid = (box_size.x() >= min_size && box_size.y() >= min_size);
}

bool Box::contains(Vec2i point) const noexcept {
    return point.x() >= min.x() && point.x() <= max.x() &&
           point.y() >= min.y() && point.y() <= max.y();
}

float Box::area() const noexcept {
    Vec2i box_size = size();
    return static_cast<float>(box_size.x() * box_size.y());
}

// Manager implementation
template<typename EntityType>
void Manager<EntityType>::register_selectable(std::unique_ptr<ISelectable<EntityType>> selectable) {
    if (selectable) {
        selectables_.push_back(std::move(selectable));
    }
}

template<typename EntityType>
void Manager<EntityType>::unregister_selectable(SelectionId id) {
    // Remove from selection if selected
    selected_ids_.erase(id);
    
    // Remove from selectables
    selectables_.erase(
        std::remove_if(selectables_.begin(), selectables_.end(),
            [id](const std::unique_ptr<ISelectable<EntityType>>& selectable) {
                return selectable->id() == id;
            }),
        selectables_.end()
    );
    
    notify_selection_changed();
}

template<typename EntityType>
void Manager<EntityType>::clear_selectables() {
    selectables_.clear();
    selected_ids_.clear();
    notify_selection_changed();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::handle_mouse_button(const UI::Window::MouseButtonEvent& event) {
    if (event.button != UI::Window::MouseButton::Left) {
        return success<void, std::string>();
    }
    
    Vec2f mouse_pos{static_cast<float>(event.position.x()), static_cast<float>(event.position.y())};
    
    if (event.action == UI::Window::Action::Press) {
        state_.mouse_start = mouse_pos;
        state_.mouse_current = mouse_pos;
        state_.button = event.button;
        state_.modifiers = event.mods;
        state_.start_time = std::chrono::high_resolution_clock::now();
        state_.is_dragging = false;
        
        // Initialize selection box
        state_.selection_box.start = Vec2i{static_cast<int>(mouse_pos.x()), static_cast<int>(mouse_pos.y())};
        state_.selection_box.end = state_.selection_box.start;
        state_.selection_box.is_active = false;
        state_.selection_box.is_valid = false;
        
        update_selection_mode();
        
    } else if (event.action == UI::Window::Action::Release) {
        if (state_.is_selecting && state_.selection_box.is_active) {
            // Complete box selection
            if (state_.selection_box.is_valid) {
                select_in_box(state_.selection_box, state_.current_mode);
            }
            
            state_.is_selecting = false;
            state_.selection_box.is_active = false;
        } else if (!state_.is_dragging) {
            // Point selection - find entity at click position
            Vec2i click_pos{static_cast<int>(mouse_pos.x()), static_cast<int>(mouse_pos.y())};
            SelectionId clicked_id = SelectionId::invalid();
            
            for (const auto& selectable : selectables_) {
                Vec2i diff = selectable->screen_position() - click_pos;
            float distance = std::sqrt(static_cast<float>(diff.x() * diff.x() + diff.y() * diff.y()));
            if (selectable->is_selectable() && distance <= selectable->selection_radius()) {
                    clicked_id = selectable->id();
                    break;
                }
            }
            
            if (clicked_id.is_valid()) {
                select(clicked_id, state_.current_mode);
            } else if (state_.current_mode == Mode::Replace) {
                deselect_all();
            }
        }
        
        state_.is_dragging = false;
    }
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::handle_mouse_move(const UI::Window::MouseMoveEvent& event) {
    Vec2f current_pos{static_cast<float>(event.position.x()), static_cast<float>(event.position.y())};
    state_.mouse_current = current_pos;
    
    if (state_.button == UI::Window::MouseButton::Left) {
        Vec2f delta = current_pos - state_.mouse_start;
        
        if (!state_.is_dragging && should_start_box_selection(delta)) {
            state_.is_dragging = true;
            
            if (enable_box_select_) {
                state_.is_selecting = true;
                state_.current_area = Area::Box;
                state_.selection_box.is_active = true;
            }
        }
        
        if (state_.is_selecting && state_.selection_box.is_active) {
            Vec2i mouse_int{static_cast<int>(current_pos.x()), static_cast<int>(current_pos.y())};
            state_.selection_box.update(mouse_int);
        }
    }
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::handle_key(const UI::Window::KeyEvent& event) {
    if (event.action != UI::Window::Action::Press) {
        return success<void, std::string>();
    }
    
    switch (event.key) {
        case UI::Window::KeyCode::A:
            if (static_cast<int>(event.mods) & static_cast<int>(UI::Window::Modifier::Ctrl)) {
                return select_all();
            }
            break;
            
        case UI::Window::KeyCode::Escape:
            return deselect_all();
            
        case UI::Window::KeyCode::I:
            if (static_cast<int>(event.mods) & static_cast<int>(UI::Window::Modifier::Ctrl)) {
                return invert_selection();
            }
            break;
            
        default:
            break;
    }
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::select(SelectionId id, Mode mode) {
    auto* selectable = find_selectable(id);
    if (!selectable || !selectable->is_selectable()) {
        return error<void, std::string>("Entity not selectable");
    }
    
    if (selection_filter_ && !selection_filter_(selectable->entity())) {
        return error<void, std::string>("Entity filtered out");
    }
    
    bool was_selected = is_selected(id);
    bool changed = false;
    
    switch (mode) {
        case Mode::Replace:
            if (!selected_ids_.empty() || !was_selected) {
                selected_ids_.clear();
                selected_ids_.insert(id);
                changed = true;
            }
            break;
            
        case Mode::Add:
            if (!was_selected) {
                selected_ids_.insert(id);
                changed = true;
            }
            break;
            
        case Mode::Remove:
            if (was_selected) {
                selected_ids_.erase(id);
                changed = true;
            }
            break;
            
        case Mode::Toggle:
            if (was_selected) {
                selected_ids_.erase(id);
            } else {
                selected_ids_.insert(id);
            }
            changed = true;
            break;
    }
    
    if (changed) {
        if (!was_selected && is_selected(id) && on_entity_selected_) {
            on_entity_selected_(id, selectable->entity());
        } else if (was_selected && !is_selected(id) && on_entity_deselected_) {
            on_entity_deselected_(id, selectable->entity());
        }
        
        notify_selection_changed();
    }
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::select_multiple(const std::vector<SelectionId>& ids, Mode mode) {
    if (mode == Mode::Replace) {
        selected_ids_.clear();
    }
    
    bool changed = false;
    
    for (SelectionId id : ids) {
        auto* selectable = find_selectable(id);
        if (!selectable || !selectable->is_selectable()) continue;
        
        if (selection_filter_ && !selection_filter_(selectable->entity())) continue;
        
        bool was_selected = is_selected(id);
        
        switch (mode) {
            case Mode::Replace:
            case Mode::Add:
                if (!was_selected) {
                    selected_ids_.insert(id);
                    changed = true;
                    if (on_entity_selected_) {
                        on_entity_selected_(id, selectable->entity());
                    }
                }
                break;
                
            case Mode::Remove:
                if (was_selected) {
                    selected_ids_.erase(id);
                    changed = true;
                    if (on_entity_deselected_) {
                        on_entity_deselected_(id, selectable->entity());
                    }
                }
                break;
                
            case Mode::Toggle:
                if (was_selected) {
                    selected_ids_.erase(id);
                    if (on_entity_deselected_) {
                        on_entity_deselected_(id, selectable->entity());
                    }
                } else {
                    selected_ids_.insert(id);
                    if (on_entity_selected_) {
                        on_entity_selected_(id, selectable->entity());
                    }
                }
                changed = true;
                break;
        }
    }
    
    if (changed) {
        notify_selection_changed();
    }
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::select_in_box(const Box& box, Mode mode) {
    std::vector<SelectionId> entities_in_box = find_entities_in_box(box);
    return select_multiple(entities_in_box, mode);
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::select_all() {
    std::vector<SelectionId> all_ids;
    
    for (const auto& selectable : selectables_) {
        if (selectable->is_selectable()) {
            if (!selection_filter_ || selection_filter_(selectable->entity())) {
                all_ids.push_back(selectable->id());
            }
        }
    }
    
    return select_multiple(all_ids, Mode::Replace);
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::deselect_all() {
    if (selected_ids_.empty()) {
        return success<void, std::string>();
    }
    
    for (SelectionId id : selected_ids_) {
        auto* selectable = find_selectable(id);
        if (selectable && on_entity_deselected_) {
            on_entity_deselected_(id, selectable->entity());
        }
    }
    
    selected_ids_.clear();
    notify_selection_changed();
    
    return success<void, std::string>();
}

template<typename EntityType>
Result<void, std::string> Manager<EntityType>::invert_selection() {
    std::unordered_set<SelectionId> new_selection;
    
    for (const auto& selectable : selectables_) {
        if (selectable->is_selectable()) {
            if (!selection_filter_ || selection_filter_(selectable->entity())) {
                SelectionId id = selectable->id();
                if (selected_ids_.find(id) == selected_ids_.end()) {
                    new_selection.insert(id);
                }
            }
        }
    }
    
    selected_ids_ = std::move(new_selection);
    notify_selection_changed();
    
    return success<void, std::string>();
}

template<typename EntityType>
std::vector<SelectionId> Manager<EntityType>::selected_ids() const {
    return std::vector<SelectionId>(selected_ids_.begin(), selected_ids_.end());
}

template<typename EntityType>
std::vector<EntityType*> Manager<EntityType>::selected_entities() {
    std::vector<EntityType*> entities;
    entities.reserve(selected_ids_.size());
    
    for (SelectionId id : selected_ids_) {
        auto* selectable = find_selectable(id);
        if (selectable) {
            entities.push_back(&selectable->entity());
        }
    }
    
    return entities;
}

template<typename EntityType>
std::vector<const EntityType*> Manager<EntityType>::selected_entities() const {
    std::vector<const EntityType*> entities;
    entities.reserve(selected_ids_.size());
    
    for (SelectionId id : selected_ids_) {
        const auto* selectable = find_selectable(id);
        if (selectable) {
            entities.push_back(&selectable->entity());
        }
    }
    
    return entities;
}

template<typename EntityType>
bool Manager<EntityType>::is_selected(SelectionId id) const noexcept {
    return selected_ids_.find(id) != selected_ids_.end();
}

template<typename EntityType>
void Manager<EntityType>::set_selection_changed_callback(std::function<void(const std::vector<SelectionId>&)> callback) {
    on_selection_changed_ = std::move(callback);
}

template<typename EntityType>
void Manager<EntityType>::set_entity_selected_callback(std::function<void(SelectionId, const EntityType&)> callback) {
    on_entity_selected_ = std::move(callback);
}

template<typename EntityType>
void Manager<EntityType>::set_entity_deselected_callback(std::function<void(SelectionId, const EntityType&)> callback) {
    on_entity_deselected_ = std::move(callback);
}

template<typename EntityType>
void Manager<EntityType>::set_selection_filter(std::function<bool(const EntityType&)> filter) {
    selection_filter_ = std::move(filter);
}

template<typename EntityType>
void Manager<EntityType>::update_selection_mode() noexcept {
    state_.current_mode = determine_mode();
}

template<typename EntityType>
std::vector<SelectionId> Manager<EntityType>::find_entities_in_box(const Box& box) const {
    std::vector<SelectionId> result;
    
    for (const auto& selectable : selectables_) {
        if (selectable->is_selectable() && selectable->is_in_box(box)) {
            if (!selection_filter_ || selection_filter_(selectable->entity())) {
                result.push_back(selectable->id());
            }
        }
    }
    
    return result;
}

template<typename EntityType>
ISelectable<EntityType>* Manager<EntityType>::find_selectable(SelectionId id) const {
    auto it = std::find_if(selectables_.begin(), selectables_.end(),
        [id](const std::unique_ptr<ISelectable<EntityType>>& selectable) {
            return selectable->id() == id;
        });
    
    return (it != selectables_.end()) ? it->get() : nullptr;
}

template<typename EntityType>
void Manager<EntityType>::notify_selection_changed() {
    if (on_selection_changed_) {
        on_selection_changed_(selected_ids());
    }
}

template<typename EntityType>
bool Manager<EntityType>::should_start_box_selection(Vec2f delta) const noexcept {
    return enable_box_select_ && delta.length() >= drag_threshold_;
}

template<typename EntityType>
Mode Manager<EntityType>::determine_mode() const noexcept {
    if (!enable_multiselect_) {
        return Mode::Replace;
    }
    
    if (state_.is_toggle()) {
        return Mode::Toggle;
    } else if (state_.is_deselect()) {
        return Mode::Remove;
    } else if (state_.is_multiselect()) {
        return Mode::Add;
    }
    
    return Mode::Replace;
}

// Explicit template instantiations for common types
template class Manager<int>;  // Placeholder - would use actual entity types

// Selectables implementation
namespace Selectables {

Unit::Unit(SelectionId id, Vec3f world_pos, int entity)
    : id_(id)
    , world_pos_(world_pos)
    , screen_pos_{0, 0}
    , radius_(10.0f)
    , unit_entity_(entity)
{
}

bool Unit::is_in_box(const Box& box) const noexcept {
    return box.contains(screen_pos_);
}

} // namespace Selectables

// Render utilities (stub implementations)
namespace Render {

void draw_selection_box(const Box& box, const BoxStyle& style) {
    // Would implement actual rendering here
    // This is a placeholder for the rendering system integration
}

void draw_selection_highlight(Vec2i position, float radius, const Vec3f& color) {
    // Would implement actual rendering here
    // This is a placeholder for the rendering system integration
}

} // namespace Render

} // namespace Manifest::Render::CameraSystem

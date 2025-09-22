#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <vector>

#include "../../core/types/Types.hpp"

namespace Manifest::World::Tiles {

using Core::Types::Layer;
using Core::Types::TileId;
using Core::Types::HexCoordinate;

// Forward declarations  
enum class ImprovementType : std::uint8_t;
enum class ResourceType : std::uint8_t;

// Layer-specific data for multi-layer tile system
struct LayerData {
private:
    Layer layer_{Layer::Surface};
    std::uint8_t density_{255};      // Material density (air=0, rock=255)
    std::uint8_t accessibility_{0};   // How accessible this layer is
    bool occupied_{false};            // Has structures/units
    bool excavated_{false};           // Has been dug out/modified
    
    // These would be proper enums when available
    std::uint8_t improvement_{0};    // ImprovementType placeholder
    std::uint8_t resource_{0};       // ResourceType placeholder

public:
    explicit LayerData(Layer layer = Layer::Surface) noexcept : layer_{layer} {}
    
    Layer layer() const noexcept { return layer_; }
    std::uint8_t density() const noexcept { return density_; }
    std::uint8_t accessibility() const noexcept { return accessibility_; }
    bool occupied() const noexcept { return occupied_; }
    bool excavated() const noexcept { return excavated_; }
    
    void set_density(std::uint8_t value) noexcept { density_ = value; }
    void set_accessibility(std::uint8_t value) noexcept { accessibility_ = value; }
    void set_occupied(bool value) noexcept { occupied_ = value; }
    void set_excavated(bool value) noexcept { excavated_ = value; }
    
    constexpr bool operator==(const LayerData& other) const noexcept {
        return layer_ == other.layer_ && 
               density_ == other.density_ &&
               accessibility_ == other.accessibility_ &&
               occupied_ == other.occupied_ &&
               excavated_ == other.excavated_ &&
               improvement_ == other.improvement_ &&
               resource_ == other.resource_;
    }
};

// Multi-layer tile supporting different elevation layers
class LayeredTile {
    // Core tile data remains at surface layer
    TileId id_{TileId::invalid()};
    HexCoordinate coordinate_{};
    
    // Layer data indexed by Layer enum
    std::array<std::optional<LayerData>, 4> layers_;
    
    // Surface layer is always present
    LayerData surface_layer_{Layer::Surface};
    
public:
    explicit LayeredTile(TileId tile_id, const HexCoordinate& coord) noexcept
        : id_{tile_id}, coordinate_{coord} {
        surface_layer_.set_density(128);
        layers_[static_cast<std::size_t>(Layer::Surface)] = surface_layer_;
    }
    
    // Basic accessors
    TileId id() const noexcept { return id_; }
    const HexCoordinate& coordinate() const noexcept { return coordinate_; }
    
    // Layer management
    bool has_layer(Layer layer) const noexcept {
        auto idx = static_cast<std::size_t>(layer);
        return idx < layers_.size() && layers_[idx].has_value();
    }
    
    const LayerData* get_layer(Layer layer) const noexcept {
        auto idx = static_cast<std::size_t>(layer);
        return (idx < layers_.size() && layers_[idx]) ? &layers_[idx].value() : nullptr;
    }
    
    LayerData* get_layer(Layer layer) noexcept {
        auto idx = static_cast<std::size_t>(layer);
        return (idx < layers_.size() && layers_[idx]) ? &layers_[idx].value() : nullptr;
    }
    
    void add_layer(const LayerData& data) {
        auto idx = static_cast<std::size_t>(data.layer());
        if (idx < layers_.size()) {
            layers_[idx] = data;
        }
    }
    
    void remove_layer(Layer layer) noexcept {
        auto idx = static_cast<std::size_t>(layer);
        if (idx < layers_.size() && layer != Layer::Surface) {
            layers_[idx].reset();
        }
    }
    
    // Layer queries
    std::vector<Layer> active_layers() const {
        std::vector<Layer> active;
        for (std::size_t idx = 0; idx < layers_.size(); ++idx) {
            if (layers_[idx]) {
                active.push_back(static_cast<Layer>(idx));
            }
        }
        return active;
    }
    
    // Layer-specific improvement checks
    bool can_excavate(Layer target_layer) const noexcept {
        if (target_layer == Layer::Surface) {
            return false;
        }
        
        const auto* layer_data = get_layer(target_layer);
        if (!layer_data) {
            return true; // Can create new layer
        }
        
        return !layer_data->excavated() && layer_data->density() > 50;
    }
    
    // Movement costs between layers
    float movement_cost_to_layer(Layer from, Layer target) const noexcept {
        if (from == target) {
            return 0.0F;
        }
        
        const auto* from_data = get_layer(from);
        const auto* to_data = get_layer(target);
        
        if (!from_data || !to_data) {
            return std::numeric_limits<float>::infinity();
        }
        
        // Moving up layers is easier than moving down
        float base_cost = 1.0F;
        if (static_cast<int>(target) > static_cast<int>(from)) {
            base_cost *= 2.0F; // Going up is harder
        }
        
        // Factor in target layer accessibility
        float accessibility_factor = (255.0F - static_cast<float>(to_data->accessibility())) / 255.0F;
        return base_cost * (1.0F + accessibility_factor);
    }
    
    // Underground/orbital construction (placeholder until ImprovementType available)
    bool can_build_at_layer(Layer layer, std::uint8_t /*improvement*/) const noexcept {
        const auto* layer_data = get_layer(layer);
        if (!layer_data) {
            return false;
        }
        
        switch (layer) {
            case Layer::Underground:
                return layer_data->excavated();
            
            case Layer::Surface:
                return true; // Normal surface rules apply
                
            case Layer::Air:
                return layer_data->accessibility() > 200; // Very accessible
                
            case Layer::Space:
                return layer_data->accessibility() == 255; // Fully accessible
        }
        
        return false;
    }
};

// Layer transition rules and effects
class LayerTransition {
    Layer from_layer_;
    Layer to_layer_;
    float transition_cost_;
    std::vector<std::uint8_t> required_improvements_; // ImprovementType placeholder
    
public:
    LayerTransition(Layer from, Layer target, float cost) noexcept
        : from_layer_{from}, to_layer_{target}, transition_cost_{cost} {}
    
    LayerTransition& requires(std::uint8_t improvement) {
        required_improvements_.push_back(improvement);
        return *this;
    }
    
    bool can_transition(const LayeredTile& tile) const noexcept {
        if (!tile.has_layer(from_layer_) || !tile.has_layer(to_layer_)) {
            return false;
        }
        
        // Check if required improvements exist (placeholder logic)
        for (auto improvement : required_improvements_) {
            bool found = false;
            for (auto layer : {from_layer_, to_layer_}) {
                const auto* layer_data = tile.get_layer(layer);
                if (layer_data) {
                    // Would check layer_data->improvement() == improvement when available
                    found = true; // Placeholder
                    (void)improvement; // Silence unused warning
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        
        return true;
    }
    
    float cost() const noexcept { return transition_cost_; }
};

// Registry for layer transition rules
class LayerRegistry {
    std::vector<LayerTransition> transitions_;
    
public:
    LayerRegistry();
    
    std::optional<float> transition_cost(const LayeredTile& /*tile*/, 
                                        Layer /*from*/, Layer /*target*/) const noexcept {
        for (const auto& transition : transitions_) {
            if (transition.can_transition(tile)) {
                return transition.cost();
            }
        }
        return std::nullopt;
    }
    
    void add_transition(LayerTransition transition) {
        transitions_.emplace_back(std::move(transition));
    }
};

LayerRegistry& get_layer_registry();

} // namespace Manifest::World::Tiles

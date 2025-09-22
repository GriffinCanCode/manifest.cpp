#include "Tile.hpp"
#include "Rules.hpp"
#include "Culture.hpp"
#include "Ownership.hpp"
#include "Layers.hpp"
#include "Supply.hpp"

namespace Manifest::World::Tiles {

// Static supply network reference
SupplyNetwork* Tile::supply_network_ = nullptr;

bool Tile::can_build_improvement(ImprovementType improvement) const noexcept {
    auto& rules = get_improvement_rules();
    
    // TODO: Integrate with proper TileMap when available
    // For now, use basic terrain/resource checks
    const auto* rule = rules.get_rule(improvement);
    if (!rule) {
        // Fallback to basic rules
        switch (improvement) {
            case ImprovementType::Farm:
                return is_land() && data_.terrain != TerrainType::Mountains;
            case ImprovementType::Mine:
                return data_.terrain == TerrainType::Hills ||
                       data_.terrain == TerrainType::Mountains;
            case ImprovementType::FishingBoats:
                return is_water();
            default:
                return is_land();
        }
    }
    
    // Use rule system (simplified without full TileMap integration for now)
    return rule != nullptr;
}

std::vector<ImprovementType> Tile::available_improvements() const {
    std::vector<ImprovementType> available;
    
    // Check each improvement type
    for (auto improvement : {ImprovementType::Farm, ImprovementType::Mine, 
                            ImprovementType::Quarry, ImprovementType::FishingBoats,
                            ImprovementType::Road, ImprovementType::Railroad, 
                            ImprovementType::Fort}) {
        if (can_build_improvement(improvement)) {
            available.push_back(improvement);
        }
    }
    
    return available;
}

bool Tile::has_layer(Layer layer) const noexcept {
    if (!layers_) {
        return layer == Layer::Surface; // Surface layer always exists
    }
    return layers_->has_layer(layer);
}

void Tile::add_layer(Layer layer, std::uint8_t density, std::uint8_t accessibility) {
    ensure_layers();
    
    LayerData data{layer};
    data.set_density(density);
    data.set_accessibility(accessibility);
    layers_->add_layer(data);
}

void Tile::remove_layer(Layer layer) noexcept {
    if (layers_) {
        layers_->remove_layer(layer);
    }
}

float Tile::movement_cost_to_layer(Layer from, Layer to) const noexcept {
    if (!layers_) {
        return from == to ? 0.0F : std::numeric_limits<float>::infinity();
    }
    return layers_->movement_cost_to_layer(from, to);
}

bool Tile::can_build_at_layer(Layer layer, ImprovementType improvement) const noexcept {
    if (!layers_) {
        return layer == Layer::Surface; // Only surface building without layers
    }
    return layers_->can_build_at_layer(layer, static_cast<std::uint8_t>(improvement));
}

void Tile::ensure_culture() {
    if (!culture_) {
        culture_ = std::make_unique<TileCulture>();
    }
}

void Tile::add_cultural_influence(NationId nation, std::uint8_t trait, Influence strength) {
    ensure_culture();
    
    CulturalInfluence influence;
    influence.source_nation = nation;
    influence.trait = static_cast<CulturalTrait>(trait);
    influence.strength = strength;
    influence.established_turn = 0; // Would get from game state
    
    culture_->add_influence(influence);
}

NationId Tile::dominant_culture() const noexcept {
    return culture_ ? culture_->dominant_culture() : NationId::invalid();
}

bool Tile::is_culturally_contested() const noexcept {
    return culture_ ? culture_->is_culturally_contested() : false;
}

void Tile::ensure_ownership() {
    if (!ownership_) {
        ownership_ = std::make_unique<TileOwnership>();
    }
}

void Tile::add_ownership_claim(NationId nation, std::uint8_t claim_type, Influence strength) {
    ensure_ownership();
    
    OwnershipClaim claim;
    claim.claimant = nation;
    claim.type = static_cast<OwnershipClaim::Type>(claim_type);
    claim.strength = strength;
    claim.established_turn = 0; // Would get from game state
    
    ownership_->add_claim(claim);
}

bool Tile::has_claim(NationId nation) const noexcept {
    return ownership_ ? ownership_->has_claim(nation) : false;
}

bool Tile::is_contested() const noexcept {
    return ownership_ ? ownership_->is_contested() : false;
}

float Tile::influence_ratio(NationId nation) const noexcept {
    return ownership_ ? ownership_->influence_ratio(nation) : 0.0F;
}

void Tile::add_as_producer(std::uint8_t resource_type, Production capacity) {
    if (!supply_network_) return;
    
    SupplyNode node;
    node.tile_id = data_.id;
    node.type = SupplyNode::Type::Producer;
    node.resource_type = resource_type;
    node.capacity = capacity;
    node.current_supply = capacity;
    
    supply_network_->add_node(node);
}

void Tile::add_as_consumer(std::uint8_t resource_type, Production demand) {
    if (!supply_network_) return;
    
    SupplyNode node;
    node.tile_id = data_.id;
    node.type = SupplyNode::Type::Consumer;
    node.resource_type = resource_type;
    node.demand = demand;
    
    supply_network_->add_node(node);
}

void Tile::add_as_storage(std::uint8_t resource_type, Production capacity) {
    if (!supply_network_) return;
    
    SupplyNode node;
    node.tile_id = data_.id;
    node.type = SupplyNode::Type::Storage;
    node.resource_type = resource_type;
    node.capacity = capacity;
    
    supply_network_->add_node(node);
}

void Tile::update(GameTurn current_turn) {
    // Update cultural influences (decay over time)
    if (culture_) {
        culture_->update(current_turn);
    }
    
    // Update ownership claims could go here if they decay
    // Update layer systems could go here for dynamic changes
    
    // Recalculate yields if anything changed
    calculate_base_yields();
}

void Tile::ensure_layers() {
    if (!layers_) {
        layers_ = std::make_unique<LayeredTile>(data_.id, data_.coordinate);
    }
}

} // namespace Manifest::World::Tiles

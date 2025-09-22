#include "Rules.hpp"
#include "Tile.hpp"
#include "Map.hpp"

namespace Manifest::World::Tiles {

bool TechPrereq::check(const Tile& /*tile*/, const TileMap& /*map*/) const noexcept {
    // For now, return true - would integrate with tech system later
    return true;
}

bool NeighborPrereq::check(const Tile& tile, const TileMap& map) const noexcept {
    std::uint8_t count = 0;
    const auto& neighbors = tile.neighbors();
    
    for (const auto& neighbor_id : neighbors) {
        if (!neighbor_id.is_valid()) {
            continue;
        }
        
        const Tile* neighbor = map.get_tile(neighbor_id);
        if (neighbor && neighbor->improvement() == required_neighbor_) {
            ++count;
            if (count >= min_count_) {
                return true;
            }
        }
    }
    
    return count >= min_count_;
}

RuleRegistry::RuleRegistry() {
    // Define improvement rules with prerequisites
    
    // Basic improvements
    add_rule(ImprovementRule{ImprovementType::Farm}
        .requires(TerrainPrereq{TerrainType::Grassland})
        .costs(Production{50.0})
        .build_time(2));
    
    add_rule(ImprovementRule{ImprovementType::Farm}
        .requires(TerrainPrereq{TerrainType::Plains})
        .costs(Production{50.0})
        .build_time(2));
    
    add_rule(ImprovementRule{ImprovementType::Mine}
        .requires(TerrainPrereq{TerrainType::Hills})
        .costs(Production{80.0})
        .build_time(3));
    
    add_rule(ImprovementRule{ImprovementType::Mine}
        .requires(TerrainPrereq{TerrainType::Mountains})
        .requires(ResourcePrereq{ResourceType::Iron})
        .costs(Production{100.0})
        .build_time(4));
    
    add_rule(ImprovementRule{ImprovementType::Quarry}
        .requires(TerrainPrereq{TerrainType::Hills})
        .requires(ResourcePrereq{ResourceType::Stone})
        .costs(Production{60.0})
        .build_time(3));
    
    add_rule(ImprovementRule{ImprovementType::FishingBoats}
        .requires(TerrainPrereq{TerrainType::Coast})
        .requires(ResourcePrereq{ResourceType::Fish})
        .costs(Production{40.0})
        .build_time(2));
    
    // Advanced improvements with neighbor prerequisites
    add_rule(ImprovementRule{ImprovementType::Road}
        .costs(Production{30.0})
        .build_time(1));
    
    add_rule(ImprovementRule{ImprovementType::Railroad}
        .requires(TechPrereq{101}) // Railroad technology
        .requires(NeighborPrereq{ImprovementType::Road, 2})
        .costs(Production{150.0})
        .build_time(5));
    
    add_rule(ImprovementRule{ImprovementType::Fort}
        .requires(TechPrereq{102}) // Military engineering
        .costs(Production{200.0})
        .build_time(6));
}

std::vector<ImprovementType> RuleRegistry::available_improvements(
    const Tile& tile, const TileMap& map) const {
    
    std::vector<ImprovementType> available;
    available.reserve(static_cast<std::size_t>(ImprovementType::Fort) + 1);
    
    for (const auto& [rule_type, rule] : rules_) {
        if (rule.can_build(tile, map)) {
            available.push_back(rule_type);
        }
    }
    
    return available;
}

RuleRegistry& get_improvement_rules() {
    static RuleRegistry registry;
    return registry;
}

} // namespace Manifest::World::Tiles

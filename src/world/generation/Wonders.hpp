#pragma once

#include "../tiles/Map.hpp"
#include "../../core/math/Vector.hpp"
#include <vector>
#include <string_view>
#include <random>
#include <unordered_set>

namespace Manifest::World::Generation {

using namespace Tiles;
using namespace Core::Math;
using namespace Core::Types;

enum class WonderType : std::uint8_t {
    Mountain,    // Everest, K2
    Volcano,     // Mount Vesuvius, Yellowstone
    Canyon,      // Grand Canyon, Antelope Canyon
    Falls,       // Niagara Falls, Victoria Falls
    Geyser,      // Old Faithful, Geysir
    Formation,   // Giant's Causeway, Uluru
    Crater,      // Meteor Crater, Ngorongoro
    Lake,        // Lake Titicaca, Dead Sea
    Desert,      // Sahara features, Atacama
    Ice,         // Glaciers, Ice caves
    Forest,      // Giant Sequoias, Amazon features
    Reef,        // Great Barrier Reef, Coral atolls
    Hot_Springs, // Yellowstone, Pamukkale
    Salt_Flats,  // Salar de Uyuni, Bonneville
    Archipelago  // Galápagos, Hawaiian chain
};

struct WonderDescriptor {
    WonderType type;
    std::string_view name;
    std::string_view description;
    TerrainType required_terrain;
    float min_elevation;
    float max_elevation;
    float rarity;
    std::vector<Feature> required_features;
    std::vector<TerrainType> nearby_terrain;
    
    // Effects
    Production science_bonus{};
    Production culture_bonus{};
    Production faith_bonus{};
    Money tourism_bonus{};
    float happiness_bonus{0.0f};
};

class NaturalWonderSystem {
    std::vector<WonderDescriptor> wonder_types_;
    std::unordered_map<TileId, std::size_t> tile_to_wonder_;
    std::mt19937 rng_;
    
public:
    explicit NaturalWonderSystem(std::uint32_t seed = 42) : rng_{seed} {
        initialize_wonder_types();
    }
    
    void generate_wonders(TileMap& map, std::size_t target_count = 12) {
        tile_to_wonder_.clear();
        
        // Find suitable locations for each wonder type
        for (std::size_t type_idx = 0; type_idx < wonder_types_.size() && 
             tile_to_wonder_.size() < target_count; ++type_idx) {
            
            const auto& wonder_type = wonder_types_[type_idx];
            
            // Skip if rarity check fails
            std::uniform_real_distribution<float> rarity_dist(0.0f, 1.0f);
            if (rarity_dist(rng_) > wonder_type.rarity) continue;
            
            auto candidates = find_wonder_locations(map, wonder_type);
            if (candidates.empty()) continue;
            
            // Select random candidate
            std::uniform_int_distribution<std::size_t> candidate_dist(0, candidates.size() - 1);
            TileId selected = candidates[candidate_dist(rng_)];
            
            // Place wonder
            if (place_wonder(map, selected, type_idx)) {
                tile_to_wonder_[selected] = type_idx;
            }
        }
    }
    
    const std::vector<WonderDescriptor>& wonder_types() const noexcept { 
        return wonder_types_; 
    }
    
    std::optional<std::size_t> get_wonder_at_tile(TileId tile_id) const {
        auto it = tile_to_wonder_.find(tile_id);
        return it != tile_to_wonder_.end() ? 
               std::optional<std::size_t>{it->second} : std::nullopt;
    }
    
private:
    void initialize_wonder_types() {
        wonder_types_ = {
            // Mountain Wonders
            {
                .type = WonderType::Mountain,
                .name = "Sacred Peak",
                .description = "A towering mountain that reaches toward the heavens",
                .required_terrain = TerrainType::Mountains,
                .min_elevation = 0.85f,
                .max_elevation = 1.0f,
                .rarity = 0.3f,
                .science_bonus = Production{3.0},
                .culture_bonus = Production{2.0},
                .faith_bonus = Production{4.0}
            },
            
            // Volcanic Wonders
            {
                .type = WonderType::Volcano,
                .name = "Active Volcano",
                .description = "A magnificent volcanic peak with geothermal activity",
                .required_terrain = TerrainType::Mountains,
                .min_elevation = 0.6f,
                .max_elevation = 0.9f,
                .rarity = 0.4f,
                .required_features = {Feature::Volcano},
                .science_bonus = Production{4.0},
                .culture_bonus = Production{1.0}
            },
            
            // Canyon Wonders
            {
                .type = WonderType::Canyon,
                .name = "Grand Canyon",
                .description = "A spectacular gorge carved by ancient waters",
                .required_terrain = TerrainType::Desert,
                .min_elevation = 0.3f,
                .max_elevation = 0.7f,
                .rarity = 0.2f,
                .culture_bonus = Production{5.0},
                .tourism_bonus = Money{3.0}
            },
            
            // Waterfall Wonders
            {
                .type = WonderType::Falls,
                .name = "Majestic Falls",
                .description = "Thundering waterfalls cascade from great heights",
                .required_terrain = TerrainType::Hills,
                .min_elevation = 0.4f,
                .max_elevation = 0.8f,
                .rarity = 0.35f,
                .required_features = {Feature::River},
                .culture_bonus = Production{3.0},
                .tourism_bonus = Money{4.0},
                .happiness_bonus = 0.1f
            },
            
            // Geyser Fields
            {
                .type = WonderType::Geyser,
                .name = "Geyser Fields",
                .description = "Natural hot springs that erupt with clockwork precision",
                .required_terrain = TerrainType::Plains,
                .min_elevation = 0.5f,
                .max_elevation = 0.8f,
                .rarity = 0.25f,
                .science_bonus = Production{4.0},
                .tourism_bonus = Money{2.0}
            },
            
            // Rock Formations
            {
                .type = WonderType::Formation,
                .name = "Stone Pillars",
                .description = "Mysterious rock formations shaped by wind and time",
                .required_terrain = TerrainType::Desert,
                .min_elevation = 0.2f,
                .max_elevation = 0.6f,
                .rarity = 0.3f,
                .culture_bonus = Production{4.0},
                .faith_bonus = Production{2.0}
            },
            
            // Impact Crater
            {
                .type = WonderType::Crater,
                .name = "Ancient Crater",
                .description = "A perfectly preserved impact crater from space",
                .required_terrain = TerrainType::Plains,
                .min_elevation = 0.1f,
                .max_elevation = 0.4f,
                .rarity = 0.15f,
                .science_bonus = Production{6.0}
            },
            
            // Sacred Lake
            {
                .type = WonderType::Lake,
                .name = "Sacred Lake",
                .description = "A pristine lake with crystal-clear waters",
                .required_terrain = TerrainType::Lake,
                .min_elevation = 0.3f,
                .max_elevation = 0.8f,
                .rarity = 0.4f,
                .faith_bonus = Production{3.0},
                .tourism_bonus = Money{2.0},
                .happiness_bonus = 0.15f
            },
            
            // Desert Wonder
            {
                .type = WonderType::Desert,
                .name = "Endless Dunes",
                .description = "Vast sand dunes that shift with the desert winds",
                .required_terrain = TerrainType::Desert,
                .min_elevation = 0.1f,
                .max_elevation = 0.5f,
                .rarity = 0.3f,
                .culture_bonus = Production{3.0},
                .faith_bonus = Production{2.0}
            },
            
            // Glacier
            {
                .type = WonderType::Ice,
                .name = "Ancient Glacier",
                .description = "A massive river of ice flowing through mountain valleys",
                .required_terrain = TerrainType::Snow,
                .min_elevation = 0.7f,
                .max_elevation = 1.0f,
                .rarity = 0.2f,
                .science_bonus = Production{3.0},
                .tourism_bonus = Money{3.0}
            },
            
            // Ancient Forest
            {
                .type = WonderType::Forest,
                .name = "Ancient Grove",
                .description = "Ancient trees that have stood for millennia",
                .required_terrain = TerrainType::Forest,
                .min_elevation = 0.2f,
                .max_elevation = 0.7f,
                .rarity = 0.35f,
                .culture_bonus = Production{3.0},
                .faith_bonus = Production{4.0},
                .happiness_bonus = 0.1f
            },
            
            // Coral Reef
            {
                .type = WonderType::Reef,
                .name = "Living Reef",
                .description = "A vibrant coral ecosystem teeming with life",
                .required_terrain = TerrainType::Coast,
                .min_elevation = 0.0f,
                .max_elevation = 0.3f,
                .rarity = 0.3f,
                .required_features = {Feature::Reef},
                .science_bonus = Production{4.0},
                .tourism_bonus = Money{4.0}
            }
        };
    }
    
    std::vector<TileId> find_wonder_locations(const TileMap& map, 
                                              const WonderDescriptor& wonder) {
        std::vector<TileId> candidates;
        
        map.for_each_tile([&](const Tile& tile) {
            if (!is_suitable_for_wonder(tile, wonder)) return;
            if (is_too_close_to_existing_wonder(map, tile.id())) return;
            
            candidates.push_back(tile.id());
        });
        
        return candidates;
    }
    
    bool is_suitable_for_wonder(const Tile& tile, const WonderDescriptor& wonder) {
        // Check terrain type
        if (tile.terrain() != wonder.required_terrain) {
            return false;
        }
        
        // Check elevation
        float elevation = tile.elevation() / 255.0f;
        if (elevation < wonder.min_elevation || elevation > wonder.max_elevation) {
            return false;
        }
        
        // Check required features
        for (Feature required_feature : wonder.required_features) {
            if (!tile.has_feature(required_feature)) {
                return false;
            }
        }
        
        // Additional terrain-specific checks
        switch (wonder.type) {
            case WonderType::Falls:
                // Waterfalls need significant elevation difference nearby
                return has_elevation_variation_nearby(tile);
                
            case WonderType::Geyser:
                // Geysers need volcanic activity or hot springs nearby
                return has_geothermal_activity_nearby(tile);
                
            case WonderType::Crater:
                // Craters should be relatively isolated
                return is_relatively_flat_area(tile);
                
            default:
                return true;
        }
    }
    
    bool has_elevation_variation_nearby(const Tile& tile) {
        std::uint8_t tile_elevation = tile.elevation();
        
        for (const TileId& neighbor_id : tile.neighbors()) {
            if (!neighbor_id.is_valid()) continue;
            
            // This would require access to the map, simplified for now
            // In practice, would check if any neighbor has significantly different elevation
        }
        
        return true; // Simplified
    }
    
    bool has_geothermal_activity_nearby(const Tile& tile) {
        // Check for volcanic features or hot springs
        return tile.has_feature(Feature::Volcano);
    }
    
    bool is_relatively_flat_area(const Tile& tile) {
        // Craters should be in relatively flat areas
        float elevation = tile.elevation() / 255.0f;
        return elevation >= 0.1f && elevation <= 0.4f;
    }
    
    bool is_too_close_to_existing_wonder(const TileMap& map, TileId tile_id) {
        const std::int32_t MIN_WONDER_DISTANCE = 10;
        
        const Tile* tile = map.get_tile(tile_id);
        if (!tile) return true;
        
        // Check distance to all existing wonders
        for (const auto& [wonder_tile_id, wonder_type_idx] : tile_to_wonder_) {
            const Tile* wonder_tile = map.get_tile(wonder_tile_id);
            if (!wonder_tile) continue;
            
            std::int32_t distance = TileMap::distance(tile->coordinate(), 
                                                      wonder_tile->coordinate());
            if (distance < MIN_WONDER_DISTANCE) {
                return true;
            }
        }
        
        return false;
    }
    
    bool place_wonder(TileMap& map, TileId tile_id, std::size_t wonder_type_idx) {
        Tile* tile = map.get_tile(tile_id);
        if (!tile) return false;
        
        const auto& wonder = wonder_types_[wonder_type_idx];
        
        // Add natural wonder feature
        tile->add_feature(Feature::NaturalWonder);
        
        // Apply wonder-specific modifications
        switch (wonder.type) {
            case WonderType::Volcano:
                tile->add_feature(Feature::Volcano);
                break;
                
            case WonderType::Geyser:
            case WonderType::Hot_Springs:
                // Could add hot springs feature if it existed
                break;
                
            case WonderType::Reef:
                tile->add_feature(Feature::Reef);
                break;
                
            case WonderType::Crater:
                // Modify terrain to show impact
                tile->set_terrain(TerrainType::Plains);
                break;
                
            default:
                break;
        }
        
        // Enhance surrounding tiles
        enhance_wonder_surroundings(map, tile_id, wonder);
        
        return true;
    }
    
    void enhance_wonder_surroundings(TileMap& map, TileId wonder_tile_id, 
                                     const WonderDescriptor& wonder) {
        const Tile* wonder_tile = map.get_tile(wonder_tile_id);
        if (!wonder_tile) return;
        
        // Get tiles in radius around wonder
        auto nearby_tiles = map.get_tiles_in_radius(wonder_tile->coordinate(), 2);
        
        for (TileId nearby_id : nearby_tiles) {
            Tile* nearby = map.get_tile(nearby_id);
            if (!nearby || nearby_id == wonder_tile_id) continue;
            
            // Enhance fertility and appeal
            std::uint8_t current_fertility = nearby->fertility();
            nearby->set_fertility(std::min(255, static_cast<int>(current_fertility + 20)));
            
            // Some wonders create specific terrain features
            switch (wonder.type) {
                case WonderType::Volcano:
                    // Volcanic soil is fertile
                    nearby->set_fertility(std::min(255, static_cast<int>(current_fertility + 40)));
                    break;
                    
                case WonderType::Falls:
                case WonderType::Lake:
                    // Water features increase nearby fertility
                    if (nearby->is_land()) {
                        nearby->set_fertility(std::min(255, static_cast<int>(current_fertility + 30)));
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
};

} // namespace Manifest::World::Generation

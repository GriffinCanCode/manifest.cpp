#pragma once

#include "Generator.hpp"
#include "../generation/Plates.hpp"
#include "../generation/Rivers.hpp"
#include "../generation/Wonders.hpp"

namespace Manifest::World::Terrain {

using namespace Generation;

class EnhancedTerrainGenerator {
    TerrainGenerator base_generator_;
    PlateSystem plate_system_;
    RiverSystem river_system_;
    NaturalWonderSystem wonder_system_;
    
public:
    explicit EnhancedTerrainGenerator(const GenerationParams& params = {})
        : base_generator_{params}
        , plate_system_{params.seed + 100}
        , river_system_{params.seed + 200}
        , wonder_system_{params.seed + 300} {}
    
    template<MapLike MapType>
    void generate_world(MapType& map) {
        // Phase 1: Generate tectonic plates
        plate_system_.generate_plates(map, 12);
        
        // Phase 2: Simulate plate tectonics to create base elevation
        for (int i = 0; i < 50; ++i) {
            plate_system_.simulate_tectonics(map, 0.1f);
        }
        
        // Phase 3: Generate base terrain using enhanced noise
        base_generator_.generate_world(map);
        
        // Phase 4: Generate river systems
        river_system_.generate_rivers(map, 25);
        
        // Phase 5: Place natural wonders
        wonder_system_.generate_wonders(map, 15);
        
        // Phase 6: Final terrain refinement
        refine_terrain(map);
        
        // Phase 7: Update all neighbor relationships
        update_all_neighbors(map);
    }
    
    // Accessors for subsystems
    const PlateSystem& plates() const noexcept { return plate_system_; }
    const RiverSystem& rivers() const noexcept { return river_system_; }
    const NaturalWonderSystem& wonders() const noexcept { return wonder_system_; }
    
private:
    template<MapLike MapType>
    void refine_terrain(MapType& map) {
        // Apply climate-based vegetation
        apply_vegetation(map);
        
        // Create coastal features
        create_coastal_features(map);
        
        // Add climate-specific features
        add_climate_features(map);
    }
    
    template<MapLike MapType>
    void apply_vegetation(MapType& map) {
        map.for_each_tile([](Tile& tile) {
            if (tile.is_water()) return;
            
            float temperature = tile.temperature() / 255.0f;
            float rainfall = tile.rainfall() / 255.0f;
            float elevation = tile.elevation() / 255.0f;
            
            // Add forest features based on climate
            if (temperature > 0.6f && rainfall > 0.7f && elevation < 0.7f) {
                if (temperature > 0.8f) {
                    tile.add_feature(Feature::Jungle);
                    if (tile.terrain() == TerrainType::Plains || 
                        tile.terrain() == TerrainType::Grassland) {
                        tile.set_terrain(TerrainType::Jungle);
                    }
                } else {
                    tile.add_feature(Feature::Forest);
                    if (tile.terrain() == TerrainType::Plains || 
                        tile.terrain() == TerrainType::Grassland) {
                        tile.set_terrain(TerrainType::Forest);
                    }
                }
            }
            
            // Add marsh in low, wet areas
            if (elevation < 0.2f && rainfall > 0.6f && !tile.has_feature(Feature::River)) {
                tile.add_feature(Feature::Marsh);
                tile.set_terrain(TerrainType::Marsh);
            }
            
            // Add oasis in desert
            if (tile.terrain() == TerrainType::Desert && rainfall > 0.4f) {
                tile.add_feature(Feature::Oasis);
            }
        });
    }
    
    template<MapLike MapType>
    void create_coastal_features(MapType& map) {
        map.for_each_tile([&map](Tile& tile) {
            if (tile.terrain() != TerrainType::Coast) return;
            
            // Check for reef formation conditions
            float temperature = tile.temperature() / 255.0f;
            if (temperature > 0.7f) { // Warm water
                // Check if in tropical zone and has good conditions
                bool has_shallow_water = true; // Simplified
                if (has_shallow_water) {
                    tile.add_feature(Feature::Reef);
                }
            }
            
            // Create ice in very cold coastal areas
            if (temperature < 0.1f) {
                tile.add_feature(Feature::Ice);
            }
        });
    }
    
    template<MapLike MapType>
    void add_climate_features(MapType& map) {
        map.for_each_tile([](Tile& tile) {
            float temperature = tile.temperature() / 255.0f;
            float elevation = tile.elevation() / 255.0f;
            
            // Add ice features to high, cold areas
            if (temperature < 0.2f && elevation > 0.8f) {
                tile.add_feature(Feature::Ice);
                if (elevation > 0.9f) {
                    tile.set_terrain(TerrainType::Snow);
                }
            }
            
            // Add volcanic features to suitable areas (already handled by plates)
            // This could be enhanced with more sophisticated volcanic placement
        });
    }
    
    template<MapLike MapType>
    void update_all_neighbors(MapType& map) {
        map.for_each_tile([&map](const Tile& tile) {
            map.update_neighbors(tile.id());
        });
    }
};

} // namespace Manifest::World::Terrain

#pragma once

#include "Plates.hpp"
#include "Rivers.hpp"
#include "Wonders.hpp"
#include "Rainfall.hpp"
#include "Biomes.hpp"
#include "../terrain/Generator.hpp"
#include "../../core/log/Log.hpp"

namespace Manifest::World::Generation {

using namespace Terrain;

class EnhancedWorldGenerator {
    TerrainGenerator base_generator_;
    PlateSystem plate_system_;
    RiverSystem river_system_;
    NaturalWonderSystem wonder_system_;
    Rainfall rainfall_system_;

public:
    explicit EnhancedWorldGenerator(const GenerationParams& params = {})
        : base_generator_{params},
          plate_system_{params.seed + 100},
          river_system_{params.seed + 200},
          wonder_system_{params.seed + 300},
          rainfall_system_{params.seed + 400, static_cast<float>(params.map_radius)} {}

    template <typename MapType>
    void generate_world(MapType& map) {
        MODULE_LOGGER("EnhancedWorldGen");
        logger_->info("Starting enhanced world generation with {} tiles", map.size());
        auto start_time = std::chrono::high_resolution_clock::now();

        // Phase 1: Generate tectonic plates for geological foundation
        logger_->info("Phase 1: Generating {} tectonic plates", 12);
        auto phase_start = std::chrono::high_resolution_clock::now();
        plate_system_.generate_plates(map, 12);
        auto phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Tectonic plates generated in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 2: Simulate plate tectonics to create base elevation
        logger_->info("Phase 2: Simulating plate tectonics (50 iterations)");
        phase_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 50; ++i) {
            plate_system_.simulate_tectonics(map, 0.1F);
            if ((i + 1) % 10 == 0) {
                logger_->debug("Tectonic simulation progress: {}/50 iterations", i + 1);
            }
        }
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Tectonic simulation completed in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 3: Generate base terrain using enhanced noise
        logger_->info("Phase 3: Generating base terrain with enhanced noise");
        phase_start = std::chrono::high_resolution_clock::now();
        base_generator_.generate_world(map);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Base terrain generated in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 4: Sophisticated rainfall simulation
        logger_->info("Phase 4: Running atmospheric rainfall simulation (15 iterations)");
        phase_start = std::chrono::high_resolution_clock::now();
        rainfall_system_.simulate(map, 15); // More iterations for better realism
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Rainfall simulation completed in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 5: Advanced biome classification based on sophisticated climate data
        logger_->info("Phase 5: Classifying biomes based on climate data");
        phase_start = std::chrono::high_resolution_clock::now();
        Biomes::classify_biomes(map);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Biome classification completed in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 6: Generate river systems (benefits from realistic rainfall patterns)
        logger_->info("Phase 6: Generating {} river systems", 25);
        phase_start = std::chrono::high_resolution_clock::now();
        river_system_.generate_rivers(map, 25);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("River systems generated in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 7: Place natural wonders
        logger_->info("Phase 7: Placing {} natural wonders", 15);
        phase_start = std::chrono::high_resolution_clock::now();
        wonder_system_.generate_wonders(map, 15);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Natural wonders placed in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 8: Final terrain refinement
        logger_->info("Phase 8: Performing final terrain refinement");
        phase_start = std::chrono::high_resolution_clock::now();
        refine_terrain(map);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Terrain refinement completed in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        // Phase 9: Update all neighbor relationships
        logger_->info("Phase 9: Updating neighbor relationships");
        phase_start = std::chrono::high_resolution_clock::now();
        update_all_neighbors(map);
        phase_end = std::chrono::high_resolution_clock::now();
        logger_->debug("Neighbor relationships updated in {}ms", 
                      std::chrono::duration_cast<std::chrono::milliseconds>(phase_end - phase_start).count());

        auto total_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_time - start_time);
        logger_->info("Enhanced world generation completed in {}ms", duration.count());
    }

    // Accessors for subsystems
    const PlateSystem& plates() const noexcept { return plate_system_; }
    const RiverSystem& rivers() const noexcept { return river_system_; }
    const NaturalWonderSystem& wonders() const noexcept { return wonder_system_; }
    const Rainfall& rainfall() const noexcept { return rainfall_system_; }

private:
    template <typename MapType>
    void refine_terrain(MapType& map) {
        // Enhanced terrain refinement that considers sophisticated climate data
        enhance_coastal_transitions(map);
        create_climate_features(map);
        add_biome_transitions(map);
    }

    template <typename MapType>
    void enhance_coastal_transitions(MapType& map) {
        map.for_each_tile([&map](Tile& tile) {
            if (tile.terrain() != Tiles::TerrainType::Coast) {
                return;
            }

            // Enhanced coastal analysis considering wind patterns and temperature
            float temperature = static_cast<float>(tile.temperature()) / 255.0F;
            float rainfall = static_cast<float>(tile.rainfall()) / 255.0F;
            
            // Create reef formation in warm, low-rainfall coastal areas
            if (temperature > 0.7F && rainfall < 0.4F) {
                bool has_shallow_water = true; // Simplified check
                if (has_shallow_water) {
                    tile.add_feature(Tiles::Feature::Reef);
                }
            }

            // Create ice in very cold coastal areas
            if (temperature < 0.15F) {
                tile.add_feature(Tiles::Feature::Ice);
            }
        });
    }

    template <typename MapType>
    void create_climate_features(MapType& map) {
        map.for_each_tile([](Tile& tile) {
            float temperature = static_cast<float>(tile.temperature()) / 255.0F;
            float elevation = static_cast<float>(tile.elevation()) / 255.0F;
            float rainfall = static_cast<float>(tile.rainfall()) / 255.0F;

            // Add ice features to high, cold areas
            if (temperature < 0.25F && elevation > 0.75F) {
                tile.add_feature(Tiles::Feature::Ice);
                if (elevation > 0.9F && temperature < 0.15F) {
                    tile.set_terrain(Tiles::TerrainType::Snow);
                }
            }

            // Create floodplains in low-lying, high-rainfall areas near rivers
            if (elevation < 0.2F && rainfall > 0.8F) {
                // Check if near river (simplified)
                bool near_river = tile.has_feature(Tiles::Feature::River);
                if (near_river) {
                    tile.add_feature(Tiles::Feature::Floodplains);
                }
            }

            // Create oases in desert areas with underground water (simplified heuristic)
            if (tile.terrain() == Tiles::TerrainType::Desert && 
                elevation > 0.3F && elevation < 0.5F && rainfall > 0.1F) {
                // Rare oasis formation
                tile.add_feature(Tiles::Feature::Oasis);
            }
        });
    }

    template <typename MapType>
    void add_biome_transitions(MapType& map) {
        // Create more realistic biome transitions by smoothing harsh boundaries
        map.for_each_tile([&map](Tile& tile) {
            analyze_biome_neighbors(tile, map);
        });
    }

    template <typename MapType>
    void analyze_biome_neighbors(Tile& tile, const MapType& map) {
        // Count different terrain types in neighborhood
        std::array<int, static_cast<int>(Tiles::TerrainType::River) + 1> terrain_counts{};
        int valid_neighbors = 0;

        for (std::size_t i = 0; i < 6; ++i) {
            auto neighbor_dir = static_cast<Core::Types::Direction>(i);
            Core::Types::TileId neighbor_id = tile.neighbor(neighbor_dir);
            
            if (neighbor_id.is_valid()) {
                const Tile* neighbor = map.get_tile(neighbor_id);
                if (neighbor != nullptr) {
                    terrain_counts[static_cast<int>(neighbor->terrain())]++;
                    valid_neighbors++;
                }
            }
        }

        // Apply transition smoothing if there's terrain diversity
        if (valid_neighbors >= 4) {
            apply_transition_smoothing(tile, terrain_counts, valid_neighbors);
        }
    }

    void apply_transition_smoothing(Tile& tile, 
                                  const std::array<int, static_cast<int>(Tiles::TerrainType::River) + 1>& terrain_counts,
                                  int total_neighbors) {
        // Find the most common neighbor terrain
        int max_count = 0;
        Tiles::TerrainType dominant_terrain = tile.terrain();
        
        for (std::size_t i = 0; i < terrain_counts.size(); ++i) {
            if (terrain_counts[i] > max_count) {
                max_count = terrain_counts[i];
                dominant_terrain = static_cast<Tiles::TerrainType>(i);
            }
        }

        // If dominated by different terrain and conditions are suitable, transition
        if (max_count >= total_neighbors / 2 && dominant_terrain != tile.terrain()) {
            if (is_terrain_transition_valid(tile.terrain(), dominant_terrain)) {
                tile.set_terrain(dominant_terrain);
            }
        }
    }

    bool is_terrain_transition_valid(Tiles::TerrainType current, Tiles::TerrainType target) const {
        // Define valid terrain transitions to prevent unrealistic changes
        using TerrainType = Tiles::TerrainType;
        
        // Mountains and oceans should be stable
        if (current == TerrainType::Mountains || current == TerrainType::Ocean ||
            target == TerrainType::Mountains || target == TerrainType::Ocean) {
            return false;
        }
        
        // Allow reasonable transitions
        switch (current) {
            case TerrainType::Desert:
                return target == TerrainType::Plains || target == TerrainType::Hills;
            case TerrainType::Plains:
                return target != TerrainType::Snow && target != TerrainType::Jungle;
            case TerrainType::Forest:
                return target == TerrainType::Plains || target == TerrainType::Hills || 
                       target == TerrainType::Grassland;
            default:
                return true; // Most other transitions are acceptable
        }
    }

    template <typename MapType>
    void update_all_neighbors(MapType& map) {
        map.for_each_tile([&map](const Tile& tile) { 
            map.update_neighbors(tile.id()); 
        });
    }
};

} // namespace Manifest::World::Generation

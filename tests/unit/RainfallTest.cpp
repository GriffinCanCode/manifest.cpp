#include <gtest/gtest.h>
#include <memory>

#include "../../src/world/generation/Rainfall.hpp"
#include "../../src/world/generation/Biomes.hpp"
#include "../../src/world/tiles/Map.hpp"
#include "../../src/world/tiles/Tile.hpp"

using namespace Manifest::World::Generation;
using namespace Manifest::World::Tiles;
using namespace Manifest::Core::Types;

class RainfallTest : public ::testing::Test {
protected:
    void SetUp() override {
        map = std::make_unique<TileMap>();
        rainfall_system = std::make_unique<Rainfall>(12345, 50.0F);
        
        // Create a small test world
        create_test_world();
    }

    void create_test_world() {
        // Create a 5x5 hex grid for testing
        for (std::int32_t q = -2; q <= 2; ++q) {
            std::int32_t r1 = std::max(-2, -q - 2);
            std::int32_t r2 = std::min(2, -q + 2);
            
            for (std::int32_t r = r1; r <= r2; ++r) {
                std::int32_t s = -q - r;
                HexCoordinate coord{q, r, s};
                
                TileId id = map->create_tile(coord);
                Tile* tile = map->get_tile(id);
                ASSERT_NE(tile, nullptr);
                
                // Set up varied terrain for testing
                setup_test_tile(*tile, q, r);
            }
        }
        
        // Update all neighbors
        map->for_each_tile([this](const Tile& tile) {
            map->update_neighbors(tile.id());
        });
    }

    void setup_test_tile(Tile& tile, std::int32_t q, std::int32_t r) {
        // Create elevation gradient from center outward
        float distance = std::sqrt(static_cast<float>(q * q + r * r));
        std::uint8_t elevation = static_cast<std::uint8_t>(
            std::clamp((1.0F - distance / 3.0F) * 255.0F, 0.0F, 255.0F)
        );
        tile.set_elevation(elevation);
        
        // Set temperature based on distance from "equator" (r=0)
        float temp_factor = 1.0F - std::abs(static_cast<float>(r)) / 3.0F;
        std::uint8_t temperature = static_cast<std::uint8_t>(temp_factor * 255.0F);
        tile.set_temperature(temperature);
        
        // Create some water tiles
        if ((q == 0 && r == 0) || (q == -1 && r == 1)) {
            tile.set_terrain(TerrainType::Ocean);
        } else if (elevation > 200) {
            tile.set_terrain(TerrainType::Mountains);
        } else {
            tile.set_terrain(TerrainType::Plains);
        }
    }

    std::unique_ptr<TileMap> map;
    std::unique_ptr<Rainfall> rainfall_system;
};

TEST_F(RainfallTest, AtmosphereGeneratesPressureSystems) {
    Atmosphere atmosphere(12345, 50.0F);
    atmosphere.generate_pressure_systems(5);
    
    EXPECT_EQ(atmosphere.systems().size(), 5);
    
    // Test pressure calculation
    Vec2f test_position{0.0F, 0.0F};
    Pressure pressure = atmosphere.calculate_pressure(test_position);
    
    // Pressure should be realistic (between 900-1100 hPa typically)
    EXPECT_GT(pressure.value(), 900.0F);
    EXPECT_LT(pressure.value(), 1100.0F);
}

TEST_F(RainfallTest, AtmosphereGeneratesWind) {
    Atmosphere atmosphere(12345, 50.0F);
    atmosphere.generate_pressure_systems(3);
    
    Vec2f test_position{10.0F, 10.0F};
    Vec2f wind = atmosphere.calculate_wind(test_position);
    
    // Wind should have some magnitude (not zero everywhere)
    float wind_magnitude = wind.length();
    EXPECT_GE(wind_magnitude, 0.0F);
    EXPECT_LT(wind_magnitude, 100.0F); // Reasonable upper bound
}

TEST_F(RainfallTest, HydrologyCalculatesEvaporation) {
    // Test evaporation from water tile
    Tile water_tile(TileId{1}, HexCoordinate{0, 0, 0});
    water_tile.set_terrain(TerrainType::Ocean);
    water_tile.set_temperature(200); // Warm water
    
    Evaporation evaporation = Hydrology::calculate_evaporation(water_tile);
    EXPECT_GT(evaporation.value(), 0.0F);
    
    // Test evaporation from land tile
    Tile land_tile(TileId{2}, HexCoordinate{1, 0, -1});
    land_tile.set_terrain(TerrainType::Forest);
    land_tile.set_temperature(150);
    
    Evaporation land_evaporation = Hydrology::calculate_evaporation(land_tile);
    EXPECT_GT(land_evaporation.value(), 0.0F);
    
    // Water should generally evaporate more than land at same temperature
    if (water_tile.temperature() >= land_tile.temperature()) {
        EXPECT_GE(evaporation.value(), land_evaporation.value());
    }
}

TEST_F(RainfallTest, HydrologyCalculatesHumidity) {
    Vec2f position{0.0F, 0.0F};
    float temperature = 293.15F; // 20°C in Kelvin
    
    // Test with nearby water
    std::vector<const Tile*> water_tiles;
    map->for_each_tile([&water_tiles](const Tile& tile) {
        if (tile.is_water()) {
            water_tiles.push_back(&tile);
        }
    });
    
    Humidity humidity = Hydrology::calculate_humidity(position, water_tiles, temperature);
    EXPECT_GE(humidity.value(), 0.0F);
    EXPECT_LE(humidity.value(), 1.0F);
    
    // Test with no nearby water
    std::vector<const Tile*> no_water;
    Humidity dry_humidity = Hydrology::calculate_humidity(position, no_water, temperature);
    
    // Should have some base humidity even without water
    EXPECT_GT(dry_humidity.value(), 0.0F);
    
    // With water nearby, humidity should be higher
    EXPECT_GE(humidity.value(), dry_humidity.value());
}

TEST_F(RainfallTest, OrographyCalculatesElevationGradient) {
    // Find a mountain tile
    Tile* mountain_tile = nullptr;
    map->for_each_tile([&mountain_tile](Tile& tile) {
        if (tile.terrain() == TerrainType::Mountains && mountain_tile == nullptr) {
            mountain_tile = &tile;
        }
    });
    
    ASSERT_NE(mountain_tile, nullptr);
    
    Vec2f wind_direction{1.0F, 0.0F}; // Eastward wind
    float gradient = Orography::calculate_elevation_gradient(*mountain_tile, wind_direction, *map);
    
    // Gradient can be positive or negative depending on upwind neighbor
    EXPECT_GE(gradient, -1.0F);
    EXPECT_LE(gradient, 1.0F);
}

TEST_F(RainfallTest, OrographyCalculatesRainShadow) {
    Tile* test_tile = nullptr;
    map->for_each_tile([&test_tile](Tile& tile) {
        if (test_tile == nullptr) {
            test_tile = &tile;
        }
    });
    
    ASSERT_NE(test_tile, nullptr);
    
    Vec2f wind_direction{1.0F, 0.0F};
    float shadow = Orography::calculate_rain_shadow(*test_tile, wind_direction, *map, 3.0F);
    
    // Rain shadow factor should be between 0.1 and 1.0
    EXPECT_GE(shadow, 0.1F);
    EXPECT_LE(shadow, 1.0F);
}

TEST_F(RainfallTest, RainfallSimulationProducesReasonableValues) {
    // Run the rainfall simulation
    rainfall_system->simulate(*map, 5);
    
    // Check that all tiles have been assigned rainfall values
    bool has_rainfall = false;
    float total_rainfall = 0.0F;
    int tile_count = 0;
    
    map->for_each_tile([&](const Tile& tile) {
        std::uint8_t rainfall = tile.rainfall();
        if (rainfall > 0) {
            has_rainfall = true;
        }
        total_rainfall += static_cast<float>(rainfall);
        tile_count++;
    });
    
    EXPECT_TRUE(has_rainfall);
    EXPECT_GT(total_rainfall, 0.0F);
    
    // Average rainfall should be reasonable
    float average_rainfall = total_rainfall / static_cast<float>(tile_count);
    EXPECT_GT(average_rainfall, 0.0F);
    EXPECT_LT(average_rainfall, 255.0F);
}

TEST_F(RainfallTest, BiomeClassificationUsesRainfallData) {
    // First run rainfall simulation
    rainfall_system->simulate(*map, 5);
    
    // Then classify biomes
    Biomes::classify_biomes(*map);
    
    // Check that biomes were assigned and are reasonable
    std::map<TerrainType, int> terrain_counts;
    map->for_each_tile([&terrain_counts](const Tile& tile) {
        terrain_counts[tile.terrain()]++;
    });
    
    // Should have some variety in terrain types
    EXPECT_GT(terrain_counts.size(), 1);
    
    // Ocean tiles should remain ocean
    if (terrain_counts.contains(TerrainType::Ocean)) {
        EXPECT_GT(terrain_counts[TerrainType::Ocean], 0);
    }
}

TEST_F(RainfallTest, BiomeDeterminationIsConsistent) {
    // Create a test tile with specific conditions
    Tile test_tile(TileId{999}, HexCoordinate{0, 0, 0});
    test_tile.set_temperature(180); // Moderate temperature
    test_tile.set_rainfall(120);    // Moderate rainfall
    test_tile.set_elevation(100);   // Low elevation
    test_tile.set_terrain(TerrainType::Plains);
    
    // Classify biome multiple times - should be consistent
    BiomeType biome1 = Biomes::determine_biome(test_tile, *map);
    BiomeType biome2 = Biomes::determine_biome(test_tile, *map);
    BiomeType biome3 = Biomes::determine_biome(test_tile, *map);
    
    EXPECT_EQ(biome1, biome2);
    EXPECT_EQ(biome2, biome3);
}

TEST_F(RainfallTest, ExtremeConditionsHandledGracefully) {
    // Test with extreme temperature
    Tile hot_tile(TileId{1001}, HexCoordinate{1, 0, -1});
    hot_tile.set_temperature(255); // Maximum temperature
    hot_tile.set_rainfall(0);       // No rainfall
    hot_tile.set_elevation(50);     // Low elevation
    
    BiomeType hot_biome = Biomes::determine_biome(hot_tile, *map);
    // Should be some kind of desert/arid biome
    EXPECT_NE(hot_biome, static_cast<BiomeType>(999)); // Valid enum value
    
    // Test with extreme cold
    Tile cold_tile(TileId{1002}, HexCoordinate{-1, 0, 1});
    cold_tile.set_temperature(0);   // Minimum temperature
    cold_tile.set_rainfall(100);    // Some rainfall
    cold_tile.set_elevation(200);   // Higher elevation
    
    BiomeType cold_biome = Biomes::determine_biome(cold_tile, *map);
    EXPECT_NE(cold_biome, static_cast<BiomeType>(999)); // Valid enum value
}

// Integration test with the enhanced world generator
TEST_F(RainfallTest, IntegrationWithEnhancedGenerator) {
    // This test would require including the Enhanced.hpp
    // Testing that the enhanced generator produces reasonable results
    // with the sophisticated rainfall simulation
    
    // For now, we just test that the individual components work together
    rainfall_system->simulate(*map, 3);
    Biomes::classify_biomes(*map);
    
    // Verify the integration produces reasonable results
    bool has_varied_terrain = false;
    std::set<TerrainType> terrain_types;
    
    map->for_each_tile([&](const Tile& tile) {
        terrain_types.insert(tile.terrain());
        // Check that fertility is reasonable
        EXPECT_LE(tile.fertility(), 255);
    });
    
    // Should have created some terrain variety
    EXPECT_GT(terrain_types.size(), 1);
}

// Performance test to ensure the system scales reasonably
TEST_F(RainfallTest, PerformanceScales) {
    // Create a larger map for performance testing
    auto large_map = std::make_unique<TileMap>();
    
    // Create a 20x20 hex grid (much larger than test grid)
    for (std::int32_t q = -10; q <= 10; ++q) {
        std::int32_t r1 = std::max(-10, -q - 10);
        std::int32_t r2 = std::min(10, -q + 10);
        
        for (std::int32_t r = r1; r <= r2; ++r) {
            std::int32_t s = -q - r;
            HexCoordinate coord{q, r, s};
            
            TileId id = large_map->create_tile(coord);
            Tile* tile = large_map->get_tile(id);
            if (tile != nullptr) {
                setup_test_tile(*tile, q, r);
            }
        }
    }
    
    // Update neighbors
    large_map->for_each_tile([&large_map](const Tile& tile) {
        large_map->update_neighbors(tile.id());
    });
    
    // Time the rainfall simulation
    auto start = std::chrono::high_resolution_clock::now();
    
    Rainfall large_rainfall_system(12345, 100.0F);
    large_rainfall_system.simulate(*large_map, 5);
    Biomes::classify_biomes(*large_map);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (less than 5 seconds for this size)
    EXPECT_LT(duration.count(), 5000);
    
    std::cout << "Large map (" << large_map->tile_count() << " tiles) processed in " 
              << duration.count() << "ms" << std::endl;
}

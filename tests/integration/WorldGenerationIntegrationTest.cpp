#include "../utils/TestUtils.hpp"
#include "../../src/world/tiles/Map.hpp"
#include "../../src/world/terrain/Generator.hpp"

using namespace Manifest::World::Tiles;
using namespace Manifest::World::Terrain;
using namespace Manifest::Test;

class WorldGenerationIntegrationTest : public DeterministicTest {};

TEST_F(WorldGenerationIntegrationTest, FullWorldGeneration) {
    TileMap map;
    GenerationParams params{
        .seed = 12345,
        .map_radius = 50,
        .sea_level = 0.3f
    };
    
    TerrainGenerator generator(params);
    generator.generate_world(map);
    
    // Verify basic constraints
    EXPECT_GT(map.tile_count(), 1000u);
    
    // Count terrain distribution
    std::unordered_map<TerrainType, int> terrain_counts;
    map.for_each_tile([&](const Tile& tile) {
        terrain_counts[tile.terrain()]++;
    });
    
    // Should have varied terrain types
    EXPECT_GE(terrain_counts.size(), 5u);
    
    // Should have some water and some land
    bool has_water = terrain_counts[TerrainType::Ocean] > 0 || terrain_counts[TerrainType::Coast] > 0;
    bool has_land = terrain_counts[TerrainType::Plains] > 0 || terrain_counts[TerrainType::Grassland] > 0;
    
    EXPECT_TRUE(has_water);
    EXPECT_TRUE(has_land);
}

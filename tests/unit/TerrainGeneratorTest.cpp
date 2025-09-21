#include "../utils/TestUtils.hpp"
#include "../../src/world/terrain/Generator.hpp"

using namespace Manifest::World::Terrain;
using namespace Manifest::World::Tiles;
using namespace Manifest::Test;

class TerrainGeneratorTest : public DeterministicTest {};

TEST_F(TerrainGeneratorTest, BasicGeneration) {
    TileMap map;
    GenerationParams params{.seed = 42, .map_radius = 5};
    TerrainGenerator generator(params);
    
    generator.generate_world(map);
    
    EXPECT_GT(map.tile_count(), 0u);
    
    // Verify all tiles have valid terrain
    map.for_each_tile([](const Tile& tile) {
        EXPECT_NE(tile.terrain(), static_cast<TerrainType>(255));
    });
}

// Placeholder - would be expanded with comprehensive generation tests

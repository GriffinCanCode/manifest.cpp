#include "../utils/TestUtils.hpp"
#include "../../src/world/tiles/Map.hpp"

using namespace Manifest::World::Tiles;
using namespace Manifest::Core::Types;
using namespace Manifest::Test;

class MapTest : public DeterministicTest {};

TEST_F(MapTest, BasicMapOperations) {
    TileMap map;
    
    EXPECT_EQ(map.tile_count(), 0u);
    
    HexCoordinate coord{0, 0, 0};
    TileId id = map.create_tile(coord);
    
    EXPECT_TRUE(id.is_valid());
    EXPECT_EQ(map.tile_count(), 1u);
    
    Tile* tile = map.get_tile(id);
    EXPECT_NE(tile, nullptr);
    EXPECT_EQ(tile->id(), id);
    EXPECT_EQ(tile->coordinate(), coord);
}

// Placeholder - would be expanded with comprehensive map tests

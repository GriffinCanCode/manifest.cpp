#include "../utils/TestUtils.hpp"
#include "../../src/world/tiles/Tile.hpp"

using namespace Manifest::World::Tiles;
using namespace Manifest::Core::Types;
using namespace Manifest::Test;

class TileTest : public DeterministicTest {};

TEST_F(TileTest, BasicConstruction) {
    TileId id{42};
    HexCoordinate coord{1, -1, 0};
    
    Tile tile{id, coord};
    
    EXPECT_EQ(tile.id(), id);
    EXPECT_EQ(tile.coordinate(), coord);
    EXPECT_EQ(tile.terrain(), TerrainType::Ocean); // Default
    EXPECT_EQ(tile.resource(), ResourceType::None);
    EXPECT_EQ(tile.improvement(), ImprovementType::None);
    EXPECT_TRUE(tile.is_passable());
}

TEST_F(TileTest, TerrainProperties) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Test water detection
    tile.set_terrain(TerrainType::Ocean);
    EXPECT_TRUE(tile.is_water());
    EXPECT_FALSE(tile.is_land());
    
    tile.set_terrain(TerrainType::Coast);
    EXPECT_TRUE(tile.is_water());
    
    tile.set_terrain(TerrainType::Lake);
    EXPECT_TRUE(tile.is_water());
    
    // Test land detection
    tile.set_terrain(TerrainType::Plains);
    EXPECT_FALSE(tile.is_water());
    EXPECT_TRUE(tile.is_land());
}

TEST_F(TileTest, Features) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    EXPECT_FALSE(tile.has_feature(Feature::River));
    EXPECT_FALSE(tile.has_feature(Feature::Forest));
    
    tile.add_feature(Feature::River);
    EXPECT_TRUE(tile.has_feature(Feature::River));
    EXPECT_FALSE(tile.has_feature(Feature::Forest));
    
    tile.add_feature(Feature::Forest);
    EXPECT_TRUE(tile.has_feature(Feature::River));
    EXPECT_TRUE(tile.has_feature(Feature::Forest));
    
    tile.remove_feature(Feature::River);
    EXPECT_FALSE(tile.has_feature(Feature::River));
    EXPECT_TRUE(tile.has_feature(Feature::Forest));
}

TEST_F(TileTest, MovementCost) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    tile.set_terrain(TerrainType::Plains);
    EXPECT_FLOAT_EQ(tile.movement_cost(), 1.0f);
    
    tile.set_terrain(TerrainType::Hills);
    EXPECT_FLOAT_EQ(tile.movement_cost(), 2.0f);
    
    tile.set_terrain(TerrainType::Mountains);
    EXPECT_FLOAT_EQ(tile.movement_cost(), 3.0f);
    
    tile.set_terrain(TerrainType::Forest);
    EXPECT_FLOAT_EQ(tile.movement_cost(), 2.0f);
}

TEST_F(TileTest, ImprovementCompatibility) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Test farm compatibility
    tile.set_terrain(TerrainType::Plains);
    EXPECT_TRUE(tile.can_build_improvement(ImprovementType::Farm));
    
    tile.set_terrain(TerrainType::Mountains);
    EXPECT_FALSE(tile.can_build_improvement(ImprovementType::Farm));
    
    // Test mine compatibility
    tile.set_terrain(TerrainType::Hills);
    EXPECT_TRUE(tile.can_build_improvement(ImprovementType::Mine));
    
    tile.set_terrain(TerrainType::Plains);
    EXPECT_FALSE(tile.can_build_improvement(ImprovementType::Mine));
    
    // Test fishing boats compatibility
    tile.set_terrain(TerrainType::Ocean);
    EXPECT_TRUE(tile.can_build_improvement(ImprovementType::FishingBoats));
    
    tile.set_terrain(TerrainType::Plains);
    EXPECT_FALSE(tile.can_build_improvement(ImprovementType::FishingBoats));
}

TEST_F(TileTest, YieldCalculation) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Test grassland yields
    tile.set_terrain(TerrainType::Grassland);
    EXPECT_DOUBLE_EQ(tile.food_yield().value(), 2.0);
    EXPECT_DOUBLE_EQ(tile.production_yield().value(), 0.0);
    
    // Test plains yields
    tile.set_terrain(TerrainType::Plains);
    EXPECT_DOUBLE_EQ(tile.food_yield().value(), 1.0);
    EXPECT_DOUBLE_EQ(tile.production_yield().value(), 1.0);
    
    // Test resource bonus
    tile.set_resource(ResourceType::Wheat);
    EXPECT_DOUBLE_EQ(tile.food_yield().value(), 2.0); // 1 + 1 bonus
    
    // Test improvement bonus
    tile.set_improvement(ImprovementType::Farm);
    EXPECT_DOUBLE_EQ(tile.food_yield().value(), 3.0); // 1 + 1 resource + 1 improvement
}

TEST_F(TileTest, NeighborManagement) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Initially all neighbors should be invalid
    for (int i = 0; i < 6; ++i) {
        Direction dir = static_cast<Direction>(i);
        EXPECT_EQ(tile.neighbor(dir), TileId::invalid());
    }
    
    // Set some neighbors
    TileId north_id{2};
    TileId south_id{3};
    
    tile.set_neighbor(Direction::North, north_id);
    tile.set_neighbor(Direction::South, south_id);
    
    EXPECT_EQ(tile.neighbor(Direction::North), north_id);
    EXPECT_EQ(tile.neighbor(Direction::South), south_id);
    EXPECT_EQ(tile.neighbor(Direction::Northeast), TileId::invalid());
}

TEST_F(TileTest, OwnershipAndControl) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Initially no owner or controlling city
    EXPECT_EQ(tile.owner(), NationId::invalid());
    EXPECT_EQ(tile.controlling_city(), CityId::invalid());
    
    NationId nation{100};
    CityId city{200};
    
    tile.set_owner(nation);
    tile.set_controlling_city(city);
    
    EXPECT_EQ(tile.owner(), nation);
    EXPECT_EQ(tile.controlling_city(), city);
}

TEST_F(TileTest, VisibilityAndExploration) {
    TileId id{1};
    HexCoordinate coord{0, 0, 0};
    Tile tile{id, coord};
    
    // Initially not explored or visible
    EXPECT_FALSE(tile.is_explored());
    EXPECT_FALSE(tile.is_visible());
    
    tile.set_explored();
    EXPECT_TRUE(tile.is_explored());
    EXPECT_FALSE(tile.is_visible());
    
    tile.set_visible();
    EXPECT_TRUE(tile.is_explored());
    EXPECT_TRUE(tile.is_visible());
    
    tile.set_visible(false);
    EXPECT_TRUE(tile.is_explored());
    EXPECT_FALSE(tile.is_visible());
}

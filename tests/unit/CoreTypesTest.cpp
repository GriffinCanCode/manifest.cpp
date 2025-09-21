#include "../utils/TestUtils.hpp"
#include "../../src/core/types/Types.hpp"

using namespace Manifest::Core::Types;
using namespace Manifest::Test;

class CoreTypesTest : public DeterministicTest {};

// Strong ID tests
TEST_F(CoreTypesTest, StrongIdBasicOperations) {
    TileId id1{42};
    TileId id2{43};
    TileId id3{42};
    
    EXPECT_EQ(id1.value(), 42u);
    EXPECT_EQ(id2.value(), 43u);
    
    EXPECT_EQ(id1, id3);
    EXPECT_NE(id1, id2);
    EXPECT_LT(id1, id2);
    EXPECT_TRUE(id1.is_valid());
    
    TileId invalid_id = TileId::invalid();
    EXPECT_FALSE(invalid_id.is_valid());
}

TEST_F(CoreTypesTest, StrongIdIncrementOperators) {
    TileId id{100};
    
    TileId pre_inc = ++id;
    EXPECT_EQ(id.value(), 101u);
    EXPECT_EQ(pre_inc.value(), 101u);
    
    TileId post_inc = id++;
    EXPECT_EQ(id.value(), 102u);
    EXPECT_EQ(post_inc.value(), 101u);
}

TEST_F(CoreTypesTest, StrongIdTypeSafety) {
    TileId tile_id{1};
    CityId city_id{1};
    
    // This should compile - same type comparison
    EXPECT_EQ(tile_id, TileId{1});
    EXPECT_EQ(city_id, CityId{1});
    
    // These would cause compile errors if uncommented (different types):
    // bool invalid = (tile_id == city_id);
    // bool invalid2 = (tile_id < city_id);
}

// Quantity tests  
TEST_F(CoreTypesTest, QuantityBasicOperations) {
    Money gold{100.0};
    Money silver{50.0};
    
    EXPECT_DOUBLE_EQ(gold.value(), 100.0);
    EXPECT_DOUBLE_EQ(silver.value(), 50.0);
    
    Money total = gold + silver;
    EXPECT_DOUBLE_EQ(total.value(), 150.0);
    
    Money difference = gold - silver;
    EXPECT_DOUBLE_EQ(difference.value(), 50.0);
    
    Money doubled = gold * 2.0;
    EXPECT_DOUBLE_EQ(doubled.value(), 200.0);
    
    Money halved = gold / 2.0;
    EXPECT_DOUBLE_EQ(halved.value(), 50.0);
}

TEST_F(CoreTypesTest, QuantityCompoundAssignment) {
    Money money{100.0};
    
    money += Money{50.0};
    EXPECT_DOUBLE_EQ(money.value(), 150.0);
    
    money -= Money{25.0};
    EXPECT_DOUBLE_EQ(money.value(), 125.0);
    
    money *= 2.0;
    EXPECT_DOUBLE_EQ(money.value(), 250.0);
    
    money /= 5.0;
    EXPECT_DOUBLE_EQ(money.value(), 50.0);
}

TEST_F(CoreTypesTest, QuantityComparison) {
    Money a{100.0};
    Money b{150.0};
    Money c{100.0};
    
    EXPECT_LT(a, b);
    EXPECT_GT(b, a);
    EXPECT_EQ(a, c);
    EXPECT_LE(a, b);
    EXPECT_LE(a, c);
    EXPECT_GE(b, a);
    EXPECT_GE(c, a);
}

// Coordinate tests
TEST_F(CoreTypesTest, CoordinateBasicOperations) {
    Coordinate coord1{10, 20, 30};
    Coordinate coord2{10, 20, 30};
    Coordinate coord3{5, 10, 15};
    
    EXPECT_EQ(coord1.x, 10);
    EXPECT_EQ(coord1.y, 20);
    EXPECT_EQ(coord1.z, 30);
    
    EXPECT_EQ(coord1, coord2);
    EXPECT_NE(coord1, coord3);
    EXPECT_LT(coord3, coord1);
}

TEST_F(CoreTypesTest, HexCoordinateValidation) {
    HexCoordinate valid{1, -1, 0};
    HexCoordinate invalid{1, 1, 1};
    
    EXPECT_TRUE(valid.is_valid());
    EXPECT_FALSE(invalid.is_valid());
    
    // Test boundary cases
    HexCoordinate origin{0, 0, 0};
    EXPECT_TRUE(origin.is_valid());
    
    HexCoordinate large{100, -50, -50};
    EXPECT_TRUE(large.is_valid());
}

// Bit manipulation tests
TEST_F(CoreTypesTest, BitManipulation) {
    std::uint32_t value = 0;
    
    value = set_bit(value, 0);
    EXPECT_EQ(value, 1u);
    EXPECT_TRUE(test_bit(value, 0));
    
    value = set_bit(value, 3);
    EXPECT_EQ(value, 9u); // Binary: 1001
    EXPECT_TRUE(test_bit(value, 3));
    
    value = toggle_bit(value, 0);
    EXPECT_EQ(value, 8u); // Binary: 1000
    EXPECT_FALSE(test_bit(value, 0));
    
    value = clear_bit(value, 3);
    EXPECT_EQ(value, 0u);
    EXPECT_FALSE(test_bit(value, 3));
}

TEST_F(CoreTypesTest, DirectionEnum) {
    Direction north = Direction::North;
    Direction northeast = Direction::Northeast;
    
    EXPECT_EQ(static_cast<std::uint8_t>(north), 5u);
    EXPECT_EQ(static_cast<std::uint8_t>(northeast), 1u);
    
    // Test that we have all 6 directions for hex grid
    std::array<Direction, 6> all_directions = {
        Direction::North,
        Direction::Northeast, 
        Direction::Southeast,
        Direction::South,
        Direction::Southwest,
        Direction::Northwest
    };
    
    for (std::size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(static_cast<std::size_t>(all_directions[i]), (i + 5) % 6);
    }
}

TEST_F(CoreTypesTest, LayerEnum) {
    Layer underground = Layer::Underground;
    Layer surface = Layer::Surface;
    Layer air = Layer::Air;
    Layer space = Layer::Space;
    
    EXPECT_LT(static_cast<std::uint8_t>(underground), static_cast<std::uint8_t>(surface));
    EXPECT_LT(static_cast<std::uint8_t>(surface), static_cast<std::uint8_t>(air));
    EXPECT_LT(static_cast<std::uint8_t>(air), static_cast<std::uint8_t>(space));
}

// Performance tests
TEST_F(CoreTypesTest, StrongIdPerformance) {
    constexpr int ITERATIONS = 1000000;
    std::vector<TileId> ids;
    ids.reserve(ITERATIONS);
    
    BenchmarkTimer timer;
    
    // Test ID creation and storage
    for (int i = 0; i < ITERATIONS; ++i) {
        ids.emplace_back(static_cast<std::uint32_t>(i));
    }
    
    auto creation_time = timer.elapsed();
    timer.restart();
    
    // Test ID comparison
    std::sort(ids.begin(), ids.end());
    
    auto sort_time = timer.elapsed();
    
    std::cout << "Created " << ITERATIONS << " IDs in " << creation_time.count() << " μs" << std::endl;
    std::cout << "Sorted " << ITERATIONS << " IDs in " << sort_time.count() << " μs" << std::endl;
    
    // Performance should be similar to raw integers
    EXPECT_LT(creation_time.count(), 50000); // 50ms
    EXPECT_LT(sort_time.count(), 100000);    // 100ms
}

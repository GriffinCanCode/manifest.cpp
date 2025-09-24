#include "../utils/TestUtils.hpp"
#include "../../src/core/math/Vector.hpp"
#include "../../src/core/math/Matrix.hpp"
#include "../../src/world/tiles/Tile.hpp"
#include "../../src/world/tiles/Map.hpp"
#include "../../src/render/camera/Camera.hpp"
#include "../../src/render/common/ProceduralHexRenderer.hpp"

using namespace Manifest::Core::Math;
using namespace Manifest::World::Tiles;
using namespace Manifest::Render::CameraSystem;
using namespace Manifest::Test;

class RenderingDebugTest : public DeterministicTest {
protected:
    void SetUp() override {
        DeterministicTest::SetUp();
    }

    void TearDown() override {
        DeterministicTest::TearDown();
    }
};

TEST_F(RenderingDebugTest, BasicMathOperations) {
    // Test 1: Verify basic math operations work
    Vec3f test_vec{1.0f, 2.0f, 3.0f};
    float length = test_vec.length();
    EXPECT_GT(length, 0.0f) << "Vector length should be positive";
    EXPECT_NEARLY_EQUAL(length, std::sqrt(14.0f), 0.001f) << "Vector length calculation incorrect";
    
    // Test matrix operations
    Mat4f identity = Mat4f::identity();
    Vec4f test_transform = identity * Vec4f{1.0f, 2.0f, 3.0f, 1.0f};
    EXPECT_NEARLY_EQUAL(test_transform.x(), 1.0f, 0.001f) << "Identity matrix should preserve coordinates";
    EXPECT_NEARLY_EQUAL(test_transform.y(), 2.0f, 0.001f) << "Identity matrix should preserve coordinates";
    EXPECT_NEARLY_EQUAL(test_transform.z(), 3.0f, 0.001f) << "Identity matrix should preserve coordinates";
    
    std::cout << "✓ Basic math operations working correctly\n";
}

TEST_F(RenderingDebugTest, CameraTransformations) {
    // Test 2: Camera matrix generation
    System camera_system;
    auto& camera = camera_system.camera();
    
    // Set up camera like in main.cpp
    camera.set_perspective(60.0f * M_PI / 180.0f, 800.0f / 600.0f, 1.0f, 10000.0f);
    camera.look_at(Vec3f{0.0f, 100.0f, 50.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    
    // Test matrices are non-zero and finite
    const Mat4f& view = camera.view_matrix();
    const Mat4f& proj = camera.projection_matrix();
    
    bool view_valid = true;
    bool proj_valid = true;
    
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            if (!std::isfinite(view[r][c])) view_valid = false;
            if (!std::isfinite(proj[r][c])) proj_valid = false;
        }
    }
    
    EXPECT_TRUE(view_valid) << "View matrix contains invalid values";
    EXPECT_TRUE(proj_valid) << "Projection matrix contains invalid values";
    
    // Test that camera position is correct
    Vec3f camera_pos = camera.position();
    EXPECT_NEARLY_EQUAL(camera_pos.x(), 0.0f, 0.1f);
    EXPECT_NEARLY_EQUAL(camera_pos.y(), 100.0f, 0.1f);
    EXPECT_NEARLY_EQUAL(camera_pos.z(), 50.0f, 0.1f);
    
    std::cout << "✓ Camera transformations working correctly\n";
    std::cout << "  Camera position: (" << camera_pos.x() << ", " << camera_pos.y() << ", " << camera_pos.z() << ")\n";
}

TEST_F(RenderingDebugTest, HexCoordinateToWorldConversion) {
    // Test 3: Hex coordinate to world position conversion
    const float hex_radius = 15.0f;
    
    // Test center hex (0, 0, 0)
    HexCoordinate center{0, 0, 0};
    ASSERT_VALID_HEX(center);
    
    Vec3f center_world = ProceduralHexRenderer::hex_coord_to_world(center, hex_radius);
    EXPECT_NEARLY_EQUAL(center_world.x(), 0.0f, 0.001f) << "Center hex should be at world origin X";
    EXPECT_NEARLY_EQUAL(center_world.z(), 0.0f, 0.001f) << "Center hex should be at world origin Z";
    EXPECT_NEARLY_EQUAL(center_world.y(), 0.0f, 0.001f) << "Center hex Y should be 0";
    
    // Test a few known coordinates
    HexCoordinate test_coords[] = {
        {1, 0, -1},   // East
        {0, 1, -1},   // Southeast  
        {-1, 1, 0},   // Southwest
        {-1, 0, 1},   // West
        {0, -1, 1},   // Northwest
        {1, -1, 0}    // Northeast
    };
    
    for (const auto& coord : test_coords) {
        ASSERT_VALID_HEX(coord);
        Vec3f world_pos = ProceduralHexRenderer::hex_coord_to_world(coord, hex_radius);
        
        EXPECT_TRUE(std::isfinite(world_pos.x())) << "World X coordinate should be finite";
        EXPECT_TRUE(std::isfinite(world_pos.z())) << "World Z coordinate should be finite";
        
        // Hex should be roughly hex_radius * sqrt(3) away from center for adjacent hexes
        float distance = Vec3f{world_pos.x(), 0.0f, world_pos.z()}.length();
        EXPECT_GT(distance, hex_radius * 0.8f) << "Adjacent hex should be reasonable distance from center";
        EXPECT_LT(distance, hex_radius * 3.0f) << "Adjacent hex shouldn't be too far from center";
    }
    
    std::cout << "✓ Hex coordinate conversions working correctly\n";
    std::cout << "  Sample conversions:\n";
    for (int i = 0; i < 3; ++i) {
        auto coord = test_coords[i];
        auto world_pos = ProceduralHexRenderer::hex_coord_to_world(coord, hex_radius);
        std::cout << "    Hex(" << coord.q << "," << coord.r << "," << coord.s 
                  << ") -> World(" << world_pos.x() << "," << world_pos.y() << "," << world_pos.z() << ")\n";
    }
}

TEST_F(RenderingDebugTest, ViewProjectionPipelineTest) {
    // Test 4: Complete view-projection pipeline
    System camera_system;
    auto& camera = camera_system.camera();
    
    // Use the same camera setup as main.cpp
    camera.set_perspective(60.0f * M_PI / 180.0f, 800.0f / 600.0f, 1.0f, 10000.0f);
    camera.look_at(Vec3f{0.0f, 100.0f, 50.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    
    Mat4f view_proj = camera.projection_matrix() * camera.view_matrix();
    
    // Test transforming some hex positions
    const float hex_radius = 15.0f;
    std::vector<HexCoordinate> test_coords = {
        {0, 0, 0},    // Center
        {1, 0, -1},   // Adjacent
        {2, -1, -1},  // Nearby
    };
    
    int visible_count = 0;
    
    for (const auto& coord : test_coords) {
        Vec3f world_pos = ProceduralHexRenderer::hex_coord_to_world(coord, hex_radius);
        
        // Transform to clip space
        Vec4f clip_pos = view_proj * Vec4f{world_pos.x(), world_pos.y(), world_pos.z(), 1.0f};
        
        // Convert to NDC
        if (clip_pos.w() > 0.0f) {
            Vec3f ndc{clip_pos.x() / clip_pos.w(), clip_pos.y() / clip_pos.w(), clip_pos.z() / clip_pos.w()};
            
            // Check if visible (within [-1, 1] NDC range)
            bool visible = (std::abs(ndc.x()) <= 1.0f && std::abs(ndc.y()) <= 1.0f && 
                           ndc.z() >= -1.0f && ndc.z() <= 1.0f);
            
            if (visible) {
                visible_count++;
                std::cout << "  Hex(" << coord.q << "," << coord.r << ") -> NDC(" 
                          << ndc.x() << "," << ndc.y() << "," << ndc.z() << ") VISIBLE\n";
            } else {
                std::cout << "  Hex(" << coord.q << "," << coord.r << ") -> NDC(" 
                          << ndc.x() << "," << ndc.y() << "," << ndc.z() << ") not visible\n";
            }
        } else {
            std::cout << "  Hex(" << coord.q << "," << coord.r << ") behind camera\n";
        }
    }
    
    std::cout << "✓ View-projection pipeline test completed\n";
    std::cout << "  Visible hexes: " << visible_count << " / " << test_coords.size() << "\n";
    
    // At least the center hex should be visible
    EXPECT_GT(visible_count, 0) << "At least some hexes should be visible with current camera setup";
}

TEST_F(RenderingDebugTest, BasicWorldGeneration) {
    // Test 5: Basic world generation
    TileMap test_map;
    
    // Create a few manual tiles
    auto tile1_id = test_map.create_tile(HexCoordinate{0, 0, 0});
    auto tile2_id = test_map.create_tile(HexCoordinate{1, 0, -1});
    auto tile3_id = test_map.create_tile(HexCoordinate{0, 1, -1});
    
    EXPECT_TRUE(tile1_id.has_value()) << "Should be able to create tiles";
    EXPECT_TRUE(tile2_id.has_value()) << "Should be able to create tiles";
    EXPECT_TRUE(tile3_id.has_value()) << "Should be able to create tiles";
    
    EXPECT_EQ(test_map.tile_count(), 3) << "Should have exactly 3 tiles";
    
    // Get tiles back
    auto* tile1 = test_map.get_tile(*tile1_id);
    auto* tile2 = test_map.get_tile(*tile2_id);
    auto* tile3 = test_map.get_tile(*tile3_id);
    
    ASSERT_NE(tile1, nullptr) << "Should be able to retrieve created tile";
    ASSERT_NE(tile2, nullptr) << "Should be able to retrieve created tile";
    ASSERT_NE(tile3, nullptr) << "Should be able to retrieve created tile";
    
    // Verify coordinates
    EXPECT_EQ(tile1->coordinate(), HexCoordinate(0, 0, 0));
    EXPECT_EQ(tile2->coordinate(), HexCoordinate(1, 0, -1));
    EXPECT_EQ(tile3->coordinate(), HexCoordinate(0, 1, -1));
    
    std::cout << "✓ Basic world generation working correctly\n";
    std::cout << "  Created " << test_map.tile_count() << " tiles successfully\n";
}

// Quick smoke test that can be run independently
TEST(RenderingQuickCheck, BasicSmokeTest) {
    std::cout << "🔍 Running basic rendering smoke test...\n";
    
    // Test 1: Basic math operations
    Vec3f test_vec{1.0f, 2.0f, 3.0f};
    float length = test_vec.length();
    EXPECT_GT(length, 0.0f) << "Vector math not working";
    
    // Test 2: Matrix operations
    Mat4f identity = Mat4f::identity();
    Vec4f test_transform = identity * Vec4f{1.0f, 2.0f, 3.0f, 1.0f};
    EXPECT_NEARLY_EQUAL(test_transform.x(), 1.0f, 0.001f) << "Matrix math not working";
    
    // Test 3: Hex coordinates
    HexCoordinate test_coord{1, -1, 0};
    EXPECT_TRUE(test_coord.is_valid()) << "Hex coordinate validation not working";
    
    std::cout << "✓ Smoke test passed - basic systems operational\n";
}
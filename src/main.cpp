#include "core/types/Types.hpp"
#include "core/math/Vector.hpp"
#include "core/math/Matrix.hpp"
#include "core/memory/Pool.hpp"
#include "core/time/Time.hpp"
#include "world/tiles/Map.hpp"
#include "world/terrain/Generator.hpp"
#include "render/common/Camera.hpp"
#include <iostream>
#include <chrono>
#include <format>

using namespace Manifest;
using namespace Core::Types;
using namespace Core::Math;
using namespace Core::Memory;
using namespace Core::Time;
using namespace World::Tiles;
using namespace World::Terrain;
using namespace Render;

void print_banner() {
    std::cout << R"(
    ╔═══════════════════════════════════════════════╗
    ║              MANIFEST ENGINE                  ║
    ║         Grand Strategy Empire Builder         ║
    ║                                               ║
    ║  Modern C++23 • Vulkan/OpenGL • Procedural   ║
    ╚═══════════════════════════════════════════════╝
    )" << '\n' << std::endl;
}

void demonstrate_strong_types() {
    std::cout << "=== Strong Type System Demo ===\n";
    
    TileId tile1{42};
    TileId tile2{43};
    CityId city{100};
    
    std::cout << std::format("Tile ID 1: {}\n", tile1.value());
    std::cout << std::format("Tile ID 2: {}\n", tile2.value());
    std::cout << std::format("City ID: {}\n", city.value());
    std::cout << std::format("Tile1 < Tile2: {}\n", tile1 < tile2);
    
    // This would cause a compile error (different types):
    // bool invalid = (tile1 == city);
    
    Money gold{1000.0};
    Population people{50000.0};
    
    std::cout << std::format("Gold: {:.2f}\n", gold.value());
    std::cout << std::format("Population: {:.0f}\n", people.value());
    
    Money more_gold = gold + Money{500.0};
    std::cout << std::format("More gold: {:.2f}\n", more_gold.value());
    std::cout << std::endl;
}

void demonstrate_math_system() {
    std::cout << "=== SIMD Math System Demo ===\n";
    
    Vec3f position{1.0f, 2.0f, 3.0f};
    Vec3f direction{0.5f, 0.0f, -0.5f};
    
    std::cout << std::format("Position: ({:.2f}, {:.2f}, {:.2f})\n", 
                             position.x(), position.y(), position.z());
    std::cout << std::format("Direction length: {:.3f}\n", direction.length());
    
    Vec3f normalized = direction.normalized();
    std::cout << std::format("Normalized: ({:.3f}, {:.3f}, {:.3f})\n", 
                             normalized.x(), normalized.y(), normalized.z());
    
    float dot_product = position.dot(direction);
    std::cout << std::format("Dot product: {:.3f}\n", dot_product);
    
    Vec3f cross_product = position.cross(direction);
    std::cout << std::format("Cross product: ({:.3f}, {:.3f}, {:.3f})\n", 
                             cross_product.x(), cross_product.y(), cross_product.z());
    
    // Matrix operations
    Mat4f transform = translation(Vec3f{10.0f, 0.0f, 0.0f}) * rotation_y(std::numbers::pi_v<float> / 4.0f);
    std::cout << "4x4 transformation matrix created successfully\n";
    std::cout << std::endl;
}

void demonstrate_memory_system() {
    std::cout << "=== Memory Pool System Demo ===\n";
    
    Pool<Vec3f> vector_pool(64);  // Pool of 64 Vec3f objects
    
    std::cout << std::format("Initial pool capacity: {}\n", vector_pool.capacity());
    std::cout << std::format("Initial pool usage: {}\n", vector_pool.used());
    
    // Acquire some objects
    auto* vec1 = vector_pool.acquire(1.0f, 2.0f, 3.0f);
    auto* vec2 = vector_pool.acquire(4.0f, 5.0f, 6.0f);
    auto* vec3 = vector_pool.acquire(7.0f, 8.0f, 9.0f);
    
    std::cout << std::format("After acquiring 3 objects: {}/{}\n", 
                             vector_pool.used(), vector_pool.capacity());
    std::cout << std::format("Usage ratio: {:.2f}\n", vector_pool.usage_ratio());
    
    std::cout << std::format("Vec1: ({:.1f}, {:.1f}, {:.1f})\n", vec1->x(), vec1->y(), vec1->z());
    std::cout << std::format("Vec2: ({:.1f}, {:.1f}, {:.1f})\n", vec2->x(), vec2->y(), vec2->z());
    
    // Return objects to pool
    vector_pool.release(vec1);
    vector_pool.release(vec2);
    vector_pool.release(vec3);
    
    std::cout << std::format("After releasing: {}/{}\n", 
                             vector_pool.used(), vector_pool.capacity());
    std::cout << std::endl;
}

void demonstrate_world_generation() {
    std::cout << "=== World Generation Demo ===\n";
    
    TileMap world_map;
    GenerationParams params{
        .seed = 12345,
        .map_radius = 20,
        .sea_level = 0.3f
    };
    
    TerrainGenerator generator(params);
    
    std::cout << "Generating world...\n";
    auto start_time = std::chrono::high_resolution_clock::now();
    
    generator.generate_world(world_map);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << std::format("Generated {} tiles in {}ms\n", 
                             world_map.tile_count(), duration.count());
    
    // Analyze terrain distribution
    std::unordered_map<TerrainType, int> terrain_counts;
    std::unordered_map<ResourceType, int> resource_counts;
    
    world_map.for_each_tile([&](const Tile& tile) {
        terrain_counts[tile.terrain()]++;
        if (tile.resource() != ResourceType::None) {
            resource_counts[tile.resource()]++;
        }
    });
    
    std::cout << "\nTerrain Distribution:\n";
    for (const auto& [terrain, count] : terrain_counts) {
        float percentage = (static_cast<float>(count) / world_map.tile_count()) * 100.0f;
        std::cout << std::format("  {}: {} ({:.1f}%)\n", 
                                 static_cast<int>(terrain), count, percentage);
    }
    
    std::cout << std::format("\nTotal resources found: {}\n", 
                             std::accumulate(resource_counts.begin(), resource_counts.end(), 0,
                                           [](int sum, const auto& pair) { return sum + pair.second; }));
    
    // Test pathfinding preparation
    auto center_coord = HexCoordinate{0, 0, 0};
    auto nearby_tiles = world_map.get_tiles_in_radius(center_coord, 3);
    std::cout << std::format("Tiles within 3 hexes of center: {}\n", nearby_tiles.size());
    
    std::cout << std::endl;
}

void demonstrate_camera_system() {
    std::cout << "=== Camera System Demo ===\n";
    
    Camera camera;
    camera.look_at(Vec3f{10.0f, 5.0f, 10.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    camera.set_perspective(std::numbers::pi_v<float> / 4.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    
    std::cout << std::format("Camera position: ({:.2f}, {:.2f}, {:.2f})\n", 
                             camera.position().x(), camera.position().y(), camera.position().z());
    std::cout << std::format("Camera target: ({:.2f}, {:.2f}, {:.2f})\n", 
                             camera.target().x(), camera.target().y(), camera.target().z());
    std::cout << std::format("Field of view: {:.2f} degrees\n", 
                             camera.fov() * 180.0f / std::numbers::pi_v<float>);
    
    // Test movement
    camera.move_forward(2.0f);
    camera.rotate_yaw(std::numbers::pi_v<float> / 6.0f);
    
    std::cout << "After movement and rotation:\n";
    std::cout << std::format("Camera position: ({:.2f}, {:.2f}, {:.2f})\n", 
                             camera.position().x(), camera.position().y(), camera.position().z());
    
    // Test matrix generation
    const Mat4f& view_matrix = camera.view_matrix();
    const Mat4f& proj_matrix = camera.projection_matrix();
    const Mat4f& vp_matrix = camera.view_projection_matrix();
    
    std::cout << "View, projection, and view-projection matrices generated successfully\n";
    std::cout << std::endl;
}

void demonstrate_time_system() {
    std::cout << "=== Time System Demo ===\n";
    
    GameTime game_time;
    FrameTimer frame_timer;
    
    std::cout << std::format("Initial game turn: {}\n", game_time.current_turn());
    std::cout << std::format("Initial game year: {}\n", game_time.current_year());
    
    // Simulate some time passing
    for (int i = 0; i < 5; ++i) {
        frame_timer.update();
        game_time.update(std::chrono::milliseconds(100));
        
        std::cout << std::format("Frame {}: Turn {}, Progress {:.2f}\n", 
                                 i + 1, 
                                 game_time.current_turn(),
                                 game_time.turn_progress_ratio());
    }
    
    // Test timer
    int timer_count = 0;
    auto timer = Timer(std::chrono::milliseconds(50), [&timer_count]() {
        ++timer_count;
        std::cout << std::format("Timer fired! Count: {}\n", timer_count);
    });
    
    for (int i = 0; i < 3; ++i) {
        timer.update(std::chrono::milliseconds(60));
    }
    
    std::cout << std::endl;
}

int main() {
    try {
        print_banner();
        
        demonstrate_strong_types();
        demonstrate_math_system();
        demonstrate_memory_system();
        demonstrate_world_generation();
        demonstrate_camera_system();
        demonstrate_time_system();
        
        std::cout << "🎉 All systems operational! The Manifest Engine foundation is complete.\n" << std::endl;
        std::cout << "Next steps:\n";
        std::cout << "  • Implement economic simulation\n";
        std::cout << "  • Add government and politics systems\n";
        std::cout << "  • Create military mechanics\n";
        std::cout << "  • Build UI framework\n";
        std::cout << "  • Add AI opponents\n";
        std::cout << "  • Implement networking\n" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

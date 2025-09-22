#include <iostream>
#include <chrono>
#include <iomanip>

#include "core/math/Matrix.hpp"
#include "core/math/Vector.hpp"
#include "core/time/Time.hpp"
#include "core/types/Types.hpp"
#include "core/log/Log.hpp"
#include "render/common/Camera.hpp"
#include "world/terrain/Generator.hpp"
#include "world/tiles/Map.hpp"

using namespace Manifest;
using namespace Manifest::Core::Types;
using namespace Manifest::Core::Math;
using namespace Manifest::Core::Memory;
using namespace Manifest::Core::Time;
using namespace Manifest::World::Tiles;
using namespace Manifest::World::Terrain;
using namespace Manifest::Render;

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

    std::cout << "Tile ID 1: " << tile1.value() << "\n";
    std::cout << "Tile ID 2: " << tile2.value() << "\n";
    std::cout << "City ID: " << city.value() << "\n";
    std::cout << "Tile1 < Tile2: " << std::boolalpha << (tile1 < tile2) << "\n";

    // This would cause a compile error (different types):
    // bool invalid = (tile1 == city);

    Money gold{1000.0};
    Population people{50000.0};

    std::cout << "Gold: " << std::fixed << std::setprecision(2) << gold.value() << "\n";
    std::cout << "Population: " << std::fixed << std::setprecision(0) << people.value() << "\n";

    Money more_gold = gold + Money{500.0};
    std::cout << "More gold: " << std::fixed << std::setprecision(2) << more_gold.value() << "\n";
    std::cout << std::endl;
}

void demonstrate_math_system() {
    std::cout << "=== SIMD Math System Demo ===\n";

    Vec3f position{1.0f, 2.0f, 3.0f};
    Vec3f direction{0.5f, 0.0f, -0.5f};

    std::cout << "Position: (" << std::fixed << std::setprecision(2) << position.x() << ", "
              << position.y() << ", " << position.z() << ")\n";
    std::cout << "Direction length: " << std::fixed << std::setprecision(3) << direction.length()
              << "\n";

    Vec3f normalized = direction.normalized();
    std::cout << "Normalized: (" << std::fixed << std::setprecision(3) << normalized.x() << ", "
              << normalized.y() << ", " << normalized.z() << ")\n";

    float dot_product = position.dot(direction);
    std::cout << "Dot product: " << std::fixed << std::setprecision(3) << dot_product << "\n";

    Vec3f cross_product = position.cross(direction);
    std::cout << "Cross product: (" << std::fixed << std::setprecision(3) << cross_product.x()
              << ", " << cross_product.y() << ", " << cross_product.z() << ")\n";

    // Matrix operations
    [[maybe_unused]] Mat4f transform =
        translation(Vec3f{10.0f, 0.0f, 0.0f}) * rotation_y(static_cast<float>(M_PI) / 4.0f);
    std::cout << "4x4 transformation matrix created successfully\n";
    std::cout << std::endl;
}

void demonstrate_memory_system() {
    std::cout << "=== Memory Pool System Demo ===\n";

    Pool<Vec3f> vector_pool(64);  // Pool of 64 Vec3f objects

    std::cout << "Initial pool capacity: " << vector_pool.capacity() << "\n";
    std::cout << "Initial pool usage: " << vector_pool.used() << "\n";

    // Acquire some objects
    auto* vec1 = vector_pool.acquire(1.0f, 2.0f, 3.0f);
    auto* vec2 = vector_pool.acquire(4.0f, 5.0f, 6.0f);
    auto* vec3 = vector_pool.acquire(7.0f, 8.0f, 9.0f);

    std::cout << "After acquiring 3 objects: " << vector_pool.used() << "/"
              << vector_pool.capacity() << "\n";
    std::cout << "Usage ratio: " << std::fixed << std::setprecision(2) << vector_pool.usage_ratio()
              << "\n";

    std::cout << "Vec1: (" << std::fixed << std::setprecision(1) << vec1->x() << ", " << vec1->y()
              << ", " << vec1->z() << ")\n";
    std::cout << "Vec2: (" << std::fixed << std::setprecision(1) << vec2->x() << ", " << vec2->y()
              << ", " << vec2->z() << ")\n";

    // Return objects to pool
    vector_pool.release(vec1);
    vector_pool.release(vec2);
    vector_pool.release(vec3);

    std::cout << "After releasing: " << vector_pool.used() << "/" << vector_pool.capacity() << "\n";
    std::cout << std::endl;
}

void demonstrate_world_generation() {
    std::cout << "=== World Generation Demo ===\n";

    TileMap world_map;
    GenerationParams params{.seed = 12345, .map_radius = 20, .sea_level = 0.3f};

    TerrainGenerator generator(params);

    std::cout << "Generating world...\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    generator.generate_world(world_map);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Generated " << world_map.tile_count() << " tiles in " << duration.count()
              << "ms\n";

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
    for (const auto& pair : terrain_counts) {
        TerrainType terrain = pair.first;
        int count = pair.second;
        float percentage = (static_cast<float>(count) / world_map.tile_count()) * 100.0f;
        std::cout << "  " << static_cast<int>(terrain) << ": " << count << " (" << std::fixed
                  << std::setprecision(1) << percentage << "%)\n";
    }

    int total_resources =
        std::accumulate(resource_counts.begin(), resource_counts.end(), 0,
                        [](int sum, const auto& pair) { return sum + pair.second; });
    std::cout << "\nTotal resources found: " << total_resources << "\n";

    // Test pathfinding preparation
    auto center_coord = HexCoordinate{0, 0, 0};
    auto nearby_tiles = world_map.get_tiles_in_radius(center_coord, 3);
    std::cout << "Tiles within 3 hexes of center: " << nearby_tiles.size() << "\n";

    std::cout << std::endl;
}

void demonstrate_camera_system() {
    std::cout << "=== Camera System Demo ===\n";

    Camera camera;
    camera.look_at(Vec3f{10.0f, 5.0f, 10.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    camera.set_perspective(static_cast<float>(M_PI) / 4.0f, 16.0f / 9.0f, 0.1f, 1000.0f);

    std::cout << "Camera position: (" << std::fixed << std::setprecision(2) << camera.position().x()
              << ", " << camera.position().y() << ", " << camera.position().z() << ")\n";
    std::cout << "Camera target: (" << std::fixed << std::setprecision(2) << camera.target().x()
              << ", " << camera.target().y() << ", " << camera.target().z() << ")\n";
    std::cout << "Field of view: " << std::fixed << std::setprecision(2)
              << (camera.fov() * 180.0f / static_cast<float>(M_PI)) << " degrees\n";

    // Test movement
    camera.move_forward(2.0f);
    camera.rotate_yaw(static_cast<float>(M_PI) / 6.0f);

    std::cout << "After movement and rotation:\n";
    std::cout << "Camera position: (" << std::fixed << std::setprecision(2) << camera.position().x()
              << ", " << camera.position().y() << ", " << camera.position().z() << ")\n";

    // Test matrix generation
    [[maybe_unused]] const Mat4f& view_matrix = camera.view_matrix();
    [[maybe_unused]] const Mat4f& proj_matrix = camera.projection_matrix();
    [[maybe_unused]] const Mat4f& vp_matrix = camera.view_projection_matrix();

    std::cout << "View, projection, and view-projection matrices generated successfully\n";
    std::cout << std::endl;
}

void demonstrate_time_system() {
    std::cout << "=== Time System Demo ===\n";

    GameTime game_time;
    FrameTimer frame_timer;

    std::cout << "Initial game turn: " << game_time.current_turn() << "\n";
    std::cout << "Initial game year: " << game_time.current_year() << "\n";

    // Simulate some time passing
    for (int i = 0; i < 5; ++i) {
        frame_timer.update();
        game_time.update(std::chrono::milliseconds(100));

        std::cout << "Frame " << (i + 1) << ": Turn " << game_time.current_turn() << ", Progress "
                  << std::fixed << std::setprecision(2) << game_time.turn_progress_ratio() << "\n";
    }

    // Test timer
    int timer_count = 0;
    auto callback = [&timer_count]() {
        ++timer_count;
        std::cout << "Timer fired! Count: " << timer_count << "\n";
    };
    auto timer = Timer<decltype(callback)>(std::chrono::milliseconds(50), callback);

    for (int i = 0; i < 3; ++i) {
        timer.update(std::chrono::milliseconds(60));
    }

    std::cout << std::endl;
}

int main() {
    try {
        // Initialize logging system with multi-sink setup
        auto multi_sink = std::make_shared<Manifest::Core::Log::MultiSink>();
        multi_sink->add_sink(std::make_shared<Manifest::Core::Log::ConsoleSink>(true));
        multi_sink->add_sink(std::make_shared<Manifest::Core::Log::FileSink>("manifest_engine.log"));
        
        auto global_logger = Manifest::Core::Log::Registry::create("Manifest", std::dynamic_pointer_cast<Manifest::Core::Log::Sink>(multi_sink));
        Manifest::Core::Log::Global::set(std::move(global_logger));

        LOG_INFO("=== Manifest Engine Initialization ===");
        
        print_banner();

        // LOG_DEBUG("Running system demonstrations...");
        demonstrate_strong_types();
        demonstrate_math_system();
        demonstrate_memory_system();
        demonstrate_world_generation();
        demonstrate_camera_system();
        demonstrate_time_system();

        LOG_INFO("🎉 All systems operational! The Manifest Engine foundation is complete.");
        LOG_INFO("Next steps:");
        LOG_INFO("  • Implement economic simulation");
        LOG_INFO("  • Add government and politics systems");
        LOG_INFO("  • Create military mechanics");
        LOG_INFO("  • Build UI framework");
        LOG_INFO("  • Add AI opponents");
        LOG_INFO("  • Implement networking");

        return 0;
    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception: {}", e.what());
        return 1;
    }
}

/**
 * Standalone debug executable to isolate the grey screen rendering issue
 */

#include "src/core/log/Logger.hpp"
#include "src/core/math/Vector.hpp"
#include "src/core/math/Matrix.hpp"
#include "src/render/camera/Camera.hpp"
#include "src/render/common/ProceduralHexRenderer.hpp"
#include "src/render/RendererFactory.cpp"
#include "src/world/tiles/Map.hpp"
#include "src/world/terrain/Generator.hpp"
#include "src/ui/window/Manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace Manifest::Core::Math;
using namespace Manifest::Core::Log;
using namespace Manifest::Render;
using namespace Manifest::Render::CameraSystem;
using namespace Manifest::World::Tiles;
using namespace Manifest::World::Terrain;
using namespace Manifest::UI::Window;

void debug_basic_math() {
    std::cout << "=== DEBUG: Basic Math Operations ===\n";
    
    Vec3f test_vec{1.0f, 2.0f, 3.0f};
    float length = test_vec.length();
    std::cout << "Vec3f({1, 2, 3}) length: " << length << " (expected ~3.74)\n";
    
    Mat4f identity = Mat4f::identity();
    Vec4f result = identity * Vec4f{1.0f, 2.0f, 3.0f, 1.0f};
    std::cout << "Identity * {1,2,3,1} = {" << result.x() << ", " << result.y() 
              << ", " << result.z() << ", " << result.w() << "}\n";
    
    std::cout << "✓ Basic math appears to be working\n\n";
}

void debug_camera_setup() {
    std::cout << "=== DEBUG: Camera Setup ===\n";
    
    System camera_system;
    auto& camera = camera_system.camera();
    
    // Use exactly the same setup as main.cpp
    camera.set_perspective(60.0f * M_PI / 180.0f, 800.0f / 600.0f, 1.0f, 10000.0f);
    camera.look_at(Vec3f{0.0f, 100.0f, 50.0f}, Vec3f{0.0f, 0.0f, 0.0f});
    
    Vec3f camera_pos = camera.position();
    std::cout << "Camera position: (" << camera_pos.x() << ", " << camera_pos.y() 
              << ", " << camera_pos.z() << ")\n";
    
    const Mat4f& view = camera.view_matrix();
    const Mat4f& proj = camera.projection_matrix();
    
    std::cout << "View matrix sample: [0][0]=" << view[0][0] << ", [1][1]=" << view[1][1] << "\n";
    std::cout << "Proj matrix sample: [0][0]=" << proj[0][0] << ", [1][1]=" << proj[1][1] << "\n";
    
    // Test transforming the origin
    Mat4f vp = proj * view;
    Vec4f origin_clip = vp * Vec4f{0.0f, 0.0f, 0.0f, 1.0f};
    if (origin_clip.w() != 0.0f) {
        Vec3f origin_ndc{origin_clip.x() / origin_clip.w(), 
                         origin_clip.y() / origin_clip.w(), 
                         origin_clip.z() / origin_clip.w()};
        std::cout << "Origin (0,0,0) -> NDC(" << origin_ndc.x() << ", " 
                  << origin_ndc.y() << ", " << origin_ndc.z() << ")\n";
        
        bool visible = (std::abs(origin_ndc.x()) <= 1.0f && 
                       std::abs(origin_ndc.y()) <= 1.0f &&
                       origin_ndc.z() >= -1.0f && origin_ndc.z() <= 1.0f);
        std::cout << "Origin is " << (visible ? "VISIBLE" : "NOT VISIBLE") << "\n";
    }
    
    std::cout << "✓ Camera setup complete\n\n";
}

void debug_world_generation() {
    std::cout << "=== DEBUG: World Generation ===\n";
    
    TileMap world_map;
    GenerationParams params{
        .seed = 42,
        .map_radius = 5,  // Much smaller for debugging
        .sea_level = 0.3f
    };
    
    TerrainGenerator generator(params);
    auto start_time = std::chrono::high_resolution_clock::now();
    generator.generate_world(world_map);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << "Generated " << world_map.tile_count() << " tiles in " 
              << duration.count() << "ms\n";
    
    // Check some specific tiles
    int count = 0;
    for (const auto& tile : world_map.tiles()) {
        if (count < 5) {  // Show first 5 tiles
            auto coord = tile.coordinate();
            std::cout << "Tile " << count << ": coord(" << coord.q << ", " 
                      << coord.r << ", " << coord.s << "), terrain=" 
                      << static_cast<int>(tile.terrain()) << "\n";
        }
        count++;
    }
    
    std::cout << "✓ World generation complete\n\n";
}

int main() {
    std::cout << "🔍 STANDALONE RENDERING DEBUGGER\n";
    std::cout << "================================\n\n";
    
    // Initialize logging
    Logger::initialize(LogLevel::DEBUG);
    
    try {
        debug_basic_math();
        debug_camera_setup();
        debug_world_generation();
        
        std::cout << "=== DEBUG: Window Creation ===\n";
        
        // Create window manager
        Manager window_manager;
        FactoryConfig config{Backend::GLFW};
        auto window_result = window_manager.initialize(config);
        if (!window_result) {
            std::cerr << "❌ Failed to initialize window manager\n";
            return 1;
        }
        std::cout << "✓ Window manager initialized\n";
        
        // Create debug window
        WindowDesc window_desc{
            .size = {800, 600},
            .title = "Rendering Debug",
            .resizable = true,
            .decorated = true,
            .initial_state = WindowState::Normal
        };
        
        auto window_handle = window_manager.create_window(window_desc);
        if (!window_handle.has_value()) {
            std::cerr << "❌ Failed to create debug window\n";
            return 1;
        }
        std::cout << "✓ Debug window created\n";
        
        auto* window = window_manager.get_window(*window_handle);
        if (!window) {
            std::cerr << "❌ Failed to get window reference\n";
            return 1;
        }
        
        std::cout << "=== DEBUG: Renderer Creation ===\n";
        
        // Try to create renderer
        auto renderer = create_renderer(API::Vulkan);
        if (!renderer) {
            std::cout << "❌ Failed to create Vulkan renderer, trying OpenGL...\n";
            renderer = create_renderer(API::OpenGL);
        }
        
        if (!renderer) {
            std::cerr << "❌ Failed to create any renderer\n";
            return 1;
        }
        std::cout << "✓ Renderer created successfully\n";
        
        // Initialize renderer
        auto init_result = renderer->initialize();
        if (!init_result) {
            std::cerr << "❌ Failed to initialize renderer\n";
            return 1;
        }
        std::cout << "✓ Renderer initialized\n";
        
        std::cout << "\n=== RENDERING TEST ===\n";
        std::cout << "Window should now be showing. Look for:\n";
        std::cout << "- Is the window visible?\n";
        std::cout << "- What color is the background?\n";
        std::cout << "- Does the window respond to events?\n\n";
        
        // Simple render loop
        Viewport viewport{
            .position = Vec2f{0.0f, 0.0f},
            .size = Vec2f{800.0f, 600.0f},
            .depth_range = Vec2f{0.0f, 1.0f}
        };
        
        Vec4f test_colors[] = {
            {1.0f, 0.0f, 0.0f, 1.0f},  // Red
            {0.0f, 1.0f, 0.0f, 1.0f},  // Green
            {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
            {1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
            {1.0f, 0.0f, 1.0f, 1.0f}   // Magenta
        };
        
        int color_index = 0;
        auto last_color_change = std::chrono::high_resolution_clock::now();
        
        std::cout << "Starting color cycle test - background should change colors every 2 seconds\n";
        std::cout << "Press Ctrl+C to exit\n";
        
        while (window_manager.should_continue()) {
            window_manager.poll_events();
            
            // Change color every 2 seconds
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_color_change).count() >= 2) {
                color_index = (color_index + 1) % 5;
                last_color_change = now;
                
                std::string color_names[] = {"RED", "GREEN", "BLUE", "YELLOW", "MAGENTA"};
                std::cout << "Switching to " << color_names[color_index] << " background\n";
            }
            
            // Clear with current test color
            Vec4f clear_color = test_colors[color_index];
            
            try {
                // Simple clear operation to test basic rendering
                renderer->clear_screen(clear_color);
                renderer->present();
                
            } catch (const std::exception& e) {
                std::cerr << "❌ Rendering error: " << e.what() << "\n";
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
        }
        
        std::cout << "\n✓ Debug session complete\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Debug session failed: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

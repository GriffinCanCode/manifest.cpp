#include <iostream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <variant>
#include <map>

#include "core/math/Matrix.hpp"
#include "core/math/Vector.hpp"
#include "core/time/Time.hpp"
#include "core/types/Types.hpp"
#include "core/log/Log.hpp"

// Window and Event Systems
#include "ui/window/Manager.hpp"
#include "ui/window/Events.hpp"

// Rendering Systems
#include "render/camera/Camera.hpp"  // Advanced RTS camera system
#include "render/common/Renderer.hpp"
#include "render/common/Types.hpp"
#include "render/common/ProceduralHexRenderer.hpp"  // Modern procedural hex rendering

// World Systems
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
using namespace Manifest::Render::CameraSystem;
using namespace Manifest::UI::Window;
// using namespace Manifest::Render::Examples; // Comment out problematic namespace

// Forward declaration for renderer factory function
namespace Manifest::Render {
    std::unique_ptr<Renderer> create_renderer(API api);
}

// Game state structure
struct GameState {
    std::unique_ptr<TileMap> world_map;
    std::unique_ptr<System> camera_system;  // Advanced RTS camera system
    std::unique_ptr<Render::ProceduralHexRenderer> hex_renderer;  // World tile renderer
    GameTime game_time;
    FrameTimer frame_timer;
    float accumulated_time = 0.0f;
    bool show_debug_info = false;
    
    GameState() : world_map(std::make_unique<TileMap>()) {}
};

void print_banner() {
    LOG_INFO("╔═══════════════════════════════════════════════╗");
    LOG_INFO("║              MANIFEST ENGINE                  ║");
    LOG_INFO("║         Grand Strategy Empire Builder         ║");
    LOG_INFO("║                                               ║");
    LOG_INFO("║    Modern C++23 • OpenGL • Procedural        ║");
    LOG_INFO("╚═══════════════════════════════════════════════╝");
}

// Initialize the world generation system
std::unique_ptr<TileMap> initialize_world() {
    LOG_INFO("Generating procedural world...");
    
    auto world_map = std::make_unique<TileMap>();
    GenerationParams params{
        .seed = 12345,
        .map_radius = 50,  // Larger world for the game
        .sea_level = 0.3f
    };
    
    TerrainGenerator generator(params);
    auto start_time = std::chrono::high_resolution_clock::now();
    
    generator.generate_world(*world_map);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    LOG_INFO("Generated {} tiles in {}ms", world_map->tile_count(), duration.count());
    return world_map;
}

// Handle window and input events
void handle_game_event(WindowHandle window_handle, const Event& event, 
                      GameState& game_state) {
    // Handle game-specific events
    std::visit([&](const auto& e) {
        using T = std::decay_t<decltype(e)>;
        
        if constexpr (std::is_same_v<T, WindowCloseEvent>) {
            LOG_INFO("Window close requested");
        } else if constexpr (std::is_same_v<T, WindowResizeEvent>) {
            LOG_DEBUG("Window resized to {}x{}", e.size.x(), e.size.y());
            // Update camera aspect ratio
            float aspect = static_cast<float>(e.size.x()) / static_cast<float>(e.size.y());
            // Camera aspect will be handled by camera system
        } else if constexpr (std::is_same_v<T, KeyEvent>) {
            if (e.action == Action::Press || e.action == Action::Repeat) {
                switch (e.key) {
                    case KeyCode::F1:
                        game_state.show_debug_info = !game_state.show_debug_info;
                        LOG_INFO("Debug info: {}", game_state.show_debug_info ? "ON" : "OFF");
                        break;
                    case KeyCode::Escape:
                        LOG_INFO("ESC pressed - use window close to exit");
                        break;
                    default:
                        // Let camera system handle all other keys
                        if (game_state.camera_system) {
                            game_state.camera_system->handle_event(event);
                        }
                        break;
                }
            } else {
                // Let camera system handle key releases
                if (game_state.camera_system) {
                    game_state.camera_system->handle_event(event);
                }
            }
        } else {
            // Let camera system handle all other events (mouse, etc.)
            if (game_state.camera_system) {
                game_state.camera_system->handle_event(event);
            }
        }
    }, event);
}


// Update game systems
void update_game_systems(GameState& game_state, float delta_time) {
    // Update frame timer and game time
    game_state.frame_timer.update();
    game_state.game_time.update(std::chrono::milliseconds(static_cast<int>(delta_time * 1000)));
    game_state.accumulated_time += delta_time;
    
    // Update camera system
    if (game_state.camera_system) {
        game_state.camera_system->update(delta_time);
    }
    
    // Log debug info periodically
    if (game_state.show_debug_info && static_cast<int>(game_state.accumulated_time * 2) % 2 == 0) {
        auto fps = game_state.frame_timer.fps();
        if (game_state.camera_system) {
            auto& camera = game_state.camera_system->camera();
            LOG_DEBUG("FPS: {:.1f}, Turn: {}, Year: {}, Camera: ({:.1f}, {:.1f}, {:.1f})", 
                     fps, game_state.game_time.current_turn(), game_state.game_time.current_year(),
                     camera.position().x(), camera.position().y(), camera.position().z());
        }
    }
}

// Initialize advanced RTS camera system
std::unique_ptr<System> initialize_camera_system(const Vec2i& window_size) {
    LOG_INFO("Initializing advanced RTS camera system...");
    
    auto camera_system = std::make_unique<System>();
    
    // Set up RTS controls (orbital mode by default) FIRST
    camera_system->setup_orbital_controls();
    
    // Then set up camera with proper perspective and position
    auto& camera = camera_system->camera();
    camera.set_perspective(60.0f * M_PI / 180.0f,  // Wider field of view to see more
                          static_cast<float>(window_size.x()) / static_cast<float>(window_size.y()), 
                          1.0f, 10000.0f);  // Increase both near and far plane for this scale
    // Position camera to view the hex world (center of generated world)
    // Hexes span X: 9-54 (center ~32), Z: -76 to +69 (center ~-3.5), Y: 0 (ground level)
    camera.look_at(Vec3f{32.0f, 50.0f, -3.5f}, Vec3f{32.0f, 0.0f, -3.5f});
    
    LOG_INFO("Advanced RTS camera system initialized with orbital controls");
    return camera_system;
}


// Create OpenGL renderer instance
std::unique_ptr<Render::Renderer> initialize_renderer(Window* window) {
    LOG_INFO("Initializing OpenGL renderer...");
    
    // Create OpenGL renderer only
    auto renderer = Render::create_renderer(Render::API::OpenGL);
    
    if (!renderer) {
        LOG_ERROR("Failed to create OpenGL renderer instance");
        return nullptr;
    }
    
    // Initialize the renderer
    if (auto result = renderer->initialize(); !result) {
        LOG_ERROR("Failed to initialize OpenGL renderer");
        return nullptr;
    }
    
    LOG_INFO("OpenGL renderer initialized successfully");
    return renderer;
}

// Initialize procedural hex renderer for world visualization
std::unique_ptr<Render::ProceduralHexRenderer> initialize_procedural_hex_renderer(std::unique_ptr<Render::Renderer> renderer) {
    LOG_INFO("Initializing procedural hex renderer for world visualization...");
    
    if (!renderer) {
        LOG_ERROR("No renderer provided to procedural hex renderer");
        return nullptr;
    }
    
    auto hex_renderer = std::make_unique<Render::ProceduralHexRenderer>();
    auto init_result = hex_renderer->initialize(std::move(renderer));
    if (!init_result) {
        LOG_ERROR("Failed to initialize procedural hex renderer");
        return nullptr;
    }
    
    LOG_INFO("Procedural hex renderer initialized successfully");
    return hex_renderer;
}

int main() {
    try {
        // Initialize logging system with multi-sink setup
        auto multi_sink = std::make_shared<Manifest::Core::Log::MultiSink>();
        multi_sink->add_sink(std::make_shared<Manifest::Core::Log::ConsoleSink>(true));
        multi_sink->add_sink(std::make_shared<Manifest::Core::Log::FileSink>("manifest_engine.log"));
        
        auto global_logger = Manifest::Core::Log::Registry::create("Manifest", std::dynamic_pointer_cast<Manifest::Core::Log::Sink>(multi_sink));
        Manifest::Core::Log::Global::set(std::move(global_logger));

        LOG_INFO("=== Manifest Engine Startup ===");
        print_banner();

        // 1. Initialize Window Manager
        LOG_INFO("Initializing window system...");
        Manager window_manager;
        FactoryConfig config{Backend::GLFW};
        if (auto result = window_manager.initialize(config); !result) {
            LOG_FATAL("Failed to initialize window manager");
            return 1;
        }
        
        // 2. Create Main Game Window
        WindowDesc window_desc{
            .size = {1920, 1080},
            .position = {100, 100},
            .title = "Manifest Engine - Grand Strategy Empire Builder",
            .resizable = true,
            .decorated = true,
            .initial_state = WindowState::Visible
        };
        
        auto window_handle = window_manager.create_window(window_desc);
        if (!window_handle) {
            LOG_FATAL("Failed to create main window");
            return 1;
        }
        
        Window* main_window = window_manager.get_window(*window_handle);
        if (!main_window) {
            LOG_FATAL("Failed to get window reference");
            return 1;
        }
        
        LOG_INFO("Main window created: {}x{}", window_desc.size.x(), window_desc.size.y());
        LOG_INFO("Window properties: visible={}, should_close={}, native_handle={}", 
                 main_window->properties().state == WindowState::Visible,
                 main_window->should_close(),
                 main_window->native_handle() != nullptr);
        
        // Ensure window is visible
        if (auto result = main_window->set_state(WindowState::Visible); !result) {
            LOG_WARN("Failed to set window visible state");
        } else {
            LOG_INFO("Window set to visible state");
        }
        
        // 3. Initialize rendering context (Vulkan or OpenGL)
        LOG_INFO("Initializing rendering context...");
        // Context initialization is now handled by the renderer during initialize()
        
        // 4. Initialize Game Systems
        auto world_map = initialize_world();
        auto camera_system = initialize_camera_system(window_desc.size);
        auto renderer = initialize_renderer(main_window);
        
        if (!renderer) {
            LOG_FATAL("Failed to initialize renderer");
            return 1;
        }
        
        // Initialize procedural hex renderer (takes ownership of renderer)
        auto hex_renderer = initialize_procedural_hex_renderer(std::move(renderer));
        
        if (!hex_renderer) {
            LOG_FATAL("Failed to initialize procedural hex renderer");
            return 1;
        }
        
        // 5. Initialize Game State
        GameState game_state;
        game_state.world_map = std::move(world_map);
        game_state.camera_system = std::move(camera_system);
        game_state.hex_renderer = std::move(hex_renderer);
        
        // 6. Set up Event Handling
        window_manager.set_global_event_callback(
            [&](WindowHandle handle, const Event& event) {
                handle_game_event(handle, event, game_state);
            }
        );
        
        LOG_INFO("🎮 Manifest Engine fully initialized! Starting main game loop...");
        LOG_INFO("Controls: WASD = Move, Mouse = RTS Camera, Right-Click = Select, F1 = Debug, ESC = Menu");
        
        // 7. Main Game Loop
        auto last_frame_time = std::chrono::high_resolution_clock::now();
        
        LOG_INFO("Entering main game loop - window should be visible now!");
        LOG_INFO("Window state: should_close = {}, window count = {}", 
                 main_window->should_close(), window_manager.window_count());
        
        while (!window_manager.should_quit()) {
            // Calculate delta time
            auto current_time = std::chrono::high_resolution_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_frame_time).count();
            last_frame_time = current_time;
            
            // Cap delta time to prevent large jumps
            delta_time = std::min(delta_time, 1.0f / 30.0f);
            
            // Poll input events
            window_manager.poll_all_events();
            
            // Update all game systems (no renderer needed for update)
            update_game_systems(game_state, delta_time);
            
            // Render the generated world!
            if (game_state.hex_renderer && game_state.camera_system && game_state.world_map) {
                // Get camera matrices and position
                auto& camera = game_state.camera_system->camera();
                Mat4f view_proj = camera.projection_matrix() * camera.view_matrix();
                Vec3f camera_pos = camera.position();
                
                // Set up basic lighting (sun from above and slightly east)
                Vec3f sun_direction = Vec3f{0.3f, -1.0f, 0.2f}.normalized();
                Vec3f sun_color{1.0f, 0.95f, 0.8f};  // Warm sunlight
                Vec3f ambient_color{0.3f, 0.4f, 0.6f};  // Cool ambient from sky
                float sun_intensity = 1.2f;
                
                // Update hex renderer with camera and lighting
                game_state.hex_renderer->update_globals(view_proj, camera_pos, game_state.accumulated_time);
                game_state.hex_renderer->update_lighting(sun_direction, sun_color, ambient_color, sun_intensity);
                
                // Gather world tiles for rendering
                std::vector<const Tile*> visible_tiles;
                visible_tiles.reserve(game_state.world_map->tile_count());
                
                // Add all tiles to render (in future we could add frustum culling here)
                for (const auto& tile : game_state.world_map->tiles()) {
                    visible_tiles.push_back(&tile);
                }
                
                // Only log occasionally to reduce spam
                static int log_counter = 0;
                if (++log_counter % 60 == 0) {  // Log every ~1 second at 60fps
                    LOG_INFO("Rendering {} tiles, camera at ({}, {}, {})", 
                             visible_tiles.size(), camera_pos.x(), camera_pos.y(), camera_pos.z());
                    
                    // Debug: Log actual hex positions using the real coordinate conversion
                    if (!visible_tiles.empty() && visible_tiles.size() > 5) {
                        for (int i = 0; i < 5; ++i) {
                            auto coord = visible_tiles[i]->coordinate();
                            // Use actual coordinate conversion matching ProceduralHexRenderer
                            float x = 1.0f * 1.5f * coord.q;
                            float z = 1.0f * std::sqrt(3.0f) * (coord.r + coord.q * 0.5f);
                            LOG_INFO("Sample hex {}: coord({}, {}) -> ACTUAL world pos ({}, {})", 
                                     i, coord.q, coord.r, x, z);
                        }
                    }
                }
                
                // Set up viewport
                Viewport viewport{
                    .position = Vec2f{0.0f, 0.0f},
                    .size = Vec2f{static_cast<float>(window_desc.size.x()), static_cast<float>(window_desc.size.y())},
                    .depth_range = Vec2f{0.0f, 1.0f}
                };
                
                // Set clear color (dark background to contrast with hex colors)
                Vec4f clear_color{0.1f, 0.1f, 0.2f, 1.0f};  // Dark blue background
                
                // Render the world using procedural hex renderer
                game_state.hex_renderer->prepare_instances(visible_tiles);
                LOG_INFO("Prepared {} hex instances for rendering", game_state.hex_renderer->instance_count());
                
                auto render_result = game_state.hex_renderer->render_frame(clear_color, viewport);
                if (!render_result) {
                    LOG_WARN("Failed to render world tiles");
                } else {
                    LOG_INFO("Successfully rendered {} hex instances", game_state.hex_renderer->instance_count());
                }
            }
            
            // Present the frame to the window - swap buffers for OpenGL
            if (main_window) {
                main_window->swap_buffers();
            }
        }
        
        LOG_INFO("Shutting down Manifest Engine...");
        
        // Cleanup happens automatically via RAII
        return 0;
        
    } catch (const std::exception& e) {
        LOG_FATAL("Unhandled exception: {}", e.what());
        return 1;
    }
}

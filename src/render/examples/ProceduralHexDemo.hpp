#pragma once

#include <memory>
#include <vector>

#include "../../world/tiles/Tile.hpp"
#include "../common/HexRenderer.hpp"

namespace Manifest {
namespace Render {
namespace Examples {

using namespace Core::Math;
using namespace World::Tiles;

/**
 * Demo class to showcase the new procedural hex rendering system
 * This creates a sample world with different terrain types and elevations
 */
class ProceduralHexDemo {
   private:
    std::unique_ptr<HexRenderer> hex_renderer_;
    std::vector<std::unique_ptr<Tile>> demo_tiles_;

    // Demo parameters
    static constexpr std::int32_t DEMO_SIZE = 50;  // 50x50 hex grid
    static constexpr float DEMO_HEIGHT_VARIATION = 100.0f;

   public:
    ProceduralHexDemo() = default;
    ~ProceduralHexDemo() = default;

    // Non-copyable but movable
    ProceduralHexDemo(const ProceduralHexDemo&) = delete;
    ProceduralHexDemo& operator=(const ProceduralHexDemo&) = delete;
    ProceduralHexDemo(ProceduralHexDemo&&) = default;
    ProceduralHexDemo& operator=(ProceduralHexDemo&&) = default;

    /**
     * Initialize the demo with a renderer
     */
    [[nodiscard]] Result<void> initialize(std::unique_ptr<class Renderer> renderer);

    /**
     * Shutdown and cleanup
     */
    void shutdown();

    /**
     * Update the demo (camera movement, animations, etc.)
     */
    void update(float delta_time);

    /**
     * Render the demo
     */
    [[nodiscard]] Result<void> render();

    /**
     * Get renderer statistics
     */
    [[nodiscard]] RenderStats get_stats() const;

   private:
    void create_demo_world();
    static TerrainType get_terrain_for_coordinate(const HexCoordinate& coord, float elevation);
    static float generate_elevation(const HexCoordinate& coord);
    static float simple_noise(float x, float y);
};

}  // namespace Examples
}  // namespace Render
}  // namespace Manifest

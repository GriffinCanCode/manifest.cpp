#pragma once

#include <memory>
#include <vector>

#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"
#include "ProceduralHexRenderer.hpp"
#include "Types.hpp"

namespace Manifest {
namespace Render {

/**
 * High-level hex rendering interface that automatically chooses between
 * legacy CPU-based and modern GPU-based rendering approaches
 */
class HexRenderer {
   public:
    enum class RenderingMode {
        Auto,       // Automatically choose best approach
        Legacy,     // Force CPU-based rendering (for compatibility)
        Procedural  // Force GPU-based procedural rendering
    };

   private:
    RenderingMode mode_{RenderingMode::Auto};
    std::unique_ptr<ProceduralHexRenderer> procedural_renderer_;
    bool use_procedural_{true};

   public:
    HexRenderer() = default;
    ~HexRenderer() = default;

    // Non-copyable but movable
    HexRenderer(const HexRenderer&) = delete;
    HexRenderer& operator=(const HexRenderer&) = delete;
    HexRenderer(HexRenderer&&) = default;
    HexRenderer& operator=(HexRenderer&&) = default;

    /**
     * Initialize the hex renderer with specified mode
     */
    [[nodiscard]] bool initialize(std::unique_ptr<class Renderer> renderer,
                                  RenderingMode mode = RenderingMode::Auto);

    /**
     * Shutdown and cleanup
     */
    void shutdown();

    /**
     * Update rendering parameters
     */
    void update_camera(const Mat4f& view_proj, const Vec3f& camera_pos, float time = 0.0f);
    void update_lighting(const Vec3f& sun_dir, const Vec3f& sun_color, const Vec3f& ambient_color,
                         float sun_intensity = 1.0f);

    /**
     * Render hex tiles
     */
    [[nodiscard]] bool render_tiles(const std::vector<const World::Tiles::Tile*>& tiles);

    /**
     * Get current rendering mode
     */
    [[nodiscard]] RenderingMode get_mode() const { return mode_; }

    /**
     * Check if using procedural rendering
     */
    [[nodiscard]] bool is_using_procedural() const { return use_procedural_; }

    /**
     * Get render statistics
     */
    [[nodiscard]] RenderStats get_stats() const;

   private:
    void determine_rendering_mode();
    [[nodiscard]] bool render_legacy(const std::vector<const World::Tiles::Tile*>& tiles);
};

}  // namespace Render
}  // namespace Manifest

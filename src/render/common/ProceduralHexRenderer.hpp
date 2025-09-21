#pragma once

#include "Types.hpp"
#include "Renderer.hpp"
#include "../../core/math/Vector.hpp"
#include "../../core/math/Matrix.hpp"
#include "../../world/tiles/Tile.hpp"
#include <vector>
#include <span>
#include <memory>
#include <expected>

namespace Manifest::Render {

using namespace Core::Math;
using namespace World::Tiles;

/**
 * Modern GPU-based hex renderer that generates hex meshes procedurally in vertex shaders
 * instead of pre-generating vertex data on CPU
 */
class ProceduralHexRenderer {
public:
    // Instance data sent to GPU (minimal)
    struct HexInstance {
        Vec3f position;           // World position of hex center
        Vec4f color;             // Terrain color
        float elevation;         // Normalized elevation (0.0-1.0)
        std::uint32_t terrain;   // Terrain type
        
        static constexpr std::size_t size() { 
            return sizeof(Vec3f) + sizeof(Vec4f) + sizeof(float) + sizeof(std::uint32_t); 
        }
    };
    
    // Global uniform data for rendering
    struct GlobalUniforms {
        Mat4f view_projection_matrix;
        Vec3f camera_position;
        float hex_radius{1.0f};
        float height_scale{0.1f};
        float time{0.0f};
        Vec2f _padding{};
    };
    
    // Lighting uniform data
    struct LightingUniforms {
        Vec3f sun_direction{0.0f, -1.0f, 0.0f};
        Vec3f sun_color{1.0f, 0.95f, 0.8f};
        Vec3f ambient_color{0.2f, 0.3f, 0.5f};
        float sun_intensity{1.0f};
        Vec3f camera_position;
        float _padding{0.0f};
    };

private:
    static constexpr std::uint32_t VERTICES_PER_HEX = 7;  // Center + 6 edge vertices
    static constexpr std::uint32_t MAX_INSTANCES = 1000000; // Support up to 1M hex tiles
    
    // Renderer reference
    std::unique_ptr<class Renderer> renderer_;
    
    // GPU resources
    BufferHandle instance_buffer_;
    BufferHandle global_uniforms_buffer_;
    BufferHandle lighting_uniforms_buffer_;
    ShaderHandle vertex_shader_;
    ShaderHandle fragment_shader_;
    PipelineHandle render_pipeline_;
    
    // Current frame data
    std::vector<HexInstance> current_instances_;
    GlobalUniforms global_uniforms_;
    LightingUniforms lighting_uniforms_;
    
    bool is_initialized_{false};
    
public:
    ProceduralHexRenderer() = default;
    ~ProceduralHexRenderer() = default;
    
    // Non-copyable but movable
    ProceduralHexRenderer(const ProceduralHexRenderer&) = delete;
    ProceduralHexRenderer& operator=(const ProceduralHexRenderer&) = delete;
    ProceduralHexRenderer(ProceduralHexRenderer&&) = default;
    ProceduralHexRenderer& operator=(ProceduralHexRenderer&&) = default;
    
    /**
     * Initialize the procedural hex renderer
     */
    [[nodiscard]] Result<void> initialize(std::unique_ptr<Renderer> renderer);
    
    /**
     * Shutdown and cleanup resources
     */
    void shutdown();
    
    /**
     * Update global rendering parameters
     */
    void update_globals(const Mat4f& view_proj, const Vec3f& camera_pos, float time = 0.0f);
    
    /**
     * Update lighting parameters
     */
    void update_lighting(const Vec3f& sun_dir, const Vec3f& sun_color, 
                        const Vec3f& ambient_color, float sun_intensity = 1.0f);
    
    /**
     * Prepare instances for rendering from tile data
     */
    void prepare_instances(std::span<const Tile* const> tiles);
    
    /**
     * Add a single hex instance for rendering
     */
    void add_instance(const Vec3f& position, const Vec4f& color, 
                     float elevation, std::uint32_t terrain_type);
    
    /**
     * Clear all pending instances
     */
    void clear_instances();
    
    /**
     * Render all prepared instances
     */
    [[nodiscard]] Result<void> render();
    
    /**
     * Get current instance count
     */
    [[nodiscard]] std::size_t instance_count() const { return current_instances_.size(); }
    
    /**
     * Get max supported instances
     */
    [[nodiscard]] static constexpr std::size_t max_instances() { return MAX_INSTANCES; }

private:
    [[nodiscard]] Result<void> create_shaders();
    [[nodiscard]] Result<void> create_buffers();
    [[nodiscard]] Result<void> create_pipeline();
    
    [[nodiscard]] Result<void> upload_instances();
    [[nodiscard]] Result<void> upload_uniforms();
    
    static Vec4f get_terrain_color(TerrainType terrain, float elevation);
    static Vec3f hex_coord_to_world(const HexCoordinate& coord, float hex_radius = 1.0f);
};

// Terrain color utilities
inline Vec4f ProceduralHexRenderer::get_terrain_color(TerrainType terrain, float elevation) {
    switch (terrain) {
        case TerrainType::Ocean:
            return Vec4f{0.1f, 0.2f, 0.6f, 1.0f} * (1.0f - elevation * 0.3f);
        case TerrainType::Grassland:
            return Vec4f{0.3f, 0.7f, 0.2f, 1.0f} * (1.0f + elevation * 0.2f);
        case TerrainType::Forest:
            return Vec4f{0.1f, 0.4f, 0.1f, 1.0f} * (1.0f + elevation * 0.3f);
        case TerrainType::Desert:
            return Vec4f{0.8f, 0.7f, 0.3f, 1.0f} * (1.0f - elevation * 0.1f);
        case TerrainType::Mountains:
            return Vec4f{0.5f, 0.4f, 0.3f, 1.0f} + Vec4f{0.2f, 0.2f, 0.2f, 0.0f} * elevation;
        case TerrainType::Tundra:
            return Vec4f{0.8f, 0.8f, 0.9f, 1.0f} + Vec4f{0.1f, 0.1f, 0.1f, 0.0f} * elevation;
        default:
            return Vec4f{0.5f, 0.5f, 0.5f, 1.0f}; // Gray fallback
    }
}

inline Vec3f ProceduralHexRenderer::hex_coord_to_world(const HexCoordinate& coord, float hex_radius) {
    float x = hex_radius * 1.5f * coord.q;
    float z = hex_radius * std::sqrt(3.0f) * (coord.r + coord.q * 0.5f);
    return Vec3f{x, 0.0f, z};
}

} // namespace Manifest::Render

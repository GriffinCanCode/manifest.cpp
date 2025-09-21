#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include "../../core/math/Vector.hpp"
#include "../../world/tiles/Tile.hpp"
#include "ProceduralHexRenderer.hpp"

namespace Manifest {
namespace Render {

using namespace Core::Math;
using namespace World::Tiles;

struct Vertex {
    Vec3f position{};
    Vec3f normal{};
    Vec2f uv{};
    Vec4f color{1.0f, 1.0f, 1.0f, 1.0f};

    bool operator==(const Vertex& other) const noexcept {
        return position == other.position && normal == other.normal && uv == other.uv &&
               color == other.color;
    }
};

/**
 * Legacy CPU-based hex mesh generator (deprecated - use ProceduralHexRenderer for new code)
 * Maintained for compatibility and fallback scenarios
 */
class [[deprecated("Use ProceduralHexRenderer for GPU-based rendering")]] HexMeshGenerator {
    static constexpr float HEX_RADIUS = 1.0f;
    static constexpr float HEIGHT_SCALE = 0.1f;
    static constexpr std::size_t HEX_SIDES = 6;

   public:
    struct HexMesh {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        void clear() {
            vertices.clear();
            indices.clear();
        }

        std::size_t vertex_count() const noexcept { return vertices.size(); }
        std::size_t index_count() const noexcept { return indices.size(); }
        std::size_t triangle_count() const noexcept { return indices.size() / 3; }
    };

    // Generate a single hex tile mesh
    static HexMesh generate_hex_tile(const HexCoordinate& coord, float elevation = 0.0f) {
        HexMesh mesh;

        Vec2f center = hex_coord_to_world(coord);
        float height = elevation * HEIGHT_SCALE;

        // Generate top face
        generate_hex_face(mesh, center, height, true);

        // Generate side faces if elevated
        if (height > 0.0f) {
            generate_hex_sides(mesh, center, height);
        }

        return mesh;
    }

    // Generate mesh for multiple tiles with proper connectivity
    static HexMesh generate_tile_mesh(const std::vector<Tile*>& tiles) {
        HexMesh combined_mesh;

        for (const Tile* tile : tiles) {
            if (!tile) continue;

            float elevation = tile->elevation() / 255.0f;
            HexMesh tile_mesh = generate_hex_tile(tile->coordinate(), elevation);

            // Apply terrain-based coloring
            apply_terrain_coloring(tile_mesh, tile->terrain(), elevation);

            // Merge into combined mesh
            merge_mesh(combined_mesh, tile_mesh);
        }

        return combined_mesh;
    }

    // Generate instancing data for efficient rendering
    struct InstanceData {
        Vec3f position{};
        Vec4f color{};
        float elevation{};
        std::uint32_t terrain_type{};

        bool operator==(const InstanceData& other) const noexcept {
            return position == other.position && color == other.color &&
                   elevation == other.elevation && terrain_type == other.terrain_type;
        }
    };

    static std::vector<InstanceData> generate_instance_data(const std::vector<Tile*>& tiles) {
        std::vector<InstanceData> instances;
        instances.reserve(tiles.size());

        for (const Tile* tile : tiles) {
            if (!tile) continue;

            Vec2f world_pos = hex_coord_to_world(tile->coordinate());
            float elevation = tile->elevation() / 255.0f;
            Vec4f color = get_terrain_color(tile->terrain(), elevation);

            instances.push_back(InstanceData{
                .position = Vec3f{world_pos.x(), elevation * HEIGHT_SCALE, world_pos.y()},
                .color = color,
                .elevation = elevation,
                .terrain_type = static_cast<std::uint32_t>(tile->terrain())});
        }

        return instances;
    }

    // Get base hex mesh (single unit hex at origin)
    static HexMesh get_base_hex_mesh() {
        static HexMesh base_mesh = create_base_hex();
        return base_mesh;
    }

    /**
     * Create a ProceduralHexRenderer for modern GPU-based rendering
     * This is the recommended approach for new code
     */
    [[nodiscard]] static std::unique_ptr<ProceduralHexRenderer> create_procedural_renderer() {
        return std::make_unique<ProceduralHexRenderer>();
    }

    /**
     * Convert legacy instance data to procedural renderer format
     */
    static void populate_procedural_renderer(ProceduralHexRenderer& renderer,
                                             const std::vector<Tile*>& tiles) {
        renderer.clear_instances();
        renderer.prepare_instances(std::span<const Tile* const>{tiles.data(), tiles.size()});
    }

   private:
    static Vec2f hex_coord_to_world(const HexCoordinate& coord) {
        // Convert hex coordinates to world position
        float x = HEX_RADIUS * 1.5f * coord.q;
        float z = HEX_RADIUS * std::sqrt(3.0f) * (coord.r + coord.q * 0.5f);
        return Vec2f{x, z};
    }

    static void generate_hex_face(HexMesh& mesh, const Vec2f& center, float height, bool is_top) {
        std::size_t start_vertex = mesh.vertices.size();

        // Center vertex
        mesh.vertices.push_back(Vertex{.position = Vec3f{center.x(), height, center.y()},
                                       .normal = Vec3f{0.0f, is_top ? 1.0f : -1.0f, 0.0f},
                                       .uv = Vec2f{0.5f, 0.5f}});

        // Edge vertices
        for (std::size_t i = 0; i < HEX_SIDES; ++i) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159265359f) / HEX_SIDES;
            float x = center.x() + HEX_RADIUS * std::cos(angle);
            float z = center.y() + HEX_RADIUS * std::sin(angle);

            float u = 0.5f + 0.5f * std::cos(angle);
            float v = 0.5f + 0.5f * std::sin(angle);

            mesh.vertices.push_back(Vertex{.position = Vec3f{x, height, z},
                                           .normal = Vec3f{0.0f, is_top ? 1.0f : -1.0f, 0.0f},
                                           .uv = Vec2f{u, v}});
        }

        // Generate triangles
        for (std::size_t i = 0; i < HEX_SIDES; ++i) {
            std::uint32_t center_idx = static_cast<std::uint32_t>(start_vertex);
            std::uint32_t curr_idx = static_cast<std::uint32_t>(start_vertex + 1 + i);
            std::uint32_t next_idx =
                static_cast<std::uint32_t>(start_vertex + 1 + ((i + 1) % HEX_SIDES));

            if (is_top) {
                mesh.indices.insert(mesh.indices.end(), {center_idx, curr_idx, next_idx});
            } else {
                mesh.indices.insert(mesh.indices.end(), {center_idx, next_idx, curr_idx});
            }
        }
    }

    static void generate_hex_sides(HexMesh& mesh, const Vec2f& center, float height) {
        std::size_t start_vertex = mesh.vertices.size();

        // Generate side vertices (top and bottom rings)
        for (std::size_t i = 0; i < HEX_SIDES; ++i) {
            float angle = static_cast<float>(i) * (2.0f * 3.14159265359f) / HEX_SIDES;
            float x = center.x() + HEX_RADIUS * std::cos(angle);
            float z = center.y() + HEX_RADIUS * std::sin(angle);

            Vec3f edge_normal = Vec3f{std::cos(angle), 0.0f, std::sin(angle)};

            // Top vertex
            mesh.vertices.push_back(Vertex{.position = Vec3f{x, height, z},
                                           .normal = edge_normal,
                                           .uv = Vec2f{static_cast<float>(i) / HEX_SIDES, 1.0f}});

            // Bottom vertex
            mesh.vertices.push_back(Vertex{.position = Vec3f{x, 0.0f, z},
                                           .normal = edge_normal,
                                           .uv = Vec2f{static_cast<float>(i) / HEX_SIDES, 0.0f}});
        }

        // Generate side face triangles
        for (std::size_t i = 0; i < HEX_SIDES; ++i) {
            std::size_t next_i = (i + 1) % HEX_SIDES;

            std::uint32_t top_curr = static_cast<std::uint32_t>(start_vertex + i * 2);
            std::uint32_t bottom_curr = static_cast<std::uint32_t>(start_vertex + i * 2 + 1);
            std::uint32_t top_next = static_cast<std::uint32_t>(start_vertex + next_i * 2);
            std::uint32_t bottom_next = static_cast<std::uint32_t>(start_vertex + next_i * 2 + 1);

            // Two triangles per side face
            mesh.indices.insert(mesh.indices.end(), {top_curr, bottom_curr, top_next, top_next,
                                                     bottom_curr, bottom_next});
        }
    }

    static void apply_terrain_coloring(HexMesh& mesh, TerrainType terrain, float elevation) {
        Vec4f color = get_terrain_color(terrain, elevation);

        for (Vertex& vertex : mesh.vertices) {
            vertex.color = color;
        }
    }

    static Vec4f get_terrain_color(TerrainType terrain, float elevation) {
        switch (terrain) {
            case TerrainType::Ocean:
                return Vec4f{0.1f, 0.3f, 0.8f, 1.0f};  // Deep blue
            case TerrainType::Coast:
                return Vec4f{0.2f, 0.5f, 0.9f, 1.0f};  // Light blue
            case TerrainType::Plains:
                return Vec4f{0.6f, 0.8f, 0.3f, 1.0f};  // Light green
            case TerrainType::Grassland:
                return Vec4f{0.4f, 0.7f, 0.2f, 1.0f};  // Green
            case TerrainType::Hills:
                return Vec4f{0.5f + (0.7f - 0.5f) * elevation, 0.6f + (0.7f - 0.6f) * elevation,
                             0.3f + (0.6f - 0.3f) * elevation, 1.0f};
            case TerrainType::Mountains:
                return Vec4f{0.6f + (0.9f - 0.6f) * elevation, 0.6f + (0.9f - 0.6f) * elevation,
                             0.5f + (0.9f - 0.5f) * elevation, 1.0f};
            case TerrainType::Desert:
                return Vec4f{0.9f, 0.8f, 0.5f, 1.0f};  // Sandy
            case TerrainType::Tundra:
                return Vec4f{0.7f, 0.8f, 0.7f, 1.0f};  // Light gray-green
            case TerrainType::Snow:
                return Vec4f{0.95f, 0.95f, 1.0f, 1.0f};  // White
            case TerrainType::Forest:
                return Vec4f{0.2f, 0.5f, 0.1f, 1.0f};  // Dark green
            case TerrainType::Jungle:
                return Vec4f{0.1f, 0.4f, 0.1f, 1.0f};  // Very dark green
            case TerrainType::Marsh:
                return Vec4f{0.3f, 0.5f, 0.3f, 1.0f};  // Muddy green
            case TerrainType::Lake:
                return Vec4f{0.2f, 0.4f, 0.7f, 1.0f};  // Lake blue
            case TerrainType::River:
                return Vec4f{0.3f, 0.5f, 0.8f, 1.0f};  // River blue
            default:
                return Vec4f{0.5f, 0.5f, 0.5f, 1.0f};  // Gray
        }
    }

    static void merge_mesh(HexMesh& target, const HexMesh& source) {
        std::uint32_t vertex_offset = static_cast<std::uint32_t>(target.vertices.size());

        // Add vertices
        target.vertices.insert(target.vertices.end(), source.vertices.begin(),
                               source.vertices.end());

        // Add indices with offset
        for (std::uint32_t index : source.indices) {
            target.indices.push_back(index + vertex_offset);
        }
    }

    static HexMesh create_base_hex() {
        HexMesh mesh;
        generate_hex_face(mesh, Vec2f{0.0f, 0.0f}, 0.0f, true);
        return mesh;
    }
};

// Level-of-detail mesh generator
class HexLODGenerator {
   public:
    enum class LODLevel : std::uint8_t {
        Tile = 0,      // Individual tiles
        Province = 1,  // Province boundaries
        Region = 2,    // Region boundaries
        Continent = 3  // Continental view
    };

    static HexMeshGenerator::HexMesh generate_lod_mesh(LODLevel level,
                                                       const std::vector<Tile*>& tiles) {
        switch (level) {
            case LODLevel::Tile:
                return HexMeshGenerator::generate_tile_mesh(tiles);

            case LODLevel::Province:
                return generate_province_mesh(tiles);

            case LODLevel::Region:
                return generate_region_mesh(tiles);

            case LODLevel::Continent:
                return generate_continent_mesh(tiles);

            default:
                return HexMeshGenerator::generate_tile_mesh(tiles);
        }
    }

   private:
    static HexMeshGenerator::HexMesh generate_province_mesh(const std::vector<Tile*>& tiles) {
        // Simplified mesh for province-level view
        // Group nearby tiles and create larger hexes
        HexMeshGenerator::HexMesh mesh;

        // For now, just use regular tile mesh with reduced detail
        return HexMeshGenerator::generate_tile_mesh(tiles);
    }

    static HexMeshGenerator::HexMesh generate_region_mesh(const std::vector<Tile*>& tiles) {
        // Even more simplified for region view
        HexMeshGenerator::HexMesh mesh;

        // Create very simplified representation
        return HexMeshGenerator::generate_tile_mesh(tiles);
    }

    static HexMeshGenerator::HexMesh generate_continent_mesh(const std::vector<Tile*>& tiles) {
        // Minimal detail for continental view
        HexMeshGenerator::HexMesh mesh;

        // Just show major terrain features
        return HexMeshGenerator::generate_tile_mesh(tiles);
    }
};

}  // namespace Render
}  // namespace Manifest

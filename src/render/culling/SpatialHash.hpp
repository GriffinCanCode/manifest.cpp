#pragma once

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "../../core/math/Vector.hpp"
#include "../../world/tiles/Tile.hpp"
#include "FrustumCuller.hpp"

namespace Manifest {
namespace Render {
namespace Culling {

using namespace Core::Math;
using namespace World::Tiles;

/**
 * Enhanced hierarchical spatial hash system optimized for frustum culling.
 * Provides efficient spatial queries for millions of hex tiles with multiple
 * level-of-detail layers.
 */
class SpatialHash {
public:
    /**
     * Spatial bucket containing tile references and spatial bounds
     */
    struct Bucket {
        std::vector<const Tile*> tiles;
        Vec3f min_bounds{std::numeric_limits<float>::max()};
        Vec3f max_bounds{std::numeric_limits<float>::lowest()};
        Vec3f center{};
        float radius{0.0f};
        
        void update_bounds() noexcept {
            if (tiles.empty()) {
                min_bounds = max_bounds = center = Vec3f{};
                radius = 0.0f;
                return;
            }
            
            min_bounds = Vec3f{std::numeric_limits<float>::max()};
            max_bounds = Vec3f{std::numeric_limits<float>::lowest()};
            
            for (const Tile* tile : tiles) {
                Vec3f world_pos = hex_coord_to_world(tile->coordinate());
                min_bounds = Vec3f{
                    std::min(min_bounds.x(), world_pos.x()),
                    std::min(min_bounds.y(), world_pos.y()),
                    std::min(min_bounds.z(), world_pos.z())
                };
                max_bounds = Vec3f{
                    std::max(max_bounds.x(), world_pos.x()),
                    std::max(max_bounds.y(), world_pos.y()),
                    std::max(max_bounds.z(), world_pos.z())
                };
            }
            
            center = (min_bounds + max_bounds) * 0.5f;
            radius = distance(center, max_bounds);
        }
        
    private:
        static Vec3f hex_coord_to_world(const HexCoordinate& coord, float hex_radius = 1.0f) noexcept {
            // Convert hex coordinates to world position
            float x = hex_radius * (3.0f / 2.0f * coord.q);
            float z = hex_radius * (std::sqrt(3.0f) / 2.0f * coord.q + std::sqrt(3.0f) * coord.r);
            return Vec3f{x, 0.0f, z};
        }
    };
    
    /**
     * Spatial query result containing visible tiles after culling
     */
    struct QueryResult {
        std::vector<const Tile*> visible_tiles;
        std::vector<const Tile*> intersecting_tiles;
        std::size_t total_tested{0};
        std::size_t total_culled{0};
        
        void clear() noexcept {
            visible_tiles.clear();
            intersecting_tiles.clear();
            total_tested = 0;
            total_culled = 0;
        }
        
        std::size_t total_visible() const noexcept {
            return visible_tiles.size() + intersecting_tiles.size();
        }
    };
    
private:
    // Multi-level spatial buckets for hierarchical culling
    struct Level {
        std::unordered_map<std::int64_t, Bucket> buckets;
        std::int32_t bucket_size;
        
        Level(std::int32_t size) : bucket_size(size) {}
    };
    
    std::vector<Level> levels_;
    float hex_radius_{1.0f};
    bool needs_rebuild_{true};
    
    // Statistics for performance monitoring
    mutable std::size_t last_query_buckets_tested_{0};
    mutable std::size_t last_query_tiles_tested_{0};
    mutable std::size_t last_query_tiles_culled_{0};
    
    [[nodiscard]] std::int64_t get_bucket_key(const Vec3f& world_pos, std::int32_t bucket_size) const noexcept;
    
    void insert_tile_into_level(Level& level, const Tile* tile) noexcept;
    
    Vec3f hex_coord_to_world(const HexCoordinate& coord) const noexcept {
        float x = hex_radius_ * (3.0f / 2.0f * coord.q);
        float z = hex_radius_ * (std::sqrt(3.0f) / 2.0f * coord.q + std::sqrt(3.0f) * coord.r);
        return Vec3f{x, 0.0f, z};
    }
    
public:
    /**
     * Initialize spatial hash with hierarchical levels
     * @param hex_radius Radius of hex tiles
     * @param base_bucket_size Base bucket size for finest level
     * @param num_levels Number of hierarchical levels (default: 3)
     */
    explicit SpatialHash(float hex_radius = 1.0f, std::int32_t base_bucket_size = 16, 
                        std::size_t num_levels = 3);
    
    ~SpatialHash() = default;
    
    // Non-copyable but movable
    SpatialHash(const SpatialHash&) = delete;
    SpatialHash& operator=(const SpatialHash&) = delete;
    SpatialHash(SpatialHash&&) = default;
    SpatialHash& operator=(SpatialHash&&) = default;
    
    /**
     * Rebuild spatial hash from tile collection
     * Call this when tiles are added/removed or positions change
     */
    void rebuild(const std::vector<const Tile*>& tiles);
    
    /**
     * Perform frustum culling query against spatial hash
     * Returns tiles that are visible or intersecting with the frustum
     */
    void query_frustum(const FrustumCuller& culler, QueryResult& result) const;
    
    /**
     * Query tiles within a sphere (for distance-based culling)
     */
    void query_sphere(const Vec3f& center, float radius, std::vector<const Tile*>& result) const;
    
    /**
     * Query tiles within an axis-aligned bounding box
     */
    void query_aabb(const Vec3f& min_point, const Vec3f& max_point, 
                   std::vector<const Tile*>& result) const;
    
    /**
     * Get approximate tile count (useful for pre-sizing containers)
     */
    [[nodiscard]] std::size_t get_tile_count() const noexcept;
    
    /**
     * Get query statistics from last frustum query
     */
    [[nodiscard]] std::size_t get_last_buckets_tested() const noexcept { return last_query_buckets_tested_; }
    [[nodiscard]] std::size_t get_last_tiles_tested() const noexcept { return last_query_tiles_tested_; }
    [[nodiscard]] std::size_t get_last_tiles_culled() const noexcept { return last_query_tiles_culled_; }
    
    /**
     * Update hex radius (triggers rebuild on next query)
     */
    void set_hex_radius(float radius) noexcept {
        if (hex_radius_ != radius) {
            hex_radius_ = radius;
            needs_rebuild_ = true;
        }
    }
    
    /**
     * Check if spatial hash needs rebuilding
     */
    [[nodiscard]] bool needs_rebuild() const noexcept { return needs_rebuild_; }
};

}  // namespace Culling
}  // namespace Render
}  // namespace Manifest

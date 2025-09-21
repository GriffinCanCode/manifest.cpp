#include "SpatialHash.hpp"

#include <cmath>

namespace Manifest {
namespace Render {  
namespace Culling {

SpatialHash::SpatialHash(float hex_radius, std::int32_t base_bucket_size, std::size_t num_levels)
    : hex_radius_(hex_radius) {
    
    levels_.reserve(num_levels);
    
    // Create hierarchical levels with exponentially increasing bucket sizes
    for (std::size_t i = 0; i < num_levels; ++i) {
        std::int32_t bucket_size = base_bucket_size * (1 << i); // 16, 32, 64, etc.
        levels_.emplace_back(bucket_size);
    }
}

void SpatialHash::rebuild(const std::vector<const Tile*>& tiles) {
    // Clear all levels
    for (auto& level : levels_) {
        level.buckets.clear();
    }
    
    // Insert tiles into all levels
    for (const Tile* tile : tiles) {
        if (!tile) continue;
        
        for (auto& level : levels_) {
            insert_tile_into_level(level, tile);
        }
    }
    
    // Update bounds for all buckets
    for (auto& level : levels_) {
        for (auto& bucket_entry : level.buckets) {
            bucket_entry.second.update_bounds();
        }
    }
    
    needs_rebuild_ = false;
}

std::int64_t SpatialHash::get_bucket_key(const Vec3f& world_pos, std::int32_t bucket_size) const noexcept {
    // Convert world position to bucket coordinates
    std::int32_t bucket_x = static_cast<std::int32_t>(std::floor(world_pos.x() / (hex_radius_ * bucket_size)));
    std::int32_t bucket_z = static_cast<std::int32_t>(std::floor(world_pos.z() / (hex_radius_ * bucket_size)));
    
    // Pack into 64-bit key
    return (static_cast<std::int64_t>(bucket_x) << 32) | static_cast<std::int64_t>(bucket_z);
}

void SpatialHash::insert_tile_into_level(Level& level, const Tile* tile) noexcept {
    Vec3f world_pos = hex_coord_to_world(tile->coordinate());
    std::int64_t bucket_key = get_bucket_key(world_pos, level.bucket_size);
    
    level.buckets[bucket_key].tiles.push_back(tile);
}

void SpatialHash::query_frustum(const FrustumCuller& culler, QueryResult& result) const {
    result.clear();
    
    // Reset statistics
    last_query_buckets_tested_ = 0;
    last_query_tiles_tested_ = 0;
    last_query_tiles_culled_ = 0;
    
    if (needs_rebuild_ || levels_.empty()) {
        return;
    }
    
    // Use coarsest level for initial broad-phase culling, then refine
    const Level& coarse_level = levels_.back();
    const Level& fine_level = levels_.front();
    
    // First pass: test coarse buckets
    std::vector<std::int64_t> potentially_visible_buckets;
    potentially_visible_buckets.reserve(coarse_level.buckets.size() / 4); // Estimate
    
    for (const auto& bucket_entry : coarse_level.buckets) {
        ++last_query_buckets_tested_;
        
        const auto& key = bucket_entry.first;
        const auto& bucket = bucket_entry.second;
        
        if (bucket.tiles.empty()) continue;
        
        // Test bucket bounding sphere against frustum
        auto cull_result = culler.test_sphere(bucket.center, bucket.radius);
        
        if (cull_result != FrustumCuller::CullResult::Outside) {
            if (cull_result == FrustumCuller::CullResult::Inside) {
                // Entire bucket is inside frustum - add all tiles
                for (const Tile* tile : bucket.tiles) {
                    result.visible_tiles.push_back(tile);
                }
                last_query_tiles_tested_ += bucket.tiles.size();
            } else {
                // Bucket intersects - need fine-grained testing
                potentially_visible_buckets.push_back(key);
            }
        } else {
            last_query_tiles_culled_ += bucket.tiles.size();
        }
    }
    
    // Second pass: fine-grained culling on intersecting buckets
    for (std::int64_t coarse_key : potentially_visible_buckets) {
        const Bucket& coarse_bucket = coarse_level.buckets.at(coarse_key);
        
        // Test individual tiles in this coarse bucket
        // Use fine level buckets if available for better spatial coherence
        for (const Tile* tile : coarse_bucket.tiles) {
            ++last_query_tiles_tested_;
            
            Vec3f tile_world_pos = hex_coord_to_world(tile->coordinate());
            
            // Test tile with a small bounding sphere (hex tiles are roughly circular)
            float tile_radius = hex_radius_ * 0.6f; // Approximation for hex inscribed circle
            auto cull_result = culler.test_sphere(tile_world_pos, tile_radius);
            
            if (cull_result != FrustumCuller::CullResult::Outside) {
                if (cull_result == FrustumCuller::CullResult::Inside) {
                    result.visible_tiles.push_back(tile);
                } else {
                    result.intersecting_tiles.push_back(tile);
                }
            } else {
                ++last_query_tiles_culled_;
            }
        }
    }
    
    result.total_tested = last_query_tiles_tested_;
    result.total_culled = last_query_tiles_culled_;
}

void SpatialHash::query_sphere(const Vec3f& center, float radius, std::vector<const Tile*>& result) const {
    result.clear();
    
    if (needs_rebuild_ || levels_.empty()) {
        return;
    }
    
    // Use finest level for sphere queries
    const Level& level = levels_.front();
    float search_radius = radius + hex_radius_; // Add tile radius for conservative test
    
    // Calculate bucket range to search
    std::int32_t bucket_range = static_cast<std::int32_t>(std::ceil(search_radius / (hex_radius_ * level.bucket_size))) + 1;
    
    Vec3f center_bucket_pos = center / (hex_radius_ * level.bucket_size);
    std::int32_t center_x = static_cast<std::int32_t>(std::floor(center_bucket_pos.x()));
    std::int32_t center_z = static_cast<std::int32_t>(std::floor(center_bucket_pos.z()));
    
    // Search in a square region around the center
    for (std::int32_t x = center_x - bucket_range; x <= center_x + bucket_range; ++x) {
        for (std::int32_t z = center_z - bucket_range; z <= center_z + bucket_range; ++z) {
            std::int64_t key = (static_cast<std::int64_t>(x) << 32) | static_cast<std::int64_t>(z);
            
            auto it = level.buckets.find(key);
            if (it == level.buckets.end()) continue;
            
            const Bucket& bucket = it->second;
            
            // Test each tile in bucket
            for (const Tile* tile : bucket.tiles) {
                Vec3f tile_pos = hex_coord_to_world(tile->coordinate());
                float dist = distance(center, tile_pos);
                
                if (dist <= radius + hex_radius_ * 0.6f) { // Include tile radius
                    result.push_back(tile);
                }
            }
        }
    }
}

void SpatialHash::query_aabb(const Vec3f& min_point, const Vec3f& max_point, 
                            std::vector<const Tile*>& result) const {
    result.clear();
    
    if (needs_rebuild_ || levels_.empty()) {
        return;
    }
    
    // Use finest level for AABB queries
    const Level& level = levels_.front();
    
    // Calculate bucket range for AABB
    Vec3f expanded_min = min_point - Vec3f{hex_radius_};
    Vec3f expanded_max = max_point + Vec3f{hex_radius_};
    
    std::int32_t min_x = static_cast<std::int32_t>(std::floor(expanded_min.x() / (hex_radius_ * level.bucket_size)));
    std::int32_t max_x = static_cast<std::int32_t>(std::ceil(expanded_max.x() / (hex_radius_ * level.bucket_size)));
    std::int32_t min_z = static_cast<std::int32_t>(std::floor(expanded_min.z() / (hex_radius_ * level.bucket_size)));
    std::int32_t max_z = static_cast<std::int32_t>(std::ceil(expanded_max.z() / (hex_radius_ * level.bucket_size)));
    
    // Search in bucket range
    for (std::int32_t x = min_x; x <= max_x; ++x) {
        for (std::int32_t z = min_z; z <= max_z; ++z) {
            std::int64_t key = (static_cast<std::int64_t>(x) << 32) | static_cast<std::int64_t>(z);
            
            auto it = level.buckets.find(key);
            if (it == level.buckets.end()) continue;
            
            const Bucket& bucket = it->second;
            
            // Test each tile in bucket
            for (const Tile* tile : bucket.tiles) {
                Vec3f tile_pos = hex_coord_to_world(tile->coordinate());
                
                if (tile_pos.x() >= expanded_min.x() && tile_pos.x() <= expanded_max.x() &&
                    tile_pos.z() >= expanded_min.z() && tile_pos.z() <= expanded_max.z()) {
                    result.push_back(tile);
                }
            }
        }
    }
}

std::size_t SpatialHash::get_tile_count() const noexcept {
    if (levels_.empty()) return 0;
    
    std::size_t total = 0;
    for (const auto& bucket_entry : levels_.front().buckets) {
        total += bucket_entry.second.tiles.size();
    }
    return total;
}

}  // namespace Culling
}  // namespace Render
}  // namespace Manifest

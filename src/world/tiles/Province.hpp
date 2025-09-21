#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../core/types/Types.hpp"
#include "Map.hpp"
#include "Tile.hpp"

namespace Manifest {
namespace World {
namespace Tiles {

using namespace Core::Types;

// Forward declarations
class Tile;
class TileMap;
enum class TerrainType : std::uint8_t;
enum class Feature : std::uint8_t;

// Simple 2D point for province centers
struct Point2f {
    float x{0.0f}, y{0.0f};

    Point2f() = default;
    Point2f(float x_, float y_) : x{x_}, y{y_} {}

    Point2f operator+(const Point2f& other) const { return {x + other.x, y + other.y}; }
    Point2f operator*(float scalar) const { return {x * scalar, y * scalar}; }
    Point2f operator/(float scalar) const { return {x / scalar, y / scalar}; }
};

// Distance function for points
inline float distance(const Point2f& a, const Point2f& b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

class Province {
    ProvinceId id_{ProvinceId::invalid()};
    std::vector<TileId> tiles_;
    Point2f center_{};
    std::unordered_set<ProvinceId> neighbors_;

    // Aggregate properties
    TerrainType dominant_terrain_{TerrainType::Ocean};
    std::uint32_t total_elevation_{0};
    std::uint32_t total_temperature_{0};
    std::uint32_t total_rainfall_{0};
    std::uint32_t total_fertility_{0};

    // Economic aggregates
    Core::Types::Production total_food_{};
    Core::Types::Production total_production_{};
    Core::Types::Production total_science_{};
    Core::Types::Production total_culture_{};
    Core::Types::Production total_faith_{};
    Core::Types::Money total_trade_{};

    // Administrative
    NationId owner_{NationId::invalid()};
    CityId capital_city_{CityId::invalid()};
    bool coastal_{false};

    mutable bool needs_update_{true};

   public:
    explicit Province(ProvinceId id) : id_{id} {}

    // Basic properties
    ProvinceId id() const noexcept { return id_; }
    const Point2f& center() const noexcept { return center_; }

    // Tile management
    void add_tile(TileId tile_id) {
        tiles_.push_back(tile_id);
        needs_update_ = true;
    }

    void remove_tile(TileId tile_id) {
        tiles_.erase(std::remove(tiles_.begin(), tiles_.end(), tile_id), tiles_.end());
        needs_update_ = true;
    }

    const std::vector<TileId>& tiles() const noexcept { return tiles_; }
    std::size_t tile_count() const noexcept { return tiles_.size(); }

    // Neighbor management
    void add_neighbor(ProvinceId neighbor) { neighbors_.insert(neighbor); }

    void remove_neighbor(ProvinceId neighbor) { neighbors_.erase(neighbor); }

    const std::unordered_set<ProvinceId>& neighbors() const noexcept { return neighbors_; }

    // Aggregate properties (calculated from tiles)
    TerrainType dominant_terrain() const {
        update_if_needed();
        return dominant_terrain_;
    }
    float average_elevation() const {
        update_if_needed();
        return tiles_.empty() ? 0.0f : total_elevation_ / static_cast<float>(tiles_.size() * 255);
    }

    float average_temperature() const {
        update_if_needed();
        return tiles_.empty() ? 0.0f : total_temperature_ / static_cast<float>(tiles_.size() * 255);
    }

    float average_rainfall() const {
        update_if_needed();
        return tiles_.empty() ? 0.0f : total_rainfall_ / static_cast<float>(tiles_.size() * 255);
    }

    float average_fertility() const {
        update_if_needed();
        return tiles_.empty() ? 0.0f : total_fertility_ / static_cast<float>(tiles_.size() * 255);
    }

    // Economic totals
    Core::Types::Production total_food() const {
        update_if_needed();
        return total_food_;
    }
    Core::Types::Production total_production() const {
        update_if_needed();
        return total_production_;
    }
    Core::Types::Production total_science() const {
        update_if_needed();
        return total_science_;
    }
    Core::Types::Production total_culture() const {
        update_if_needed();
        return total_culture_;
    }
    Core::Types::Production total_faith() const {
        update_if_needed();
        return total_faith_;
    }
    Core::Types::Money total_trade() const {
        update_if_needed();
        return total_trade_;
    }

    // Administrative
    NationId owner() const noexcept { return owner_; }
    void set_owner(NationId owner) noexcept { owner_ = owner; }

    CityId capital_city() const noexcept { return capital_city_; }
    void set_capital_city(CityId city) noexcept { capital_city_ = city; }

    bool is_coastal() const {
        update_if_needed();
        return coastal_;
    }

    // Update from constituent tiles
    void update_from_tiles(const TileMap& map) {
        if (!needs_update_) return;

        reset_aggregates();

        if (tiles_.empty()) {
            needs_update_ = false;
            return;
        }

        // Calculate center
        Point2f center_sum{};
        std::unordered_map<TerrainType, int> terrain_counts;

        for (TileId tile_id : tiles_) {
            const Tile* tile = map.get_tile(tile_id);
            if (!tile) continue;

            const HexCoordinate& coord = tile->coordinate();
            center_sum += Point2f{static_cast<float>(coord.q), static_cast<float>(coord.r)};

            // Aggregate properties
            total_elevation_ += tile->elevation();
            total_temperature_ += tile->temperature();
            total_rainfall_ += tile->rainfall();
            total_fertility_ += tile->fertility();

            // Economic yields
            total_food_ += tile->food_yield();
            total_production_ += tile->production_yield();
            total_science_ += tile->science_yield();
            total_culture_ += tile->culture_yield();
            total_faith_ += tile->faith_yield();
            total_trade_ += tile->trade_yield();

            // Terrain analysis
            terrain_counts[tile->terrain()]++;

            // Check for coastal tiles
            if (tile->is_water() || tile->terrain() == TerrainType::Coast) {
                coastal_ = true;
            }
        }

        // Calculate center
        center_ = center_sum / static_cast<float>(tiles_.size());

        // Determine dominant terrain
        if (!terrain_counts.empty()) {
            auto max_terrain =
                std::max_element(terrain_counts.begin(), terrain_counts.end(),
                                 [](const auto& a, const auto& b) { return a.second < b.second; });
            dominant_terrain_ = max_terrain->first;
        }

        needs_update_ = false;
    }

    // Utility functions
    bool contains_tile(TileId tile_id) const {
        return std::find(tiles_.begin(), tiles_.end(), tile_id) != tiles_.end();
    }

    float area() const noexcept {
        return static_cast<float>(tiles_.size());  // Each hex tile has unit area
    }

    bool is_valid() const noexcept { return id_.is_valid() && !tiles_.empty(); }

   private:
    void reset_aggregates() {
        total_elevation_ = 0;
        total_temperature_ = 0;
        total_rainfall_ = 0;
        total_fertility_ = 0;
        total_food_ = Core::Types::Production{};
        total_production_ = Core::Types::Production{};
        total_science_ = Core::Types::Production{};
        total_culture_ = Core::Types::Production{};
        total_faith_ = Core::Types::Production{};
        total_trade_ = Core::Types::Money{};
        coastal_ = false;
    }

    void update_if_needed() const {
        // Note: This would need access to map, simplified for now
        // In practice, would call update_from_tiles when needed
    }
};

class ProvinceMap {
    std::unordered_map<ProvinceId, std::unique_ptr<Province>> provinces_;
    std::unordered_map<TileId, ProvinceId> tile_to_province_;
    ProvinceId next_id_{ProvinceId{1}};

   public:
    ProvinceMap() = default;
    ~ProvinceMap() = default;

    // Non-copyable but movable
    ProvinceMap(const ProvinceMap&) = delete;
    ProvinceMap& operator=(const ProvinceMap&) = delete;
    ProvinceMap(ProvinceMap&&) = default;
    ProvinceMap& operator=(ProvinceMap&&) = default;

    // Province creation and management
    ProvinceId create_province() {
        ProvinceId id = next_id_++;
        auto province = std::make_unique<Province>(id);
        provinces_[id] = std::move(province);
        return id;
    }

    bool remove_province(ProvinceId id) {
        auto it = provinces_.find(id);
        if (it == provinces_.end()) return false;

        // Remove all tile mappings
        for (TileId tile_id : it->second->tiles()) {
            tile_to_province_.erase(tile_id);
        }

        provinces_.erase(it);
        return true;
    }

    // Province access
    Province* get_province(ProvinceId id) noexcept {
        auto it = provinces_.find(id);
        return it != provinces_.end() ? it->second.get() : nullptr;
    }

    const Province* get_province(ProvinceId id) const noexcept {
        return const_cast<ProvinceMap*>(this)->get_province(id);
    }

    ProvinceId get_province_for_tile(TileId tile_id) const noexcept {
        auto it = tile_to_province_.find(tile_id);
        return it != tile_to_province_.end() ? it->second : ProvinceId::invalid();
    }

    // Tile assignment
    bool assign_tile_to_province(TileId tile_id, ProvinceId province_id) {
        Province* province = get_province(province_id);
        if (!province) return false;

        // Remove from previous province if assigned
        ProvinceId old_province = get_province_for_tile(tile_id);
        if (old_province.is_valid() && old_province != province_id) {
            Province* old = get_province(old_province);
            if (old) old->remove_tile(tile_id);
        }

        // Add to new province
        province->add_tile(tile_id);
        tile_to_province_[tile_id] = province_id;

        return true;
    }

    // Bulk operations
    void update_all_provinces(const TileMap& tile_map) {
        for (auto& province_pair : provinces_) {
            province_pair.second->update_from_tiles(tile_map);
        }

        // Update neighbor relationships
        update_province_neighbors(tile_map);
    }

    void for_each_province(std::function<void(Province&)> func) {
        for (auto& province_pair : provinces_) {
            func(*province_pair.second);
        }
    }

    void for_each_province(std::function<void(const Province&)> func) const {
        for (const auto& province_pair : provinces_) {
            func(*province_pair.second);
        }
    }

    // Statistics
    std::size_t province_count() const noexcept { return provinces_.size(); }

    void clear() {
        provinces_.clear();
        tile_to_province_.clear();
        next_id_ = ProvinceId{1};
    }

    // Auto-generation from tile map
    void generate_provinces_from_tiles(const TileMap& tile_map, std::size_t target_size = 15) {
        clear();

        // Use watershed-like algorithm to create provinces
        auto seed_tiles = select_province_seeds(tile_map, target_size);

        // Create initial provinces from seeds
        for (TileId seed : seed_tiles) {
            ProvinceId province_id = create_province();
            assign_tile_to_province(seed, province_id);
        }

        // Grow provinces using flood fill
        grow_provinces(tile_map);

        // Update all province data
        update_all_provinces(tile_map);
    }

   private:
    std::vector<TileId> select_province_seeds(const TileMap& tile_map, std::size_t count) {
        std::vector<std::pair<TileId, float>> candidates;

        // Score tiles based on suitability as province centers
        tile_map.for_each_tile([&](const Tile& tile) {
            if (tile.is_water()) return;

            float score = calculate_province_seed_score(tile);
            candidates.emplace_back(tile.id(), score);
        });

        // Sort by score and select top candidates with distance constraints
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::vector<TileId> seeds;
        const std::int32_t MIN_SEED_DISTANCE = 8;

        for (const auto& candidate : candidates) {
            if (seeds.size() >= count) break;

            TileId candidate_id = candidate.first;
            const Tile* candidate_tile = tile_map.get_tile(candidate_id);
            if (!candidate_tile) continue;

            // Check distance to existing seeds
            bool too_close = false;
            for (TileId existing_seed : seeds) {
                const Tile* existing_tile = tile_map.get_tile(existing_seed);
                if (!existing_tile) continue;

                std::int32_t distance =
                    TileMap::distance(candidate_tile->coordinate(), existing_tile->coordinate());
                if (distance < MIN_SEED_DISTANCE) {
                    too_close = true;
                    break;
                }
            }

            if (!too_close) {
                seeds.push_back(candidate_id);
            }
        }

        return seeds;
    }

    float calculate_province_seed_score(const Tile& tile) {
        float score = 0.0f;

        // Prefer fertile, moderate elevation areas
        float fertility = tile.fertility() / 255.0f;
        float elevation = tile.elevation() / 255.0f;

        score += fertility * 2.0f;
        score += (elevation > 0.2f && elevation < 0.7f) ? 1.0f : 0.0f;

        // Bonus for good terrain types
        switch (tile.terrain()) {
            case TerrainType::Grassland:
                score += 2.0f;
                break;
            case TerrainType::Plains:
                score += 1.5f;
                break;
            case TerrainType::Hills:
                score += 1.0f;
                break;
            case TerrainType::Forest:
                score += 1.0f;
                break;
            case TerrainType::Coast:
                score += 0.5f;
                break;
            default:
                break;
        }

        // Bonus for rivers
        if (tile.has_feature(Feature::River)) {
            score += 1.0f;
        }

        return score;
    }

    void grow_provinces(const TileMap& tile_map) {
        std::queue<TileId> growth_queue;

        // Initialize queue with all assigned tiles
        for (const auto& tile_province_pair : tile_to_province_) {
            growth_queue.push(tile_province_pair.first);
        }

        // Grow provinces using flood fill
        while (!growth_queue.empty()) {
            TileId current_tile = growth_queue.front();
            growth_queue.pop();

            ProvinceId current_province = get_province_for_tile(current_tile);
            if (!current_province.is_valid()) continue;

            const Tile* tile = tile_map.get_tile(current_tile);
            if (!tile) continue;

            // Try to expand to neighbors
            for (TileId neighbor_id : tile->neighbors()) {
                if (!neighbor_id.is_valid()) continue;

                // Skip if already assigned
                if (get_province_for_tile(neighbor_id).is_valid()) continue;

                const Tile* neighbor = tile_map.get_tile(neighbor_id);
                if (!neighbor || neighbor->is_water()) continue;

                // Check if suitable for this province
                if (should_assign_to_province(*neighbor, current_province)) {
                    assign_tile_to_province(neighbor_id, current_province);
                    growth_queue.push(neighbor_id);
                }
            }
        }
    }

    bool should_assign_to_province(const Tile& tile, ProvinceId province_id) {
        const Province* province = get_province(province_id);
        if (!province) return false;

        // Simple heuristics for province growth
        TerrainType tile_terrain = tile.terrain();
        TerrainType province_terrain = province->dominant_terrain();

        // Prefer similar terrain types
        if (tile_terrain == province_terrain) return true;

        // Allow compatible terrain combinations
        return are_compatible_terrains(tile_terrain, province_terrain);
    }

    bool are_compatible_terrains(TerrainType a, TerrainType b) {
        if (a == b) return true;

        // Simple terrain compatibility logic
        auto is_compatible = [](TerrainType x, TerrainType y) {
            return (x == TerrainType::Plains && y == TerrainType::Grassland) ||
                   (x == TerrainType::Grassland && y == TerrainType::Plains) ||
                   (x == TerrainType::Hills && y == TerrainType::Plains) ||
                   (x == TerrainType::Plains && y == TerrainType::Hills) ||
                   (x == TerrainType::Forest && y == TerrainType::Hills) ||
                   (x == TerrainType::Hills && y == TerrainType::Forest);
        };

        return is_compatible(a, b) || is_compatible(b, a);
    }

    void update_province_neighbors(const TileMap& tile_map) {
        // Clear existing neighbor relationships
        // Note: Province::neighbors() returns const reference, need mutable access
        // This would need a clear_neighbors() method on Province class

        // Find adjacent provinces through tile neighbors
        for (const auto& province_pair : provinces_) {
            ProvinceId id = province_pair.first;
            const auto& province = province_pair.second;
            std::unordered_set<ProvinceId> province_neighbors;

            for (TileId tile_id : province->tiles()) {
                const Tile* tile = tile_map.get_tile(tile_id);
                if (!tile) continue;

                for (TileId neighbor_id : tile->neighbors()) {
                    if (!neighbor_id.is_valid()) continue;

                    ProvinceId neighbor_province = get_province_for_tile(neighbor_id);
                    if (neighbor_province.is_valid() && neighbor_province != id) {
                        province_neighbors.insert(neighbor_province);
                    }
                }
            }

            // Add all found neighbors
            for (ProvinceId neighbor : province_neighbors) {
                // Note: This needs a mutable reference to province
                // Need to get non-const pointer from provinces_ map
                if (auto* mutable_province = get_province(id)) {
                    mutable_province->add_neighbor(neighbor);
                }
            }
        }
    }
};

}  // namespace Tiles
}  // namespace World
}  // namespace Manifest

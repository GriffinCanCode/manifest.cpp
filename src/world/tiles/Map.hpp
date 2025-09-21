#pragma once

#include <algorithm>
#include <functional>
#include <ranges>
#include <span>
#include <unordered_map>
#include <vector>

#include "../../core/memory/Pool.hpp"
#include "Tile.hpp"

namespace Manifest::World::Tiles {

using namespace Core::Memory;

// Hash function for HexCoordinate
struct HexCoordinateHash {
    std::size_t operator()(const HexCoordinate& coord) const noexcept {
        return std::hash<std::int32_t>{}(coord.q) ^ (std::hash<std::int32_t>{}(coord.r) << 1) ^
               (std::hash<std::int32_t>{}(coord.s) << 2);
    }
};

class TileMap {
    std::unordered_map<HexCoordinate, std::unique_ptr<Tile>, HexCoordinateHash> tiles_;
    std::unordered_map<TileId, HexCoordinate> id_to_coord_;
    TileId next_id_{TileId{1}};

    // Spatial indexing for efficient queries
    struct SpatialBucket {
        std::vector<TileId> tile_ids;
    };

    static constexpr std::int32_t BUCKET_SIZE = 32;
    std::unordered_map<std::int64_t, SpatialBucket> spatial_index_;

    std::int64_t get_bucket_key(const HexCoordinate& coord) const noexcept {
        std::int32_t bucket_q = coord.q / BUCKET_SIZE;
        std::int32_t bucket_r = coord.r / BUCKET_SIZE;
        return (static_cast<std::int64_t>(bucket_q) << 32) | static_cast<std::int64_t>(bucket_r);
    }

    void add_to_spatial_index(TileId id, const HexCoordinate& coord) {
        std::int64_t bucket_key = get_bucket_key(coord);
        spatial_index_[bucket_key].tile_ids.push_back(id);
    }

    void remove_from_spatial_index(TileId id, const HexCoordinate& coord) {
        std::int64_t bucket_key = get_bucket_key(coord);
        auto& bucket = spatial_index_[bucket_key];
        bucket.tile_ids.erase(std::remove(bucket.tile_ids.begin(), bucket.tile_ids.end(), id),
                              bucket.tile_ids.end());
    }

   public:
    TileMap() = default;
    ~TileMap() = default;

    // Non-copyable but movable
    TileMap(const TileMap&) = delete;
    TileMap& operator=(const TileMap&) = delete;
    TileMap(TileMap&&) = default;
    TileMap& operator=(TileMap&&) = default;

    // Tile creation and removal
    TileId create_tile(const HexCoordinate& coordinate) {
        if (!coordinate.is_valid()) {
            return TileId::invalid();
        }

        if (tiles_.contains(coordinate)) {
            return TileId::invalid();  // Tile already exists
        }

        TileId id = next_id_++;
        auto tile = std::make_unique<Tile>(id, coordinate);

        id_to_coord_[id] = coordinate;
        add_to_spatial_index(id, coordinate);
        tiles_[coordinate] = std::move(tile);

        return id;
    }

    bool remove_tile(TileId id) {
        if (!id.is_valid()) return false;

        auto coord_it = id_to_coord_.find(id);
        if (coord_it == id_to_coord_.end()) return false;

        const HexCoordinate& coord = coord_it->second;

        // Remove from spatial index
        remove_from_spatial_index(id, coord);

        // Remove from maps
        tiles_.erase(coord);
        id_to_coord_.erase(coord_it);

        return true;
    }

    // Access tiles
    Tile* get_tile(TileId id) noexcept {
        if (!id.is_valid()) return nullptr;

        auto coord_it = id_to_coord_.find(id);
        if (coord_it == id_to_coord_.end()) return nullptr;

        return get_tile(coord_it->second);
    }

    const Tile* get_tile(TileId id) const noexcept {
        return const_cast<TileMap*>(this)->get_tile(id);
    }

    Tile* get_tile(const HexCoordinate& coordinate) noexcept {
        auto it = tiles_.find(coordinate);
        return it != tiles_.end() ? it->second.get() : nullptr;
    }

    const Tile* get_tile(const HexCoordinate& coordinate) const noexcept {
        return const_cast<TileMap*>(this)->get_tile(coordinate);
    }

    // Hex coordinate utilities
    static HexCoordinate neighbor_coordinate(const HexCoordinate& coord,
                                             Direction direction) noexcept {
        static constexpr std::array<HexCoordinate, 6> NEIGHBOR_OFFSETS = {{
            {1, -1, 0},  // Northeast
            {1, 0, -1},  // Southeast
            {0, 1, -1},  // South
            {-1, 1, 0},  // Southwest
            {-1, 0, 1},  // Northwest
            {0, -1, 1}   // North
        }};

        const auto& offset = NEIGHBOR_OFFSETS[static_cast<std::size_t>(direction)];
        return HexCoordinate{coord.q + offset.q, coord.r + offset.r, coord.s + offset.s};
    }

    static std::int32_t distance(const HexCoordinate& a, const HexCoordinate& b) noexcept {
        return (std::abs(a.q - b.q) + std::abs(a.r - b.r) + std::abs(a.s - b.s)) / 2;
    }

    // Neighbor management
    void update_neighbors(TileId tile_id) {
        Tile* tile = get_tile(tile_id);
        if (!tile) return;

        const HexCoordinate& coord = tile->coordinate();

        for (std::size_t i = 0; i < 6; ++i) {
            Direction dir = static_cast<Direction>(i);
            HexCoordinate neighbor_coord = neighbor_coordinate(coord, dir);

            if (Tile* neighbor = get_tile(neighbor_coord)) {
                tile->set_neighbor(dir, neighbor->id());

                // Set reciprocal neighbor
                Direction opposite_dir = static_cast<Direction>((i + 3) % 6);
                neighbor->set_neighbor(opposite_dir, tile_id);
            } else {
                tile->set_neighbor(dir, TileId::invalid());
            }
        }
    }

    // Query methods
    std::vector<TileId> get_tiles_in_radius(const HexCoordinate& center,
                                            std::int32_t radius) const {
        std::vector<TileId> result;

        for (std::int32_t q = -radius; q <= radius; ++q) {
            std::int32_t r1 = std::max(-radius, -q - radius);
            std::int32_t r2 = std::min(radius, -q + radius);

            for (std::int32_t r = r1; r <= r2; ++r) {
                std::int32_t s = -q - r;
                HexCoordinate coord{center.q + q, center.r + r, center.s + s};

                if (const Tile* tile = get_tile(coord)) {
                    result.push_back(tile->id());
                }
            }
        }

        return result;
    }

    std::vector<TileId> get_tiles_in_range(const HexCoordinate& min_coord,
                                           const HexCoordinate& max_coord) const {
        std::vector<TileId> result;

        for (std::int32_t q = min_coord.q; q <= max_coord.q; ++q) {
            for (std::int32_t r = min_coord.r; r <= max_coord.r; ++r) {
                std::int32_t s = -q - r;
                HexCoordinate coord{q, r, s};

                if (coord.is_valid()) {
                    if (const Tile* tile = get_tile(coord)) {
                        result.push_back(tile->id());
                    }
                }
            }
        }

        return result;
    }

    template <typename Predicate>
    std::vector<TileId> find_tiles(Predicate&& predicate) const {
        std::vector<TileId> result;

        for (const auto& [coord, tile] : tiles_) {
            if (predicate(*tile)) {
                result.push_back(tile->id());
            }
        }

        return result;
    }

    // Pathfinding support
    std::vector<TileId> find_path(TileId start, TileId goal,
                                  std::function<float(const Tile&)> cost_function = nullptr) const {
        if (!cost_function) {
            cost_function = [](const Tile& tile) { return tile.movement_cost(); };
        }

        // Simple A* implementation - would be expanded with more sophisticated pathfinding
        const Tile* start_tile = get_tile(start);
        const Tile* goal_tile = get_tile(goal);

        if (!start_tile || !goal_tile) return {};

        std::vector<TileId> path;
        // TODO: Implement A* pathfinding

        return path;
    }

    // Iteration support
    auto tiles() const noexcept {
        return tiles_ | std::views::values |
               std::views::transform([](const auto& ptr) -> const Tile& { return *ptr; });
    }

    auto tiles() noexcept {
        return tiles_ | std::views::values |
               std::views::transform([](const auto& ptr) -> Tile& { return *ptr; });
    }

    // Statistics
    std::size_t tile_count() const noexcept { return tiles_.size(); }

    std::pair<HexCoordinate, HexCoordinate> get_bounds() const noexcept {
        if (tiles_.empty()) {
            return {HexCoordinate{}, HexCoordinate{}};
        }

        auto coords = tiles_ | std::views::keys;
        auto [min_q, max_q] = std::ranges::minmax(coords, {}, &HexCoordinate::q);
        auto [min_r, max_r] = std::ranges::minmax(coords, {}, &HexCoordinate::r);
        auto [min_s, max_s] = std::ranges::minmax(coords, {}, &HexCoordinate::s);

        return {HexCoordinate{min_q.q, min_r.r, min_s.s}, HexCoordinate{max_q.q, max_r.r, max_s.s}};
    }

    // Bulk operations
    void for_each_tile(std::function<void(Tile&)> func) {
        for (auto& [coord, tile] : tiles_) {
            func(*tile);
        }
    }

    void for_each_tile(std::function<void(const Tile&)> func) const {
        for (const auto& [coord, tile] : tiles_) {
            func(*tile);
        }
    }

    // Clear all tiles
    void clear() {
        tiles_.clear();
        id_to_coord_.clear();
        spatial_index_.clear();
        next_id_ = TileId{1};
    }
};

}  // namespace Manifest::World::Tiles

#pragma once

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <vector>

#include "../../core/math/Vector.hpp"
#include "../tiles/Map.hpp"

namespace Manifest::World::Generation {

using namespace Tiles;
using namespace Core::Math;
using namespace Core::Types;

struct RiverSegment {
    TileId source_tile{TileId::invalid()};
    TileId target_tile{TileId::invalid()};
    float flow_rate{1.0f};
    std::uint8_t width{1};
};

class River {
    std::vector<RiverSegment> segments_;
    TileId source_{TileId::invalid()};
    TileId mouth_{TileId::invalid()};
    float total_flow_{0.0f};

   public:
    River(TileId source) : source_{source} {}

    void add_segment(const RiverSegment& segment) {
        segments_.push_back(segment);
        total_flow_ += segment.flow_rate;
    }

    void set_mouth(TileId mouth) { mouth_ = mouth; }

    const std::vector<RiverSegment>& segments() const noexcept { return segments_; }
    TileId source() const noexcept { return source_; }
    TileId mouth() const noexcept { return mouth_; }
    float total_flow() const noexcept { return total_flow_; }

    void calculate_width() {
        float flow_accumulator = 0.0f;

        for (auto& segment : segments_) {
            flow_accumulator += segment.flow_rate;

            // Width based on accumulated flow
            if (flow_accumulator > 50.0f) {
                segment.width = 5;  // Major river
            } else if (flow_accumulator > 20.0f) {
                segment.width = 3;  // Large river
            } else if (flow_accumulator > 10.0f) {
                segment.width = 2;  // Medium river
            } else {
                segment.width = 1;  // Small river/stream
            }
        }
    }
};

class RiverSystem {
    std::vector<River> rivers_;
    std::unordered_map<TileId, std::size_t> tile_to_river_;
    std::mt19937 rng_;

   public:
    explicit RiverSystem(std::uint32_t seed = 42) : rng_{seed} {}

    void generate_rivers(TileMap& map, std::size_t target_river_count = 15) {
        rivers_.clear();
        tile_to_river_.clear();

        // Find potential river sources (high elevation, high rainfall)
        auto sources = find_river_sources(map);

        // Generate rivers from sources
        std::size_t rivers_created = 0;
        for (TileId source : sources) {
            if (rivers_created >= target_river_count) break;

            if (generate_river(map, source)) {
                rivers_created++;
            }
        }

        // Calculate river widths based on flow
        for (auto& river : rivers_) {
            river.calculate_width();
        }

        // Apply river features to tiles
        apply_river_features(map);
    }

    const std::vector<River>& rivers() const noexcept { return rivers_; }

   private:
    std::vector<TileId> find_river_sources(const TileMap& map) {
        std::vector<std::pair<TileId, float>> candidates;

        map.for_each_tile([&](const Tile& tile) {
            if (tile.is_water()) return;

            float elevation = tile.elevation() / 255.0f;
            float rainfall = tile.rainfall() / 255.0f;

            // Prefer high elevation + high rainfall
            float suitability = elevation * 0.7f + rainfall * 0.3f;

            // Avoid too high elevations (snow-covered peaks)
            if (elevation > 0.9f) suitability *= 0.3f;

            // Require minimum thresholds
            if (elevation > 0.4f && rainfall > 0.3f && suitability > 0.5f) {
                candidates.emplace_back(tile.id(), suitability);
            }
        });

        // Sort by suitability and return top candidates
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        std::vector<TileId> sources;
        for (const auto& candidate : candidates) {
            sources.push_back(candidate.first);
            if (sources.size() >= 50) break;  // Limit candidates
        }

        return sources;
    }

    bool generate_river(TileMap& map, TileId source_id) {
        River river(source_id);

        std::unordered_set<TileId> visited;
        TileId current = source_id;

        while (current.is_valid()) {
            visited.insert(current);

            Tile* current_tile = map.get_tile(current);
            if (!current_tile) break;

            // Find next tile (steepest descent)
            TileId next_tile = find_next_river_tile(map, current, visited);

            if (!next_tile.is_valid()) break;

            Tile* next = map.get_tile(next_tile);
            if (!next) break;

            // Calculate flow rate based on rainfall and terrain
            float flow_rate = calculate_flow_rate(*current_tile, *next);

            // Create river segment
            RiverSegment segment{.source_tile = current,
                                 .target_tile = next_tile,
                                 .flow_rate = flow_rate,
                                 .width = 1};

            river.add_segment(segment);

            // Check if we've reached water (river mouth)
            if (next->is_water()) {
                river.set_mouth(next_tile);
                break;
            }

            current = next_tile;

            // Prevent infinite loops
            if (visited.size() > 200) break;
        }

        // Only keep rivers with meaningful length
        if (river.segments().size() >= 3) {
            std::size_t river_index = rivers_.size();
            rivers_.push_back(std::move(river));

            // Map tiles to river
            for (const auto& segment : rivers_[river_index].segments()) {
                tile_to_river_[segment.source_tile] = river_index;
            }

            return true;
        }

        return false;
    }

    TileId find_next_river_tile(const TileMap& map, TileId current_id,
                                const std::unordered_set<TileId>& visited) {
        const Tile* current_tile = map.get_tile(current_id);
        if (!current_tile) return TileId::invalid();

        TileId best_next = TileId::invalid();
        float best_score = -1.0f;

        // Check all neighbors
        for (const TileId& neighbor_id : current_tile->neighbors()) {
            if (!neighbor_id.is_valid() || visited.contains(neighbor_id)) continue;

            const Tile* neighbor = map.get_tile(neighbor_id);
            if (!neighbor) continue;

            float score = calculate_flow_score(*current_tile, *neighbor);

            if (score > best_score) {
                best_score = score;
                best_next = neighbor_id;
            }
        }

        return best_next;
    }

    float calculate_flow_score(const Tile& current, const Tile& neighbor) {
        float current_elevation = current.elevation() / 255.0f;
        float neighbor_elevation = neighbor.elevation() / 255.0f;

        // Water is always attractive for rivers
        if (neighbor.is_water()) {
            return 100.0f;
        }

        // Must flow downhill
        if (neighbor_elevation >= current_elevation) {
            return -1.0f;
        }

        float elevation_drop = current_elevation - neighbor_elevation;
        float neighbor_rainfall = neighbor.rainfall() / 255.0f;

        // Combine elevation drop with rainfall attraction
        return elevation_drop * 2.0f + neighbor_rainfall * 0.5f;
    }

    float calculate_flow_rate(const Tile& current, const Tile& next) {
        float base_flow = 1.0f;

        // Higher rainfall = more flow
        float rainfall_factor = (current.rainfall() + next.rainfall()) / (2.0f * 255.0f);
        base_flow *= (1.0f + rainfall_factor * 2.0f);

        // Terrain affects flow
        float terrain_multiplier = 1.0f;
        switch (current.terrain()) {
            case TerrainType::Mountains:
                terrain_multiplier = 2.0f;
                break;
            case TerrainType::Hills:
                terrain_multiplier = 1.5f;
                break;
            case TerrainType::Forest:
                terrain_multiplier = 1.2f;
                break;
            case TerrainType::Jungle:
                terrain_multiplier = 1.3f;
                break;
            case TerrainType::Desert:
                terrain_multiplier = 0.3f;
                break;
            default:
                break;
        }

        return base_flow * terrain_multiplier;
    }

    void apply_river_features(TileMap& map) {
        for (const auto& river : rivers_) {
            for (const auto& segment : river.segments()) {
                Tile* tile = map.get_tile(segment.source_tile);
                if (!tile) continue;

                // Add river feature
                tile->add_feature(Feature::River);

                // Modify tile properties based on river
                modify_tile_for_river(*tile, segment);
            }

            // Mark river mouth
            if (river.mouth().is_valid()) {
                Tile* mouth_tile = map.get_tile(river.mouth());
                if (mouth_tile && !mouth_tile->is_water()) {
                    // Create delta/estuary
                    mouth_tile->set_terrain(TerrainType::Marsh);
                }
            }
        }

        // Create flood plains along major rivers
        create_flood_plains(map);
    }

    void modify_tile_for_river(Tile& tile, const RiverSegment& segment) {
        // Increase fertility due to river
        std::uint8_t current_fertility = tile.fertility();
        std::uint8_t fertility_boost = segment.width * 20;  // Larger rivers = more fertility
        tile.set_fertility(std::min(255, static_cast<int>(current_fertility + fertility_boost)));

        // Slight food yield bonus
        Production current_food = tile.food_yield();
        Production river_bonus = Production{static_cast<double>(segment.width) * 0.5};
        // Note: Can't modify yields directly, would need setter methods
    }

    void create_flood_plains(TileMap& map) {
        for (const auto& river : rivers_) {
            if (river.total_flow() < 30.0f) continue;  // Only major rivers

            for (const auto& segment : river.segments()) {
                if (segment.width < 3) continue;  // Only wide river sections

                Tile* river_tile = map.get_tile(segment.source_tile);
                if (!river_tile) continue;

                // Check neighbors of river tiles
                for (const TileId& neighbor_id : river_tile->neighbors()) {
                    if (!neighbor_id.is_valid()) continue;

                    Tile* neighbor = map.get_tile(neighbor_id);
                    if (!neighbor || neighbor->has_feature(Feature::River)) continue;

                    // Create floodplain if suitable
                    if (neighbor->terrain() == TerrainType::Plains ||
                        neighbor->terrain() == TerrainType::Grassland) {
                        neighbor->add_feature(Feature::Floodplains);

                        // Increase fertility
                        std::uint8_t current_fertility = neighbor->fertility();
                        neighbor->set_fertility(
                            std::min(255, static_cast<int>(current_fertility + 40)));
                    }
                }
            }
        }
    }
};

}  // namespace Manifest::World::Generation

#pragma once

#include <queue>
#include <random>
#include <span>
#include <unordered_set>
#include <vector>

#include "../../core/math/Vector.hpp"
#include "../tiles/Map.hpp"

namespace Manifest::World::Generation {

using namespace Tiles;
using namespace Core::Math;
using namespace Core::Types;

enum class PlateType : std::uint8_t { Continental, Oceanic };

struct TectonicPlate {
    PlateId id{PlateId::invalid()};
    Vec2f velocity{};
    Vec2f center{};
    PlateType type{PlateType::Continental};
    std::vector<TileId> tiles{};
    float age{0.0f};
    float density{2.7f};  // Continental: ~2.7, Oceanic: ~3.0

    bool is_continental() const noexcept { return type == PlateType::Continental; }
    bool is_oceanic() const noexcept { return type == PlateType::Oceanic; }
};

class PlateSystem {
    std::vector<TectonicPlate> plates_;
    std::unordered_map<TileId, PlateId> tile_to_plate_;
    PlateId next_id_{PlateId{1}};
    std::mt19937 rng_;

   public:
    explicit PlateSystem(std::uint32_t seed = 42) : rng_{seed} {}

    void generate_plates(TileMap& map, std::size_t plate_count = 8) {
        plates_.clear();
        tile_to_plate_.clear();

        // Create initial plate centers using Poisson disk sampling
        auto centers = generate_plate_centers(map, plate_count);

        // Initialize plates
        for (const auto& center : centers) {
            create_plate(center);
        }

        // Assign tiles to plates using Voronoi regions
        assign_tiles_to_plates(map);

        // Set plate velocities and types
        initialize_plate_dynamics();
    }

    void simulate_tectonics(TileMap& map, float time_step = 1.0f) {
        // Move plates
        move_plates(time_step);

        // Handle plate interactions
        process_plate_boundaries(map);

        // Update terrain based on tectonics
        apply_tectonic_effects(map);
    }

    const std::vector<TectonicPlate>& plates() const noexcept { return plates_; }

    PlateId get_plate_for_tile(TileId tile_id) const noexcept {
        auto it = tile_to_plate_.find(tile_id);
        return it != tile_to_plate_.end() ? it->second : PlateId::invalid();
    }

   private:
    std::vector<Vec2f> generate_plate_centers(const TileMap& map, std::size_t count) {
        std::vector<Vec2f> centers;
        auto [min_coord, max_coord] = map.get_bounds();

        // Use Poisson disk sampling for even distribution
        float min_distance = std::sqrt(static_cast<float>(map.tile_count()) / count) * 0.8f;

        std::uniform_real_distribution<float> x_dist(min_coord.q, max_coord.q);
        std::uniform_real_distribution<float> y_dist(min_coord.r, max_coord.r);

        for (std::size_t attempts = 0; attempts < count * 100 && centers.size() < count;
             ++attempts) {
            Vec2f candidate{x_dist(rng_), y_dist(rng_)};

            bool valid = true;
            for (const auto& center : centers) {
                if (distance(candidate, center) < min_distance) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                centers.push_back(candidate);
            }
        }

        return centers;
    }

    void create_plate(const Vec2f& center) {
        TectonicPlate plate;
        plate.id = next_id_++;
        plate.center = center;
        plate.velocity = generate_random_velocity();
        plates_.push_back(std::move(plate));
    }

    Vec2f generate_random_velocity() {
        std::uniform_real_distribution<float> speed_dist(0.5f, 2.0f);
        std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * std::numbers::pi_v<float>);

        float speed = speed_dist(rng_);
        float angle = angle_dist(rng_);

        return Vec2f{speed * std::cos(angle), speed * std::sin(angle)};
    }

    void assign_tiles_to_plates(TileMap& map) {
        map.for_each_tile([this](const Tile& tile) {
            const auto& coord = tile.coordinate();
            Vec2f tile_pos{static_cast<float>(coord.q), static_cast<float>(coord.r)};

            PlateId closest_plate = PlateId::invalid();
            float min_distance = std::numeric_limits<float>::max();

            for (const auto& plate : plates_) {
                float dist = distance(tile_pos, plate.center);
                if (dist < min_distance) {
                    min_distance = dist;
                    closest_plate = plate.id;
                }
            }

            if (closest_plate.is_valid()) {
                tile_to_plate_[tile.id()] = closest_plate;

                // Add tile to plate
                auto plate_it =
                    std::find_if(plates_.begin(), plates_.end(),
                                 [closest_plate](const auto& p) { return p.id == closest_plate; });
                if (plate_it != plates_.end()) {
                    plate_it->tiles.push_back(tile.id());
                }
            }
        });
    }

    void initialize_plate_dynamics() {
        std::uniform_real_distribution<float> type_dist(0.0f, 1.0f);
        std::uniform_real_distribution<float> age_dist(0.0f, 200.0f);

        for (auto& plate : plates_) {
            // Determine plate type (more oceanic plates)
            plate.type = type_dist(rng_) < 0.7f ? PlateType::Oceanic : PlateType::Continental;
            plate.density = plate.is_oceanic() ? 3.0f : 2.7f;
            plate.age = age_dist(rng_);
        }
    }

    void move_plates(float time_step) {
        for (auto& plate : plates_) {
            plate.center += plate.velocity * time_step;
        }
    }

    void process_plate_boundaries(TileMap& map) {
        // Find boundary tiles and determine interaction types
        std::unordered_map<TileId, std::vector<PlateId>> boundary_tiles;

        map.for_each_tile([&](const Tile& tile) {
            PlateId tile_plate = get_plate_for_tile(tile.id());
            if (!tile_plate.is_valid()) return;

            std::unordered_set<PlateId> neighbor_plates;
            for (const auto& neighbor_id : tile.neighbors()) {
                if (!neighbor_id.is_valid()) continue;

                PlateId neighbor_plate = get_plate_for_tile(neighbor_id);
                if (neighbor_plate.is_valid() && neighbor_plate != tile_plate) {
                    neighbor_plates.insert(neighbor_plate);
                }
            }

            if (!neighbor_plates.empty()) {
                boundary_tiles[tile.id()].push_back(tile_plate);
                for (auto plate_id : neighbor_plates) {
                    boundary_tiles[tile.id()].push_back(plate_id);
                }
            }
        });

        // Process boundary interactions
        for (const auto& [tile_id, plate_ids] : boundary_tiles) {
            if (plate_ids.size() < 2) continue;

            Tile* tile = map.get_tile(tile_id);
            if (!tile) continue;

            apply_boundary_effects(*tile, plate_ids);
        }
    }

    void apply_boundary_effects(Tile& tile, const std::vector<PlateId>& plate_ids) {
        if (plate_ids.size() < 2) return;

        const auto* plate1 = get_plate(plate_ids[0]);
        const auto* plate2 = get_plate(plate_ids[1]);

        if (!plate1 || !plate2) return;

        // Calculate relative velocity
        Vec2f relative_velocity = plate1->velocity - plate2->velocity;
        float convergence = relative_velocity.length();

        // Determine interaction type
        BoundaryType boundary_type = classify_boundary(*plate1, *plate2, relative_velocity);

        // Apply effects based on boundary type
        switch (boundary_type) {
            case BoundaryType::Convergent:
                apply_convergent_effects(tile, *plate1, *plate2, convergence);
                break;
            case BoundaryType::Divergent:
                apply_divergent_effects(tile, *plate1, *plate2, convergence);
                break;
            case BoundaryType::Transform:
                apply_transform_effects(tile, *plate1, *plate2, convergence);
                break;
        }
    }

    enum class BoundaryType { Convergent, Divergent, Transform };

    BoundaryType classify_boundary(const TectonicPlate& plate1, const TectonicPlate& plate2,
                                   const Vec2f& relative_velocity) {
        // Simplified classification based on relative motion
        Vec2f plate_direction = (plate2.center - plate1.center).normalized();
        float dot_product = relative_velocity.normalized().dot(plate_direction);

        if (dot_product > 0.3f) {
            return BoundaryType::Convergent;
        } else if (dot_product < -0.3f) {
            return BoundaryType::Divergent;
        } else {
            return BoundaryType::Transform;
        }
    }

    void apply_convergent_effects(Tile& tile, const TectonicPlate& plate1,
                                  const TectonicPlate& plate2, float intensity) {
        // Create mountains at convergent boundaries
        std::uint8_t current_elevation = tile.elevation();
        std::uint8_t elevation_boost = static_cast<std::uint8_t>(intensity * 50.0f);

        tile.set_elevation(std::min(255, static_cast<int>(current_elevation + elevation_boost)));

        // Oceanic-Continental: Create volcanic arcs
        if ((plate1.is_oceanic() && plate2.is_continental()) ||
            (plate1.is_continental() && plate2.is_oceanic())) {
            if (tile.elevation() > 180) {
                tile.add_feature(Feature::Volcano);
            }
        }

        // Continental-Continental: Create mountain ranges
        if (plate1.is_continental() && plate2.is_continental()) {
            if (tile.elevation() > 200) {
                tile.set_terrain(TerrainType::Mountains);
            }
        }
    }

    void apply_divergent_effects(Tile& tile, const TectonicPlate& plate1,
                                 const TectonicPlate& plate2, float intensity) {
        // Create rifts and lower elevation
        std::uint8_t current_elevation = tile.elevation();
        std::uint8_t elevation_reduction = static_cast<std::uint8_t>(intensity * 30.0f);

        tile.set_elevation(std::max(0, static_cast<int>(current_elevation - elevation_reduction)));

        // Create oceanic ridges or continental rifts
        if (tile.elevation() < 100) {
            tile.set_terrain(TerrainType::Ocean);
        }
    }

    void apply_transform_effects(Tile& tile, const TectonicPlate& plate1,
                                 const TectonicPlate& plate2, float intensity) {
        // Transform boundaries create less dramatic elevation changes
        // but can create fault systems and localized elevation changes

        if (intensity > 1.5f) {
            std::uint8_t current_elevation = tile.elevation();
            std::uint8_t elevation_change = static_cast<std::uint8_t>(intensity * 10.0f);

            // Random elevation change (up or down)
            std::uniform_int_distribution<int> change_dist(-elevation_change, elevation_change);
            int change = change_dist(rng_);

            tile.set_elevation(std::clamp(static_cast<int>(current_elevation) + change, 0, 255));
        }
    }

    void apply_tectonic_effects(TileMap& map) {
        // Age plates and modify their properties
        for (auto& plate : plates_) {
            plate.age += 0.1f;

            // Older oceanic plates become denser
            if (plate.is_oceanic()) {
                plate.density = std::min(3.2f, 3.0f + plate.age * 0.001f);
            }
        }
    }

    const TectonicPlate* get_plate(PlateId id) const noexcept {
        auto it = std::find_if(plates_.begin(), plates_.end(),
                               [id](const auto& p) { return p.id == id; });
        return it != plates_.end() ? &*it : nullptr;
    }
};

}  // namespace Manifest::World::Generation

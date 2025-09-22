#pragma once

#include <concepts>
#include <memory>
#include <numbers>
#include <random>

#include "../../core/math/Vector.hpp"
#include "../tiles/Map.hpp"
#include "../generation/Noise.hpp"

namespace Manifest::World::Terrain {

using namespace Tiles;
using namespace Core::Math;
using namespace Generation;

// Helper type trait to check if a type is map-like
template <typename T>
struct is_map_like : std::false_type {};

template <typename T>
constexpr bool is_map_like_v = is_map_like<T>::value;

// Specialize for TileMap
template <>
struct is_map_like<Tiles::TileMap> : std::true_type {};

// Legacy type aliases for compatibility
using NoiseGenerator = Noise;
using PerlinNoise = Perlin;
using FractalNoise = Fractal;

// Terrain generation parameters
struct GenerationParams {
    std::uint32_t seed{42};
    std::int32_t map_radius{100};

    // Elevation
    float elevation_scale{100.0f};
    float continent_frequency{0.005f};
    float mountain_frequency{0.02f};
    float hill_frequency{0.05f};

    // Climate
    float temperature_gradient{0.8f};  // From equator to poles
    float rainfall_scale{1.0f};
    float rainfall_frequency{0.01f};

    // Water level (0.0 = all land, 1.0 = all water)
    float sea_level{0.3f};

    // Resource distribution
    float resource_density{0.1f};
    float luxury_rarity{0.05f};
    float strategic_rarity{0.08f};
};

template <typename MapType>
concept MapLike = requires(MapType& map, TileId id, const HexCoordinate& coord) {
    { map.create_tile(coord) } -> std::same_as<TileId>;
    { map.get_tile(id) } -> std::convertible_to<Tile*>;
    { map.update_neighbors(id) } -> std::same_as<void>;
};

class TerrainGenerator {
    GenerationParams params_;
    std::mt19937 rng_;
    std::unique_ptr<NoiseGenerator> elevation_noise_;
    std::unique_ptr<NoiseGenerator> temperature_noise_;
    std::unique_ptr<NoiseGenerator> rainfall_noise_;
    std::unique_ptr<NoiseGenerator> resource_noise_;

   public:
    explicit TerrainGenerator(const GenerationParams& params = {})
        : params_{params}, rng_{params.seed} {
        // Create noise generators with different seeds using new system
        elevation_noise_ = std::make_unique<Continent>(
            std::make_unique<Fractal>(std::make_unique<Perlin>(params.seed), 6, 
                Persistence{0.6F}, Lacunarity{2.0F}),
            std::make_unique<Fractal>(std::make_unique<Perlin>(params.seed + 10), 4),
            ContinentShape::Pangaea, params.sea_level, Vec2f{0.0F, 0.0F}, 
            static_cast<float>(params.map_radius));

        temperature_noise_ = std::make_unique<Fractal>(
            std::make_unique<Perlin>(params.seed + 1), 3, Persistence{0.5F});

        rainfall_noise_ = std::make_unique<Fractal>(
            std::make_unique<Perlin>(params.seed + 2), 4, Persistence{0.5F});

        resource_noise_ = std::make_unique<Perlin>(params.seed + 3);
    }

    template <MapLike MapType>
    void generate_world(MapType& map) {
        // Generate base terrain
        generate_elevation(map);
        generate_climate(map);
        generate_terrain_types(map);
        generate_resources(map);

        // Update all neighbor relationships
        update_all_neighbors(map);
    }

   private:
    template <MapLike MapType>
    void generate_elevation(MapType& map) {
        for (std::int32_t q = -params_.map_radius; q <= params_.map_radius; ++q) {
            std::int32_t r1 = std::max(-params_.map_radius, -q - params_.map_radius);
            std::int32_t r2 = std::min(params_.map_radius, -q + params_.map_radius);

            for (std::int32_t r = r1; r <= r2; ++r) {
                std::int32_t s = -q - r;
                HexCoordinate coord{q, r, s};

                TileId tile_id = map.create_tile(coord);
                if (!tile_id.is_valid()) continue;

                Tile* tile = map.get_tile(tile_id);
                if (!tile) continue;

                // Sample noise for elevation
                float x = static_cast<float>(q);
                float y = static_cast<float>(r);

                float elevation = elevation_noise_->sample(x, y);
                elevation = (elevation + 1.0f) * 0.5f;  // Normalize to [0, 1]

                // Apply distance-based falloff for island generation
                float distance_from_center = std::sqrt(x * x + y * y) / params_.map_radius;
                float falloff = std::max(0.0f, 1.0f - distance_from_center * distance_from_center);
                elevation *= falloff;

                tile->set_elevation(static_cast<std::uint8_t>(elevation * 255));
            }
        }
    }

    template <typename MapType>
    typename std::enable_if<is_map_like_v<MapType>, void>::type generate_climate(
        MapType& map) {
        map.for_each_tile([this](Tile& tile) {
            const HexCoordinate& coord = tile.coordinate();
            float x = static_cast<float>(coord.q);
            float y = static_cast<float>(coord.r);

            // Temperature based on latitude (distance from center)
            float latitude = std::abs(y) / params_.map_radius;
            float base_temperature = 1.0f - latitude * params_.temperature_gradient;

            // Add noise for variation
            float temp_noise = temperature_noise_->sample(x * 0.01f, y * 0.01f);
            float temperature = base_temperature + temp_noise * 0.2f;
            temperature = std::clamp(temperature, 0.0f, 1.0f);

            tile.set_temperature(static_cast<std::uint8_t>(temperature * 255));

            // Rainfall
            float rainfall = rainfall_noise_->sample(x, y);
            rainfall = (rainfall + 1.0f) * 0.5f;  // Normalize to [0, 1]

            // Modify rainfall based on elevation (rain shadow effect)
            float elevation_factor = tile.elevation() / 255.0f;
            if (elevation_factor > 0.7f) {
                rainfall *= 0.5f;  // Mountains get less rain on far side
            }

            tile.set_rainfall(static_cast<std::uint8_t>(rainfall * 255));
        });
    }

    template <typename MapType>
    typename std::enable_if<is_map_like<MapType>::value, void>::type generate_terrain_types(
        MapType& map) {
        map.for_each_tile([this](Tile& tile) {
            float elevation = tile.elevation() / 255.0f;
            float temperature = tile.temperature() / 255.0f;
            float rainfall = tile.rainfall() / 255.0f;

            // Determine terrain type based on elevation, temperature, and rainfall
            TerrainType terrain;

            if (elevation < params_.sea_level) {
                terrain =
                    elevation < params_.sea_level * 0.5f ? TerrainType::Ocean : TerrainType::Coast;
            } else if (elevation > 0.8f) {
                terrain = TerrainType::Mountains;
            } else if (elevation > 0.6f) {
                terrain = TerrainType::Hills;
            } else {
                // Land terrain based on temperature and rainfall
                if (temperature < 0.2f) {
                    terrain = TerrainType::Tundra;
                } else if (temperature < 0.4f && rainfall < 0.3f) {
                    terrain = TerrainType::Tundra;
                } else if (rainfall < 0.2f) {
                    terrain = TerrainType::Desert;
                } else if (temperature > 0.7f && rainfall > 0.6f) {
                    terrain = TerrainType::Jungle;
                } else if (rainfall > 0.5f) {
                    terrain = TerrainType::Forest;
                } else if (rainfall > 0.3f) {
                    terrain = TerrainType::Grassland;
                } else {
                    terrain = TerrainType::Plains;
                }
            }

            tile.set_terrain(terrain);
            tile.set_passable(terrain != TerrainType::Mountains && terrain != TerrainType::Ocean);

            // Calculate fertility based on terrain
            std::uint8_t fertility = 0;
            switch (terrain) {
                case TerrainType::Grassland:
                    fertility = 200;
                    break;
                case TerrainType::Plains:
                    fertility = 150;
                    break;
                case TerrainType::Forest:
                    fertility = 120;
                    break;
                case TerrainType::Jungle:
                    fertility = 100;
                    break;
                case TerrainType::Hills:
                    fertility = 80;
                    break;
                case TerrainType::Desert:
                    fertility = 20;
                    break;
                case TerrainType::Tundra:
                    fertility = 30;
                    break;
                case TerrainType::Coast:
                    fertility = 60;
                    break;
                default:
                    fertility = 10;
                    break;
            }

            tile.set_fertility(fertility);
        });
    }

    template <typename MapType>
    typename std::enable_if<is_map_like<MapType>::value, void>::type generate_resources(
        MapType& map) {
        map.for_each_tile([this](Tile& tile) {
            if (tile.is_water()) return;

            const HexCoordinate& coord = tile.coordinate();
            float resource_value = resource_noise_->sample(static_cast<float>(coord.q) * 0.1f,
                                                           static_cast<float>(coord.r) * 0.1f);

            if (resource_value < -0.8f + params_.resource_density) return;

            // Determine resource type based on terrain and noise
            ResourceType resource = ResourceType::None;

            switch (tile.terrain()) {
                case TerrainType::Hills:
                case TerrainType::Mountains:
                    if (resource_value > 0.7f)
                        resource = ResourceType::Gold;
                    else if (resource_value > 0.3f)
                        resource = ResourceType::Iron;
                    else
                        resource = ResourceType::Stone;
                    break;

                case TerrainType::Plains:
                case TerrainType::Grassland:
                    if (resource_value > 0.5f)
                        resource = ResourceType::Wheat;
                    else
                        resource = ResourceType::Cattle;
                    break;

                case TerrainType::Coast:
                    resource = ResourceType::Fish;
                    break;

                case TerrainType::Desert:
                    if (resource_value > 0.8f) resource = ResourceType::Oil;
                    break;

                default:
                    if (resource_value > 0.9f) resource = ResourceType::Luxury;
                    break;
            }

            tile.set_resource(resource);
        });
    }

    template <typename MapType>
    typename std::enable_if<is_map_like<MapType>::value, void>::type update_all_neighbors(
        MapType& map) {
        map.for_each_tile([&map](const Tile& tile) { map.update_neighbors(tile.id()); });
    }
};

}  // namespace Manifest::World::Terrain

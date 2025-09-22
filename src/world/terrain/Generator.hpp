#pragma once

#include <concepts>
#include <memory>
#include <numbers>
#include <random>

#include "../../core/math/Vector.hpp"
#include "../tiles/Map.hpp"
#include "../generation/Noise.hpp"
#include "../generation/Climate.hpp"
#include "../generation/Latitude.hpp"
#include "../generation/Geology.hpp"

namespace Manifest::World::Terrain {

using namespace Tiles;
using namespace Core::Math;
using namespace Generation;
using Generation::Latitude;
using Generation::Climate;

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
        map.for_each_tile([this, &map](Tile& tile) {
            const HexCoordinate& coord = tile.coordinate();
            float x = static_cast<float>(coord.q);
            float y = static_cast<float>(coord.r);

            // Enhanced latitude-based temperature calculation
            float latitude_deg = Latitude::calculate_spherical_latitude(coord, params_.map_radius);
            float latitude_factor = Latitude::temperature_latitude_factor(coord, params_.map_radius);
            
            // Base temperature from latitude and seasonal effects
            float base_temperature = latitude_factor * params_.temperature_gradient;
            
            // Add noise for regional variation
            float temp_noise = temperature_noise_->sample(x * 0.01f, y * 0.01f);
            float temperature = base_temperature + temp_noise * 0.15f;
            temperature = std::clamp(temperature, 0.0f, 1.0f);
            
            tile.set_temperature(static_cast<std::uint8_t>(temperature * 255));

            // Enhanced rainfall calculation
            float rainfall = rainfall_noise_->sample(x * params_.rainfall_frequency, y * params_.rainfall_frequency);
            rainfall = (rainfall + 1.0f) * 0.5f;  // Normalize to [0, 1]
            
            // Apply latitude effects to rainfall (more complex patterns)
            auto climate_influence = Latitude::climate_influence(coord, params_.map_radius);
            float latitude_rainfall_modifier = 1.0f;
            
            // Tropical regions: higher base rainfall
            latitude_rainfall_modifier += climate_influence.tropical_weight * 0.5f;
            // Temperate regions: moderate rainfall
            latitude_rainfall_modifier += climate_influence.temperate_weight * 0.2f;
            // Polar regions: lower rainfall due to cold air holding less moisture
            latitude_rainfall_modifier -= climate_influence.polar_weight * 0.3f;
            
            rainfall *= latitude_rainfall_modifier;

            // Elevation effects (rain shadow and orographic precipitation)
            float elevation_factor = tile.elevation() / 255.0f;
            if (elevation_factor > 0.6f) {
                // High elevations get orographic precipitation on windward side
                rainfall *= 1.2f; 
            }
            if (elevation_factor > 0.8f) {
                // Very high elevations have reduced precipitation
                rainfall *= 0.7f;  
            }
            
            rainfall = std::clamp(rainfall, 0.0f, 1.0f);
            tile.set_rainfall(static_cast<std::uint8_t>(rainfall * 255));

            // Assign climate zone based on coordinates and calculated climate data
            float annual_precipitation = rainfall * 2500.0f; // Convert to mm/year estimate
            bool is_coastal = is_near_coastline(coord, map);
            
            Generation::ClimateZone climate_zone = Climate::classify_climate(
                coord, params_.map_radius, elevation_factor, annual_precipitation, is_coastal
            );
            
            tile.set_climate_zone(climate_zone);
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
            TerrainType terrain = TerrainType::Plains; // Default fallback

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
            
            // Get geological noise values for different aspects
            float geology_noise = resource_noise_->sample(static_cast<float>(coord.q) * 0.05f,
                                                         static_cast<float>(coord.r) * 0.05f);
            float age_noise = resource_noise_->sample(static_cast<float>(coord.q) * 0.02f,
                                                     static_cast<float>(coord.r) * 0.02f);
            
            // Simulate simplified geological inputs
            float plate_age = 50.0F + age_noise * 100.0F; // 0-150 million years
            bool is_oceanic = tile.terrain() == TerrainType::Ocean || tile.elevation() < 80;
            
            // Determine boundary type from elevation patterns (simplified)
            Generation::Boundary boundary_type = Generation::Boundary::Transform;
            if (tile.terrain() == TerrainType::Mountains) {
                boundary_type = Generation::Boundary::Convergent;
            } else if (tile.terrain() == TerrainType::Ocean && tile.elevation() < 50) {
                boundary_type = Generation::Boundary::Divergent;
            }
            
            // Convert terrain types to our geological system
            Generation::Terrain geo_terrain = convert_terrain_to_geological(tile.terrain());
            bool is_coastal = (tile.terrain() == TerrainType::Coast);
            
            // Generate geological formation
            auto formation = Generation::Geology::generate_geology(
                coord, static_cast<float>(params_.map_radius), plate_age, is_oceanic,
                boundary_type, tile.elevation(), geo_terrain, is_coastal, geology_noise
            );
            
            // Calculate resource distribution for this formation
            auto resource_distribution = Generation::Geology::calculate_resource_distribution(
                formation,
                age_noise * 0.5F + 0.5F,        // geological age factor [0-1]
                params_.resource_density,         // mineral concentration
                std::abs(geology_noise)          // hydrothermal factor
            );
            
            // Select resource based on probabilities
            ResourceType selected_resource = select_resource_from_distribution(resource_distribution, geology_noise);
            
            tile.set_resource(selected_resource);
        });
    }

private:
    // Convert TerrainType to Geology::Terrain
    static constexpr Generation::Terrain convert_terrain_to_geological(TerrainType terrain) noexcept {
        switch (terrain) {
            case TerrainType::Ocean: return Generation::Terrain::Ocean;
            case TerrainType::Coast: return Generation::Terrain::Coast;
            case TerrainType::Plains: return Generation::Terrain::Plains;
            case TerrainType::Grassland: return Generation::Terrain::Grassland;
            case TerrainType::Hills: return Generation::Terrain::Hills;
            case TerrainType::Mountains: return Generation::Terrain::Mountains;
            case TerrainType::Desert: return Generation::Terrain::Desert;
            case TerrainType::Tundra: return Generation::Terrain::Tundra;
            case TerrainType::Snow: return Generation::Terrain::Snow;
            default: return Generation::Terrain::Plains;
        }
    }
    
    // Convert Geology::Resource to ResourceType
    static constexpr ResourceType convert_resource_from_geological(Generation::Resource resource) noexcept {
        switch (resource) {
            case Generation::Resource::None: return ResourceType::None;
            case Generation::Resource::Iron: return ResourceType::Iron;
            case Generation::Resource::Coal: return ResourceType::Coal;
            case Generation::Resource::Oil: return ResourceType::Oil;
            case Generation::Resource::Gold: return ResourceType::Gold;
            case Generation::Resource::Wheat: return ResourceType::Wheat;
            case Generation::Resource::Cattle: return ResourceType::Cattle;
            case Generation::Resource::Fish: return ResourceType::Fish;
            case Generation::Resource::Stone: return ResourceType::Stone;
            case Generation::Resource::Luxury: return ResourceType::Luxury;
            case Generation::Resource::Strategic: return ResourceType::Strategic;
            default: return ResourceType::None;
        }
    }
    
    // Select resource based on probability distribution
    ResourceType select_resource_from_distribution(
        const std::array<std::pair<Generation::Resource, float>, 4>& distribution,
        float random_value) noexcept {
        
        // Normalize random value to [0, 1]
        float normalized_random = (random_value + 1.0F) * 0.5F;
        
        // Use weighted selection based on probabilities
        float cumulative_probability = 0.0F;
        for (const auto& [resource, probability] : distribution) {
            if (resource == Generation::Resource::None) continue;
            
            cumulative_probability += probability;
            if (normalized_random <= cumulative_probability) {
                return convert_resource_from_geological(resource);
            }
        }
        
        return ResourceType::None;
    }

    template <typename MapType>
    typename std::enable_if<is_map_like<MapType>::value, void>::type update_all_neighbors(
        MapType& map) {
        map.for_each_tile([&map](const Tile& tile) { map.update_neighbors(tile.id()); });
    }

    // Helper method to determine if a coordinate is near coastline
    template <typename MapType>
    bool is_near_coastline(const HexCoordinate& coord, [[maybe_unused]] const MapType& map, [[maybe_unused]] int search_radius = 2) {
        // Check neighboring tiles for water presence
        for (int dq = -search_radius; dq <= search_radius; ++dq) {
            for (int dr = -search_radius; dr <= search_radius; ++dr) {
                if (dq == 0 && dr == 0) continue;
                
                HexCoordinate neighbor_coord{coord.q + dq, coord.r + dr, -(coord.q + dq + coord.r + dr)};
                
                // Simple distance check within hex grid
                if (std::abs(dq) + std::abs(dr) + std::abs(-dq-dr) > search_radius * 2) continue;
                
                // Check if this neighbor would be water based on simple heuristic
                // This is a simplified check - in a full implementation, 
                // you'd query the actual map tiles
                float distance_from_center = std::sqrt(
                    static_cast<float>(neighbor_coord.q * neighbor_coord.q + 
                                     neighbor_coord.r * neighbor_coord.r)
                ) / params_.map_radius;
                
                // Assume water near map edges or based on elevation
                if (distance_from_center > 0.8f) {
                    return true; // Near map boundary (likely ocean)
                }
            }
        }
        return false; // Default to inland
    }
};

}  // namespace Manifest::World::Terrain

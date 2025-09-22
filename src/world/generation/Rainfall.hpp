#pragma once

#include <cmath>
#include <numbers>
#include <random>
#include <vector>

#include "../../core/math/Vector.hpp"
#include "../../core/types/Types.hpp"
#include "../tiles/Map.hpp"

namespace Manifest::World::Generation {

using Core::Math::Vec2f;
using Core::Types::Quantity;
using Core::Types::TileId;
using Tiles::Tile;
using Tiles::TileMap;

// Strong types for atmospheric properties
struct PressureTag {};
struct HumidityTag {};
struct WindSpeedTag {};
struct PrecipitationTag {};
struct EvaporationTag {};

using Pressure = Quantity<PressureTag, float>;
using Humidity = Quantity<HumidityTag, float>;
using WindSpeed = Quantity<WindSpeedTag, float>;
using Precipitation = Quantity<PrecipitationTag, float>;
using Evaporation = Quantity<EvaporationTag, float>;

// Atmospheric state for each tile
struct alignas(32) AtmosphericData {
    Pressure pressure{1013.25F};      // Standard atmospheric pressure in hPa
    Humidity humidity{0.0F};          // Relative humidity [0, 1]
    Vec2f wind_velocity{0.0F, 0.0F};  // Wind velocity vector
    Precipitation precipitation{0.0F}; // mm/time_step
    float temperature{288.15F};       // Kelvin
};

// Weather front types
enum class FrontType : std::uint8_t {
    Cold,
    Warm, 
    Occluded,
    Stationary
};

// Pressure system types  
enum class PressureSystemType : std::uint8_t {
    High,
    Low,
    Ridge,
    Trough
};

struct alignas(32) PressureSystem {
    PressureSystemType type;
    Vec2f center;
    float intensity;
    float radius;
    Vec2f velocity;
    std::array<char, 3> padding; // Ensure proper alignment
};

class Atmosphere {
    std::vector<PressureSystem> systems_;
    std::mt19937 rng_;
    float map_radius_;

public:
    explicit Atmosphere(std::uint32_t seed, float map_radius) 
        : rng_{seed}, map_radius_{map_radius} {}

    void generate_pressure_systems(std::size_t count = 8) {
        systems_.clear();
        systems_.reserve(count);

        std::uniform_real_distribution<float> coord_dist(-map_radius_, map_radius_);
        std::uniform_real_distribution<float> type_dist(0.0F, 1.0F);
        std::uniform_real_distribution<float> intensity_dist(980.0F, 1040.0F);
        std::uniform_real_distribution<float> radius_dist(map_radius_ * 0.2F, map_radius_ * 0.6F);

#pragma unroll 8
        for (std::size_t i = 0; i < count; ++i) {
            PressureSystem system{
                .type = type_dist(rng_) > 0.5F ? PressureSystemType::High : PressureSystemType::Low,
                .center = Vec2f{coord_dist(rng_), coord_dist(rng_)},
                .intensity = intensity_dist(rng_),
                .radius = radius_dist(rng_),
                .velocity = generate_system_velocity(),
                .padding = {0, 0, 0}
            };
            systems_.push_back(system);
        }
    }

    Pressure calculate_pressure(const Vec2f& position) const {
        float base_pressure = 1013.25F;
        
#pragma unroll 8
        for (const auto& system : systems_) {
            float distance = (position - system.center).length();
            if (distance < system.radius) {
                float influence = 1.0F - (distance / system.radius);
                float pressure_delta = (system.intensity - base_pressure) * influence;
                
                if (system.type == PressureSystemType::Low) {
                    pressure_delta = -std::abs(pressure_delta);
                }
                
                base_pressure += pressure_delta;
            }
        }
        
        return Pressure{base_pressure};
    }

    Vec2f calculate_wind(const Vec2f& position) const {
        Vec2f total_wind{0.0F, 0.0F};

#pragma unroll 8
        for (const auto& system : systems_) {
            Vec2f to_center = system.center - position;
            float distance = to_center.length();
            
            if (distance < system.radius && distance > 0.1F) {
                float influence = 1.0F - (distance / system.radius);
                float wind_speed = system.intensity * 0.01F * influence;

                // Geostrophic wind (perpendicular to pressure gradient)
                Vec2f wind_direction = Vec2f{-to_center.y(), to_center.x()}.normalized();
                
                // Cyclonic (low pressure) vs anticyclonic (high pressure)
                if (system.type == PressureSystemType::Low) {
                    wind_direction = wind_direction * -1.0F;
                }

                total_wind += wind_direction * wind_speed;
            }
        }

        return total_wind;
    }

    void update(float time_step) {
#pragma unroll 8
        for (auto& system : systems_) {
            system.center += system.velocity * time_step;
        }
    }

    const std::vector<PressureSystem>& systems() const { return systems_; }

private:
    Vec2f generate_system_velocity() {
        std::uniform_real_distribution<float> speed_dist(0.1F, 0.5F);
        std::uniform_real_distribution<float> angle_dist(0.0F, 2.0F * std::numbers::pi_v<float>);
        
        float speed = speed_dist(rng_);
        float angle = angle_dist(rng_);
        
        return Vec2f{speed * std::cos(angle), speed * std::sin(angle)};
    }
};

class Hydrology {
    std::mt19937 rng_;

public:
    explicit Hydrology(std::uint32_t seed) : rng_{seed} {}

    static Evaporation calculate_evaporation(const Tile& tile) {
        float temperature = static_cast<float>(tile.temperature()) / 255.0F;
        float base_evaporation = 0.0F;

        if (tile.is_water()) {
            // Water evaporation based on temperature
            base_evaporation = std::pow(temperature, 2.0F) * 10.0F;
        } else {
            // Land evapotranspiration
            float vegetation_factor = get_vegetation_factor(tile.terrain());
            base_evaporation = temperature * vegetation_factor * 3.0F;
        }

        return Evaporation{base_evaporation};
    }

    static Humidity calculate_humidity(const Vec2f& position, const std::vector<const Tile*>& nearby_water,
                                      float temperature) {
        float base_humidity = 0.1F;

        // Add humidity from nearby water bodies
#pragma unroll 8
        for (const auto* water_tile : nearby_water) {
            Vec2f water_pos{static_cast<float>(water_tile->coordinate().q), 
                           static_cast<float>(water_tile->coordinate().r)};
            float distance = (position - water_pos).length();
            
            if (distance < 10.0F) {
                float influence = 1.0F - (distance / 10.0F);
                float water_humidity = Hydrology::calculate_evaporation(*water_tile).value() * 0.1F;
                base_humidity += water_humidity * influence;
            }
        }

        // Temperature affects humidity capacity
        float humidity_capacity = std::exp(17.27F * (temperature - 273.15F) / (temperature + 35.86F));
        return Humidity{std::clamp(base_humidity / humidity_capacity, 0.0F, 1.0F)};
    }

    static Precipitation calculate_precipitation(Humidity humidity, const Vec2f& wind_velocity,
                                              float elevation_change, Pressure pressure) {
        float precip = 0.0F;

        // Base precipitation from humidity and pressure
        if (humidity.value() > 0.6F) {
            float humidity_factor = (humidity.value() - 0.6F) * 2.5F;
            float pressure_factor = std::max(0.0F, 1020.0F - pressure.value()) * 0.01F;
            precip = humidity_factor * pressure_factor;
        }

        // Orographic precipitation
        if (elevation_change > 0.0F) {
            float orographic_lift = elevation_change * 0.1F * wind_velocity.length();
            precip += orographic_lift * humidity.value();
        }

        return Precipitation{std::max(0.0F, precip)};
    }

private:
    static float get_vegetation_factor(Tiles::TerrainType terrain) {
        switch (terrain) {
            case Tiles::TerrainType::Jungle: return 2.5F;
            case Tiles::TerrainType::Forest: return 2.0F;
            case Tiles::TerrainType::Grassland: return 1.5F;
            case Tiles::TerrainType::Plains: return 1.0F;
            case Tiles::TerrainType::Hills: return 0.8F;
            case Tiles::TerrainType::Desert: return 0.1F;
            case Tiles::TerrainType::Tundra: return 0.3F;
            case Tiles::TerrainType::Mountains: return 0.2F;
            default: return 0.5F;
        }
    }
};

class Orography {
public:
    static float calculate_elevation_gradient(const Tile& tile, const Vec2f& wind_direction,
                                            const TileMap& map) {
        float current_elevation = static_cast<float>(tile.elevation()) / 255.0F;
        
        // Find upwind neighbor
        Core::Types::Direction upwind_dir = wind_to_direction(wind_direction);
        TileId upwind_id = tile.neighbor(upwind_dir);
        
        if (!upwind_id.is_valid()) {
            return 0.0F;
        }
        
        const Tile* upwind_tile = map.get_tile(upwind_id);
        if (upwind_tile == nullptr) {
            return 0.0F;
        }
        
        float upwind_elevation = static_cast<float>(upwind_tile->elevation()) / 255.0F;
        return current_elevation - upwind_elevation;
    }

    static float calculate_rain_shadow(const Tile& tile, const Vec2f& wind_direction,
                                     const TileMap& map, float max_distance = 5.0F) {
        Vec2f position{static_cast<float>(tile.coordinate().q), 
                      static_cast<float>(tile.coordinate().r)};
        
        float shadow_factor = 1.0F;
        float current_elevation = static_cast<float>(tile.elevation()) / 255.0F;

        // Check for higher terrain upwind
#pragma unroll 8
        for (std::int32_t distance_int = 1; distance_int <= static_cast<std::int32_t>(max_distance); ++distance_int) {
            auto distance = static_cast<float>(distance_int);
            Vec2f upwind_pos = position - wind_direction.normalized() * distance;
            Core::Types::HexCoordinate upwind_coord{
                static_cast<std::int32_t>(std::round(upwind_pos.x())),
                static_cast<std::int32_t>(std::round(upwind_pos.y())),
                -static_cast<std::int32_t>(std::round(upwind_pos.x() + upwind_pos.y()))
            };
            
            if (!upwind_coord.is_valid()) {
                continue;
            }
            
            const Tile* upwind_tile = map.get_tile(upwind_coord);
            if (upwind_tile == nullptr) {
                continue;
            }
            
            float upwind_elevation = static_cast<float>(upwind_tile->elevation()) / 255.0F;
            if (upwind_elevation > current_elevation + 0.1F) {
                float elevation_diff = upwind_elevation - current_elevation;
                float distance_factor = 1.0F - (distance / max_distance);
                shadow_factor *= (1.0F - elevation_diff * distance_factor * 0.5F);
            }
        }

        return std::max(0.1F, shadow_factor);
    }

private:
    static Core::Types::Direction wind_to_direction(const Vec2f& wind) {
        float angle = std::atan2(wind.y(), wind.x());
        if (angle < 0) {
            angle += 2.0F * std::numbers::pi_v<float>;
        }
        
        int dir_index = static_cast<int>(std::round(angle * 6.0F / (2.0F * std::numbers::pi_v<float>))) % 6;
        return static_cast<Core::Types::Direction>(dir_index);
    }
};

class Rainfall {
    Atmosphere atmosphere_;
    Hydrology hydrology_;
    std::unordered_map<TileId, AtmosphericData> atmospheric_data_;

public:
    explicit Rainfall(std::uint32_t seed, float map_radius)
        : atmosphere_{seed, map_radius}, hydrology_{seed + 1000} {}

    template <typename MapType>
    void simulate(MapType& map, std::size_t iterations = 10) {
        MODULE_LOGGER("RainfallSystem");
        
        logger_->debug("Starting rainfall simulation: {} iterations for {} tiles", 
                      iterations, map.size());
        
        // Initialize atmospheric systems
        logger_->trace("Generating pressure systems");
        atmosphere_.generate_pressure_systems();
        logger_->trace("Initializing atmospheric data");
        initialize_atmospheric_data(map);

        // Run simulation iterations
        logger_->debug("Running atmospheric simulation iterations");
#pragma unroll 8
        for (std::size_t i = 0; i < iterations; ++i) {
            update_atmospheric_data(map);
            calculate_precipitation_for_map(map);
            atmosphere_.update(0.1F);
            
            if ((i + 1) % 5 == 0) {
                logger_->trace("Rainfall simulation progress: {}/{} iterations", i + 1, iterations);
            }
        }

        // Apply final rainfall values to tiles
        logger_->trace("Applying final rainfall values to map tiles");
        apply_rainfall_to_tiles(map);
        logger_->debug("Rainfall simulation completed successfully");
    }

private:
    template <typename MapType>
    void initialize_atmospheric_data(const MapType& map) {
        atmospheric_data_.clear();
        
        map.for_each_tile([this](const Tile& tile) {
            Vec2f position{static_cast<float>(tile.coordinate().q),
                          static_cast<float>(tile.coordinate().r)};
            
            AtmosphericData data{
                .pressure = atmosphere_.calculate_pressure(position),
                .humidity = Humidity{0.5F}, // Will be calculated properly in update
                .wind_velocity = atmosphere_.calculate_wind(position),
                .precipitation = Precipitation{0.0F},
                .temperature = (static_cast<float>(tile.temperature()) / 255.0F * 40.0F) + 253.15F // Convert to Kelvin
            };
            
            atmospheric_data_[tile.id()] = data;
        });
    }

    template <typename MapType>
    void update_atmospheric_data(const MapType& map) {
        map.for_each_tile([this, &map](const Tile& tile) {
            auto& data = atmospheric_data_[tile.id()];
            Vec2f position{static_cast<float>(tile.coordinate().q),
                          static_cast<float>(tile.coordinate().r)};

            // Update pressure and wind
            data.pressure = atmosphere_.calculate_pressure(position);
            data.wind_velocity = atmosphere_.calculate_wind(position);

            // Find nearby water for humidity calculation
            std::vector<const Tile*> nearby_water;
            for (std::int32_t dq = -5; dq <= 5; ++dq) {
    #pragma unroll 4
            for (std::int32_t dr = -5; dr <= 5; ++dr) {
                    Core::Types::HexCoordinate coord{
                        tile.coordinate().q + dq,
                        tile.coordinate().r + dr,
                        tile.coordinate().s - dq - dr
                    };
                    
                    if (coord.is_valid()) {
                        const Tile* nearby_tile = map.get_tile(coord);
                        if (nearby_tile != nullptr && nearby_tile->is_water()) {
                            nearby_water.push_back(nearby_tile);
                        }
                    }
                }
            }

            data.humidity = Hydrology::calculate_humidity(position, nearby_water, data.temperature);
        });
    }

    template <typename MapType>
    void calculate_precipitation_for_map(const MapType& map) {
        map.for_each_tile([this, &map](const Tile& tile) {
            auto& data = atmospheric_data_[tile.id()];

            // Calculate orographic effects
            float elevation_gradient = Orography::calculate_elevation_gradient(
                tile, data.wind_velocity, map);
            float rain_shadow = Orography::calculate_rain_shadow(
                tile, data.wind_velocity, map);

            // Calculate base precipitation
            Precipitation base_precip = Hydrology::calculate_precipitation(
                data.humidity, data.wind_velocity, elevation_gradient, data.pressure);

            // Apply rain shadow effect
            data.precipitation = Precipitation{base_precip.value() * rain_shadow};
        });
    }

    template <typename MapType>
    void apply_rainfall_to_tiles(MapType& map) {
        map.for_each_tile([this](Tile& tile) {
            const auto& data = atmospheric_data_[tile.id()];
            
            // Convert precipitation to tile rainfall value (0-255)
            float normalized_rainfall = std::clamp(data.precipitation.value() / 10.0F, 0.0F, 1.0F);
            tile.set_rainfall(static_cast<std::uint8_t>(normalized_rainfall * 255.0F));
        });
    }
};

} // namespace Manifest::World::Generation

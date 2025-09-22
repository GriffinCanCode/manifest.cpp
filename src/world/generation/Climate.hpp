#pragma once

#include <array>
#include <cmath>
#include <numbers>

#include "../../core/math/Vector.hpp" 
#include "../../core/types/Types.hpp"

namespace Manifest::World::Generation {

using Core::Math::Vec2f;
using Core::Types::HexCoordinate;
using Core::Types::Quantity;

// Strong types for climate classification
struct LatitudeTag {};
struct TemperatureModifierTag {};
struct SeasonalityTag {};
struct HumidityTag {};

using LatitudeValue = Quantity<LatitudeTag, float>;
using TemperatureModifier = Quantity<TemperatureModifierTag, float>;
using Seasonality = Quantity<SeasonalityTag, float>;
using Humidity = Quantity<HumidityTag, float>;

// Climate zone classifications following Köppen system
enum class ClimateZone : std::uint8_t {
    // Tropical (A) - no winter season, all months > 18°C
    TropicalRainforest,    // Af - high precipitation year-round
    TropicalMonsoon,       // Am - seasonal wet/dry, high annual precipitation
    TropicalSavanna,       // Aw/As - distinct wet and dry seasons
    
    // Arid (B) - low precipitation
    HotDesert,             // BWh - hot arid
    ColdDesert,            // BWk - cold arid  
    HotSemiArid,           // BSh - hot semi-arid
    ColdSemiArid,          // BSk - cold semi-arid
    
    // Temperate (C) - mild winters, 3 coldest months between -3°C and 18°C
    Mediterranean,         // Csa/Csb - dry summer subtropical
    HumidSubtropical,      // Cfa/Cwa - humid subtropical
    OceanicTemperate,      // Cfb/Cfc - oceanic climate
    ContinentalWarm,       // Dfb - warm-summer humid continental
    
    // Continental (D) - severe winters, coldest month < -3°C
    ContinentalCold,       // Dfa/Dfb - hot/warm summer continental
    Subarctic,             // Dfc/Dfd - subarctic
    
    // Polar (E) - no warm season, all months < 10°C
    Tundra,                // ET - tundra climate
    IceCap,                // EF - ice cap climate
    
    // Highland (H) - significant elevation effects
    Highland               // H - highland climate
};

struct alignas(32) ClimateData {
    ClimateZone zone{};
    LatitudeValue latitude_min{-90.0F};
    LatitudeValue latitude_max{90.0F};
    float temperature_annual_mean{};     // °C
    float temperature_range{};           // °C seasonal variation
    float precipitation_annual{};        // mm/year
    Seasonality precipitation_pattern{}; // 0.0=even, 1.0=highly seasonal
    Humidity relative_humidity{};        // 0.0-1.0
    float elevation_modifier{};          // climate shifts per 100m elevation
    bool requires_coastal{};             // needs ocean proximity
};

class Climate {
    static constexpr std::array<ClimateData, 17> climate_definitions = {{
        // Tropical zones (0°-23.5°)
        {ClimateZone::TropicalRainforest, LatitudeValue{-23.5F}, LatitudeValue{23.5F}, 
         26.0F, 3.0F, 2000.0F, Seasonality{0.2F}, Humidity{0.8F}, -6.5F, false},
        
        {ClimateZone::TropicalMonsoon, LatitudeValue{-23.5F}, LatitudeValue{23.5F},
         27.0F, 5.0F, 1800.0F, Seasonality{0.8F}, Humidity{0.7F}, -6.5F, true},
         
        {ClimateZone::TropicalSavanna, LatitudeValue{-23.5F}, LatitudeValue{23.5F},
         24.0F, 8.0F, 1200.0F, Seasonality{0.9F}, Humidity{0.6F}, -6.5F, false},
        
        // Arid zones (variable latitudes)
        {ClimateZone::HotDesert, LatitudeValue{-35.0F}, LatitudeValue{35.0F},
         28.0F, 20.0F, 150.0F, Seasonality{0.3F}, Humidity{0.3F}, -5.0F, false},
         
        {ClimateZone::ColdDesert, LatitudeValue{-60.0F}, LatitudeValue{60.0F},
         8.0F, 25.0F, 200.0F, Seasonality{0.4F}, Humidity{0.4F}, -6.0F, false},
         
        {ClimateZone::HotSemiArid, LatitudeValue{-40.0F}, LatitudeValue{40.0F},
         22.0F, 18.0F, 400.0F, Seasonality{0.5F}, Humidity{0.4F}, -5.5F, false},
         
        {ClimateZone::ColdSemiArid, LatitudeValue{-55.0F}, LatitudeValue{55.0F},
         12.0F, 22.0F, 350.0F, Seasonality{0.6F}, Humidity{0.4F}, -6.0F, false},
        
        // Temperate zones (23.5°-66.5°)
        {ClimateZone::Mediterranean, LatitudeValue{-45.0F}, LatitudeValue{45.0F},
         16.0F, 12.0F, 600.0F, Seasonality{0.7F}, Humidity{0.6F}, -6.5F, true},
         
        {ClimateZone::HumidSubtropical, LatitudeValue{-40.0F}, LatitudeValue{40.0F},
         18.0F, 15.0F, 1000.0F, Seasonality{0.3F}, Humidity{0.7F}, -6.5F, false},
         
        {ClimateZone::OceanicTemperate, LatitudeValue{-60.0F}, LatitudeValue{60.0F},
         12.0F, 8.0F, 1200.0F, Seasonality{0.2F}, Humidity{0.8F}, -5.5F, true},
         
        {ClimateZone::ContinentalWarm, LatitudeValue{-60.0F}, LatitudeValue{60.0F},
         10.0F, 25.0F, 800.0F, Seasonality{0.4F}, Humidity{0.6F}, -6.5F, false},
        
        // Continental zones (45°-66.5°)
        {ClimateZone::ContinentalCold, LatitudeValue{-66.5F}, LatitudeValue{66.5F},
         2.0F, 30.0F, 600.0F, Seasonality{0.5F}, Humidity{0.6F}, -6.5F, false},
         
        {ClimateZone::Subarctic, LatitudeValue{-70.0F}, LatitudeValue{70.0F},
         -5.0F, 35.0F, 450.0F, Seasonality{0.6F}, Humidity{0.6F}, -6.5F, false},
        
        // Polar zones (66.5°-90°)
        {ClimateZone::Tundra, LatitudeValue{-90.0F}, LatitudeValue{-60.0F},
         -10.0F, 25.0F, 250.0F, Seasonality{0.7F}, Humidity{0.7F}, -5.0F, false},
         
        {ClimateZone::IceCap, LatitudeValue{-90.0F}, LatitudeValue{-75.0F},
         -20.0F, 15.0F, 100.0F, Seasonality{0.4F}, Humidity{0.8F}, -4.0F, false},
        
        // Highland (elevation-dependent)
        {ClimateZone::Highland, LatitudeValue{-90.0F}, LatitudeValue{90.0F},
         5.0F, 20.0F, 800.0F, Seasonality{0.5F}, Humidity{0.6F}, -6.5F, false}
    }};

public:
    // Calculate latitude from hex coordinate
    static constexpr LatitudeValue calculate_latitude(const HexCoordinate& coord, float map_radius) noexcept {
        // Convert hex r-coordinate to latitude (-90° to +90°)
        // Assuming equator at r=0, poles at r=±map_radius
        float normalized_r = static_cast<float>(coord.r) / map_radius;
        float latitude_degrees = std::clamp(normalized_r * 90.0F, -90.0F, 90.0F);
        return LatitudeValue{latitude_degrees};
    }

    // Determine climate zone for a coordinate
    static ClimateZone classify_climate(const HexCoordinate& coord, 
                                       float map_radius,
                                       float elevation_normalized,
                                       float annual_precipitation,
                                       bool is_coastal) noexcept {
        
        LatitudeValue lat = calculate_latitude(coord, map_radius);
        float abs_latitude = std::abs(lat.value());
        
        // Highland climate for high elevations
        if (elevation_normalized > 0.75F) {
            return ClimateZone::Highland;
        }
        
        // Polar climates (>66.5°)
        if (abs_latitude > 66.5F) {
            return elevation_normalized > 0.8F ? ClimateZone::IceCap : ClimateZone::Tundra;
        }
        
        // Subarctic (50°-66.5°)
        if (abs_latitude > 50.0F) {
            return ClimateZone::Subarctic;
        }
        
        // Continental (30°-60°) - depends on continentality
        if (abs_latitude > 30.0F && abs_latitude <= 60.0F) {
            if (is_coastal && annual_precipitation > 800.0F) {
                return ClimateZone::OceanicTemperate;
            }
            
            // Check for Mediterranean (dry summers, wet winters)
            if (is_coastal && abs_latitude > 30.0F && abs_latitude < 45.0F) {
                return ClimateZone::Mediterranean;
            }
            
            return annual_precipitation > 500.0F ? 
                   ClimateZone::ContinentalWarm : ClimateZone::ColdSemiArid;
        }
        
        // Arid classification (multiple latitudes)
        if (annual_precipitation < 250.0F) {
            float temperature_estimate = 30.0F - abs_latitude * 0.8F; // rough estimate
            return temperature_estimate > 15.0F ? 
                   ClimateZone::HotDesert : ClimateZone::ColdDesert;
        }
        if (annual_precipitation < 600.0F) {
            float temperature_estimate = 30.0F - abs_latitude * 0.8F;
            return temperature_estimate > 15.0F ? 
                   ClimateZone::HotSemiArid : ClimateZone::ColdSemiArid;
        }
        
        // Tropical (0°-23.5°)
        if (abs_latitude <= 23.5F) {
            if (annual_precipitation > 1800.0F) {
                return ClimateZone::TropicalRainforest;
            }
            if (annual_precipitation > 1200.0F && is_coastal) {
                return ClimateZone::TropicalMonsoon;
            }
            return ClimateZone::TropicalSavanna;
        }
        
        // Subtropical/Temperate (23.5°-45°)
        if (is_coastal) {
            return annual_precipitation > 1000.0F ? 
                   ClimateZone::HumidSubtropical : ClimateZone::Mediterranean;
        }
        
        return ClimateZone::ContinentalWarm; // Default for remaining cases
    }

    // Get climate data for a zone
    static const ClimateData& get_climate_data(ClimateZone zone) noexcept {
        auto it = std::find_if(climate_definitions.begin(), climate_definitions.end(),
                              [zone](const ClimateData& data) { return data.zone == zone; });
        return it != climate_definitions.end() ? *it : climate_definitions[0];
    }

    // Calculate temperature with elevation and latitude effects
    static float calculate_temperature(ClimateZone zone,
                                     LatitudeValue latitude, 
                                     float elevation_normalized,
                                     float seasonal_factor = 0.0F) noexcept {
        const auto& climate_data = get_climate_data(zone);
        
        // Base temperature from climate zone
        float base_temp = climate_data.temperature_annual_mean;
        
        // Latitude effect (additional cooling beyond zone classification)
        float latitude_effect = std::abs(latitude.value()) * 0.2F;
        base_temp -= latitude_effect;
        
        // Elevation lapse rate (temperature decreases with altitude)
        float elevation_effect = elevation_normalized * climate_data.elevation_modifier;
        base_temp += elevation_effect;
        
        // Seasonal variation
        float seasonal_variation = seasonal_factor * climate_data.temperature_range * 0.5F;
        base_temp += seasonal_variation;
        
        return base_temp;
    }

private:
    // Check if coordinate is near coast (simplified heuristic)
    template<typename MapType>
    static bool is_near_coast(const HexCoordinate& coord, [[maybe_unused]] const MapType& map, [[maybe_unused]] int search_radius = 3) noexcept {
        // This would need to be implemented with actual map querying
        // For now, return a simplified heuristic based on coordinate
        return (coord.q + coord.r) % 7 == 0; // Simplified placeholder
    }
};

} // namespace Manifest::World::Generation

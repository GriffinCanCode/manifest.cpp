#pragma once

#include <cmath>
#include <numbers>

#include "../../core/math/Vector.hpp"
#include "../../core/types/Types.hpp"

namespace Manifest::World::Generation {

using Core::Math::Vec2f;
using Core::Types::HexCoordinate;
using Core::Types::Quantity;

// Strong types for geographical coordinates
struct LongitudeTag {};
struct DistanceTag {};
struct AngleTag {};

using Longitude = Quantity<LongitudeTag, float>;
using Distance = Quantity<DistanceTag, float>;
using Angle = Quantity<AngleTag, float>;

// Geographic coordinate system utilities
class Latitude {
public:
    // Convert hex coordinate to geographic latitude/longitude
    static constexpr Vec2f hex_to_geographic(const HexCoordinate& hex, 
                                           float map_radius,
                                           float world_size_km = 40075.0F) noexcept {
        // Normalize hex coordinates to [-1, 1] range
        float normalized_r = static_cast<float>(hex.r) / map_radius;
        float normalized_q = static_cast<float>(hex.q) / map_radius;
        
        // Convert to latitude/longitude (-90° to +90° lat, -180° to +180° lon)
        float latitude = std::clamp(normalized_r * 90.0F, -90.0F, 90.0F);
        float longitude = std::clamp(normalized_q * 180.0F, -180.0F, 180.0F);
        
        return Vec2f{latitude, longitude};
    }

    // Calculate precise latitude from hex coordinate with spherical projection
    static constexpr float calculate_spherical_latitude(const HexCoordinate& hex,
                                                       float map_radius) noexcept {
        // Use Mercator-like projection for more realistic latitude distribution
        float normalized_r = static_cast<float>(hex.r) / map_radius;
        
        // Apply inverse hyperbolic tangent for Mercator-like distribution
        // This gives more realistic pole compression
        if (std::abs(normalized_r) > 0.99F) {
            return normalized_r > 0 ? 89.0F : -89.0F; // Avoid singularity at poles
        }
        
        float mercator_y = normalized_r * std::numbers::pi_v<float>;
        float latitude = (2.0F * std::atan(std::exp(mercator_y)) - std::numbers::pi_v<float> * 0.5F) 
                        * (180.0F / std::numbers::pi_v<float>);
        
        return std::clamp(latitude, -90.0F, 90.0F);
    }

    // Calculate distance between two latitude points on sphere
    static constexpr Distance latitude_distance(float lat1_deg, float lat2_deg,
                                               float earth_radius_km = 6371.0F) noexcept {
        float lat1_rad = lat1_deg * std::numbers::pi_v<float> / 180.0F;
        float lat2_rad = lat2_deg * std::numbers::pi_v<float> / 180.0F;
        
        float delta_lat = lat2_rad - lat1_rad;
        float distance = earth_radius_km * std::abs(delta_lat);
        
        return Distance{distance};
    }

    // Get climate latitude bands based on Trewartha climate classification
    static constexpr std::pair<float, float> get_climate_band(const HexCoordinate& hex,
                                                             float map_radius) noexcept {
        float lat = calculate_spherical_latitude(hex, map_radius);
        float abs_lat = std::abs(lat);
        
        // Return band center and width
        if (abs_lat <= 10.0F) {
            return {0.0F, 20.0F};  // Tropical
        }
        if (abs_lat <= 23.5F) {
            return {lat > 0 ? 17.0F : -17.0F, 13.0F};  // Subtropical
        }
        if (abs_lat <= 35.0F) {
            return {lat > 0 ? 29.0F : -29.0F, 12.0F};  // Warm temperate
        }
        if (abs_lat <= 50.0F) {
            return {lat > 0 ? 42.5F : -42.5F, 15.0F};  // Cool temperate
        }
        if (abs_lat <= 66.5F) {
            return {lat > 0 ? 58.0F : -58.0F, 16.5F};  // Subarctic
        }
        
        return {lat > 0 ? 78.0F : -78.0F, 23.0F};  // Polar
    }

    // Calculate solar angle (affects temperature and seasons)
    static constexpr Angle solar_angle(const HexCoordinate& hex, 
                                      float map_radius,
                                      float day_of_year = 180.0F) noexcept {
        float latitude_rad = calculate_spherical_latitude(hex, map_radius) 
                           * std::numbers::pi_v<float> / 180.0F;
        
        // Solar declination angle (varies by season)
        float declination = 23.5F * std::sin(2.0F * std::numbers::pi_v<float> 
                                           * (day_of_year - 81.0F) / 365.0F);
        float declination_rad = declination * std::numbers::pi_v<float> / 180.0F;
        
        // Solar elevation angle at solar noon
        float solar_elevation = std::asin(
            std::sin(latitude_rad) * std::sin(declination_rad) +
            std::cos(latitude_rad) * std::cos(declination_rad)
        );
        
        return Angle{solar_elevation * 180.0F / std::numbers::pi_v<float>};
    }

    // Calculate daylight hours for latitude
    static constexpr float daylight_hours(const HexCoordinate& hex,
                                        float map_radius,
                                        float day_of_year = 180.0F) noexcept {
        float latitude_rad = calculate_spherical_latitude(hex, map_radius)
                           * std::numbers::pi_v<float> / 180.0F;
        
        float declination = 23.5F * std::sin(2.0F * std::numbers::pi_v<float>
                                           * (day_of_year - 81.0F) / 365.0F);
        float declination_rad = declination * std::numbers::pi_v<float> / 180.0F;
        
        // Hour angle
        float cos_hour_angle = -std::tan(latitude_rad) * std::tan(declination_rad);
        
        // Handle polar day/night
        if (cos_hour_angle > 1.0F) return 0.0F;   // Polar night
        if (cos_hour_angle < -1.0F) return 24.0F;  // Polar day
        
        float hour_angle = std::acos(cos_hour_angle);
        return 2.0F * hour_angle * 12.0F / std::numbers::pi_v<float>;
    }

    // Determine hemisphere from coordinate  
    enum class Hemisphere : std::uint8_t { Northern, Southern, Equatorial };
    
    static constexpr Hemisphere get_hemisphere(const HexCoordinate& hex, 
                                              float map_radius,
                                              float equatorial_band = 5.0F) noexcept {
        float lat = calculate_spherical_latitude(hex, map_radius);
        
        if (std::abs(lat) <= equatorial_band) {
            return Hemisphere::Equatorial;
        }
        
        return lat > 0.0F ? Hemisphere::Northern : Hemisphere::Southern;
    }

    // Calculate temperature modification factor from latitude
    static constexpr float temperature_latitude_factor(const HexCoordinate& hex,
                                                      float map_radius) noexcept {
        float lat = calculate_spherical_latitude(hex, map_radius);
        float abs_lat = std::abs(lat);
        
        // Temperature decreases roughly 0.5°C per degree latitude from equator
        // Using cosine for more realistic distribution
        float latitude_rad = abs_lat * std::numbers::pi_v<float> / 180.0F;
        return std::cos(latitude_rad * 0.5F); // Range: 1.0 (equator) to ~0.7 (poles)
    }

    // Get Köppen-Geiger climate influence from latitude
    struct LatitudeClimateInfluence {
        float tropical_weight{};      // 0-1, strongest near equator
        float temperate_weight{};     // 0-1, strongest in mid-latitudes
        float polar_weight{};         // 0-1, strongest near poles
        float seasonal_variation{};   // 0-1, temperature range factor
    };

    static constexpr LatitudeClimateInfluence climate_influence(const HexCoordinate& hex,
                                                              float map_radius) noexcept {
        float abs_lat = std::abs(calculate_spherical_latitude(hex, map_radius));
        
        LatitudeClimateInfluence influence{};
        
        // Tropical influence (strongest 0-20°, fades by 30°)
        influence.tropical_weight = std::max(0.0F, (30.0F - abs_lat) / 30.0F);
        
        // Temperate influence (peaks around 45°)
        float temperate_center = 45.0F;
        float temperate_distance = std::abs(abs_lat - temperate_center);
        influence.temperate_weight = std::max(0.0F, (30.0F - temperate_distance) / 30.0F);
        
        // Polar influence (starts at 60°, peaks at 90°)
        influence.polar_weight = std::max(0.0F, (abs_lat - 50.0F) / 40.0F);
        
        // Seasonal variation (minimal at equator, maximum at poles)
        influence.seasonal_variation = abs_lat / 90.0F;
        
        // Normalize weights
        float total_weight = influence.tropical_weight + influence.temperate_weight + influence.polar_weight;
        if (total_weight > 0.0F) {
            influence.tropical_weight /= total_weight;
            influence.temperate_weight /= total_weight; 
            influence.polar_weight /= total_weight;
        }
        
        return influence;
    }
};

} // namespace Manifest::World::Generation

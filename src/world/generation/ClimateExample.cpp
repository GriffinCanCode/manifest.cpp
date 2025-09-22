// Climate Zones System Integration Example
// This file demonstrates the complete climate system implementation

#include "Climate.hpp"
#include "Latitude.hpp"
#include "Geology.hpp"
#include "../tiles/Tile.hpp"

namespace Manifest::World::Generation {

// Example function showing how climate zones work with the terrain generator
void demonstrate_climate_system() {
    const float MAP_RADIUS = 100.0F;
    
    // Example coordinates for different climate regions
    Core::Types::HexCoordinate equatorial{0, 0, 0};           // Equator
    Core::Types::HexCoordinate temperate{0, 45, -45};         // Mid-latitude
    Core::Types::HexCoordinate polar{0, 80, -80};             // High latitude
    Core::Types::HexCoordinate highland{0, 30, -30};          // Mountain region
    
    // === Climate Zone Classification ===
    
    // Tropical region (high rainfall, sea level)
    ClimateZone tropical_climate = Climate::classify_climate(
        equatorial, MAP_RADIUS, 0.1F, 2200.0F, false
    );
    // Result: TropicalRainforest
    
    // Arid region (low rainfall, inland)
    ClimateZone arid_climate = Climate::classify_climate(
        temperate, MAP_RADIUS, 0.2F, 150.0F, false
    );
    // Result: HotDesert or HotSemiArid
    
    // Polar region (high latitude, moderate rainfall)
    ClimateZone polar_climate = Climate::classify_climate(
        polar, MAP_RADIUS, 0.1F, 300.0F, false
    );
    // Result: Tundra
    
    // Highland region (high elevation overrides latitude)
    ClimateZone highland_climate = Climate::classify_climate(
        highland, MAP_RADIUS, 0.85F, 800.0F, false
    );
    // Result: Highland
    
    // === Latitude-Based Calculations ===
    
    // Calculate precise spherical latitude
    float spherical_lat = Latitude::calculate_spherical_latitude(temperate, MAP_RADIUS);
    // More realistic latitude distribution using Mercator-like projection
    
    // Temperature factor based on latitude
    float temp_factor = Latitude::temperature_latitude_factor(temperate, MAP_RADIUS);
    // Range: 1.0 (equator) to ~0.7 (poles)
    
    // Climate influence weights
    auto influence = Latitude::climate_influence(temperate, MAP_RADIUS);
    // Returns: {tropical_weight, temperate_weight, polar_weight, seasonal_variation}
    
    // Solar calculations
    Angle summer_angle = Latitude::solar_angle(temperate, MAP_RADIUS, 172.0F);
    Angle winter_angle = Latitude::solar_angle(temperate, MAP_RADIUS, 355.0F);
    
    float summer_daylight = Latitude::daylight_hours(temperate, MAP_RADIUS, 172.0F);
    float winter_daylight = Latitude::daylight_hours(temperate, MAP_RADIUS, 355.0F);
    
    // === Temperature Calculation with Climate Effects ===
    
    auto lat_degree = Climate::calculate_latitude(temperate, MAP_RADIUS);
    
    float tropical_temp = Climate::calculate_temperature(
        ClimateZone::TropicalRainforest, lat_degree, 0.1F, 0.0F
    );
    // Result: ~25-28°C
    
    float polar_temp = Climate::calculate_temperature(
        ClimateZone::Tundra, lat_degree, 0.1F, -0.5F
    );
    // Result: ~-15°C to -5°C (winter factor applied)
    
    float highland_temp = Climate::calculate_temperature(
        ClimateZone::Highland, lat_degree, 0.9F, 0.0F
    );
    // Result: Significantly colder due to elevation lapse rate
    
    // === Geological Integration ===
    
    // Generate geological formation based on tectonic setting
    Formation continental_formation = Geology::generate_geology(
        temperate, MAP_RADIUS,
        120.0F,                    // plate age (Ma)
        false,                     // continental (not oceanic)
        Boundary::Transform,       // transform boundary
        180,                       // elevation (0-255)
        Terrain::Hills,           // terrain type
        false,                    // not coastal
        0.3F                      // noise value
    );
    // Result: Formation::Schist or Formation::Granite
    
    // Calculate resource probabilities for this formation
    auto resource_dist = Geology::calculate_resource_distribution(
        continental_formation,
        0.7F,                     // geological age factor
        0.4F,                     // mineral concentration
        0.2F                      // hydrothermal factor
    );
    // Result: Array of {Resource, probability} pairs
    
    // === Complete Tile Integration ===
    
    // Example of how a tile gets all its properties set:
    /*
    Tiles::Tile example_tile(Tiles::TileId{1}, temperate);
    
    // Physical properties from terrain generation
    example_tile.set_elevation(180);      // From elevation noise
    example_tile.set_temperature(165);    // From latitude + climate + elevation
    example_tile.set_rainfall(120);       // From rainfall patterns + latitude
    
    // Climate classification
    example_tile.set_climate_zone(arid_climate);
    
    // Terrain type based on elevation + climate
    example_tile.set_terrain(Tiles::TerrainType::Hills);
    
    // Resource from geological processes
    example_tile.set_resource(Tiles::ResourceType::Iron);
    */
    
    // === Climate Data Access ===
    
    const auto& climate_data = Climate::get_climate_data(ClimateZone::Mediterranean);
    // Access: temperature ranges, precipitation patterns, elevation effects, etc.
    
    // === System Benefits ===
    /*
    1. REALISTIC: Based on Köppen-Geiger climate classification
    2. EXTENSIBLE: Strong typing prevents mixing incompatible values  
    3. TESTABLE: Each component isolated and can be unit tested
    4. SOPHISTICATED: Complex interactions between latitude, elevation, geology
    5. PERFORMANT: Constexpr functions, efficient lookups, minimal memory
    6. CONSISTENT: All systems use same coordinate and type systems
    */
}

// Example usage in terrain generator integration:
/*
void TerrainGenerator::generate_climate(MapType& map) {
    map.for_each_tile([this, &map](Tile& tile) {
        const HexCoordinate& coord = tile.coordinate();
        
        // 1. Calculate latitude-based temperature
        float latitude_factor = Latitude::temperature_latitude_factor(coord, params_.map_radius);
        float temperature = latitude_factor * params_.temperature_gradient + noise_variation;
        tile.set_temperature(static_cast<std::uint8_t>(temperature * 255));
        
        // 2. Calculate latitude-modified rainfall  
        auto climate_influence = Latitude::climate_influence(coord, params_.map_radius);
        float rainfall = base_rainfall * (1.0F + climate_influence.tropical_weight * 0.5F - 
                                                 climate_influence.polar_weight * 0.3F);
        tile.set_rainfall(static_cast<std::uint8_t>(rainfall * 255));
        
        // 3. Classify climate zone
        ClimateZone zone = Climate::classify_climate(coord, params_.map_radius, 
                                                   elevation, annual_precipitation, is_coastal);
        tile.set_climate_zone(zone);
        
        // 4. Generate geological resources
        auto formation = Geology::generate_geology(coord, params_.map_radius, plate_age, 
                                                 is_oceanic, boundary_type, elevation, 
                                                 terrain, is_coastal, geology_noise);
        auto resource_dist = Geology::calculate_resource_distribution(formation, age_factor, 
                                                                    mineral_density, hydrothermal);
        ResourceType resource = select_resource_from_distribution(resource_dist, random_value);
        tile.set_resource(resource);
    });
}
*/

} // namespace Manifest::World::Generation

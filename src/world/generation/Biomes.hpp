#pragma once

#include <algorithm>
#include <array>

#include "../../core/types/Types.hpp"
#include "Rainfall.hpp"

namespace Manifest::World::Generation {

using Core::Types::Quantity;
using Tiles::Tile;
using Tiles::TerrainType;

// Strong types for biome classification
struct BiodiversityTag {};
struct AridsityTag {};
struct ContinentalityTag {};

using Biodiversity = Quantity<BiodiversityTag, float>;
using Aridity = Quantity<AridsityTag, float>;
using Continentality = Quantity<ContinentalityTag, float>;

// Enhanced biome types with more realistic classification
enum class BiomeType : std::uint8_t {
    // Arctic biomes
    IceCap,
    Tundra,
    BorealForest,
    
    // Temperate biomes
    TemperateRainforest,
    TemperateDeciduousForest,
    TemperateGrassland,
    MediterraneanScrub,
    
    // Arid biomes
    Desert,
    SemiDesert,
    Savanna,
    
    // Tropical biomes
    TropicalRainforest,
    TropicalDryForest,
    TropicalGrassland,
    
    // Montane biomes
    Alpine,
    MontaneForest,
    
    // Wetland biomes
    Wetlands,
    
    // Transitional biomes
    ParklandTundra,
    WoodlandSavanna
};

struct alignas(32) BiomeData {
    BiomeType biome{};
    Biodiversity biodiversity;
    float temperature_range_min{};
    float temperature_range_max{};
    float precipitation_range_min{};
    float precipitation_range_max{};
    float elevation_preference{};
    bool requires_water_nearby{};
};

class Biomes {
    static constexpr std::array<BiomeData, 17> biome_definitions = {{
        // Arctic biomes
        {BiomeType::IceCap, Biodiversity{0.1F}, -40.0F, -10.0F, 0.0F, 200.0F, 0.0F, false},
        {BiomeType::Tundra, Biodiversity{0.3F}, -15.0F, 5.0F, 100.0F, 400.0F, 0.1F, false},
        {BiomeType::BorealForest, Biodiversity{0.6F}, -10.0F, 15.0F, 300.0F, 800.0F, 0.2F, false},
        
        // Temperate biomes
        {BiomeType::TemperateRainforest, Biodiversity{0.9F}, 5.0F, 25.0F, 1200.0F, 3000.0F, 0.3F, true},
        {BiomeType::TemperateDeciduousForest, Biodiversity{0.8F}, 0.0F, 25.0F, 600.0F, 1500.0F, 0.25F, false},
        {BiomeType::TemperateGrassland, Biodiversity{0.5F}, -5.0F, 25.0F, 250.0F, 600.0F, 0.15F, false},
        {BiomeType::MediterraneanScrub, Biodiversity{0.6F}, 10.0F, 25.0F, 200.0F, 700.0F, 0.2F, false},
        
        // Arid biomes
        {BiomeType::Desert, Biodiversity{0.2F}, 10.0F, 50.0F, 0.0F, 200.0F, 0.1F, false},
        {BiomeType::SemiDesert, Biodiversity{0.3F}, 5.0F, 40.0F, 100.0F, 400.0F, 0.15F, false},
        {BiomeType::Savanna, Biodiversity{0.7F}, 15.0F, 35.0F, 400.0F, 1200.0F, 0.1F, false},
        
        // Tropical biomes
        {BiomeType::TropicalRainforest, Biodiversity{1.0F}, 20.0F, 35.0F, 1500.0F, 4000.0F, 0.0F, true},
        {BiomeType::TropicalDryForest, Biodiversity{0.7F}, 20.0F, 35.0F, 500.0F, 1500.0F, 0.1F, false},
        {BiomeType::TropicalGrassland, Biodiversity{0.6F}, 18.0F, 30.0F, 300.0F, 1000.0F, 0.05F, false},
        
        // Montane biomes
        {BiomeType::Alpine, Biodiversity{0.4F}, -10.0F, 10.0F, 300.0F, 1000.0F, 0.8F, false},
        {BiomeType::MontaneForest, Biodiversity{0.7F}, 0.0F, 20.0F, 600.0F, 2000.0F, 0.6F, false},
        
        // Wetland biomes
        {BiomeType::Wetlands, Biodiversity{0.8F}, 5.0F, 30.0F, 800.0F, 2500.0F, 0.0F, true},
        
        // Transitional biomes
        {BiomeType::ParklandTundra, Biodiversity{0.4F}, -10.0F, 10.0F, 200.0F, 600.0F, 0.2F, false}
    }};

public:
    template <typename MapType>
    static void classify_biomes(MapType& map) {
        map.for_each_tile([&map](Tile& tile) {
            BiomeType biome = determine_biome(tile, map);
            apply_biome_effects(tile, biome);
        });
    }

    static BiomeType determine_biome(const Tile& tile, const auto& map) {
        // Convert tile properties to usable values
        float temperature = kelvin_to_celsius(tile_temperature_to_kelvin(tile));
        float precipitation = tile_rainfall_to_mm(tile);
        float elevation = tile_elevation_normalized(tile);
        
        // Calculate derived metrics
        Aridity aridity = calculate_aridity(temperature, precipitation);
        Continentality continentality = calculate_continentality(tile, map);
        
        // Score each biome and select the best match
        float best_score = -1.0F;
        BiomeType best_biome = BiomeType::Desert;
        
#pragma unroll 8
        for (const auto& biome_def : biome_definitions) {
            float score = calculate_biome_suitability(
                biome_def, temperature, precipitation, elevation,
                aridity, continentality, tile, map
            );
            
            if (score > best_score) {
                best_score = score;
                best_biome = biome_def.biome;
            }
        }
        
        return best_biome;
    }

private:
    static float kelvin_to_celsius(float kelvin) {
        return kelvin - 273.15F;
    }
    
    static float tile_temperature_to_kelvin(const Tile& tile) {
        // Convert uint8_t tile temperature to Kelvin (assuming 0-255 maps to realistic range)
        return (static_cast<float>(tile.temperature()) / 255.0F * 60.0F) + 233.15F; // -40°C to +20°C
    }
    
    static float tile_rainfall_to_mm(const Tile& tile) {
        // Convert uint8_t rainfall to mm/year (0-255 maps to 0-4000mm)
        return static_cast<float>(tile.rainfall()) / 255.0F * 4000.0F;
    }
    
    static float tile_elevation_normalized(const Tile& tile) {
        // Normalized elevation 0.0-1.0
        return static_cast<float>(tile.elevation()) / 255.0F;
    }

    static Aridity calculate_aridity(float temperature_celsius, float precipitation_mm) {
        // Modified Thornthwaite aridity index
        float pet = calculate_potential_evapotranspiration(temperature_celsius);
        float aridity_index = precipitation_mm / std::max(pet, 1.0F);
        return Aridity{std::clamp(aridity_index, 0.0F, 3.0F)};
    }
    
    static float calculate_potential_evapotranspiration(float temperature_celsius) {
        // Simplified PET calculation based on temperature
        if (temperature_celsius < 0.0F) {
            return 0.0F;
        }
        return std::pow(temperature_celsius / 5.0F, 1.514F) * 16.0F;
    }
    
    static Continentality calculate_continentality(const Tile& tile, const auto& map) {
        // Distance to nearest water body affects continental climate characteristics
        float min_water_distance = find_nearest_water_distance(tile, map, 20);
        float continentality_factor = std::clamp(min_water_distance / 20.0F, 0.0F, 1.0F);
        return Continentality{continentality_factor};
    }
    
    static float find_nearest_water_distance(const Tile& tile, const auto& map, std::int32_t max_radius) {
        Core::Types::HexCoordinate center = tile.coordinate();
        
#pragma unroll 4
        for (std::int32_t radius = 1; radius <= max_radius; ++radius) {
            std::vector<Core::Types::TileId> ring_tiles = map.get_tiles_in_radius(center, radius);
            
#pragma unroll 4
            for (Core::Types::TileId tile_id : ring_tiles) {
                const Tile* check_tile = map.get_tile(tile_id);
                if (check_tile != nullptr && check_tile->is_water()) {
                    return static_cast<float>(radius);
                }
            }
        }
        
        return static_cast<float>(max_radius);
    }

    static float calculate_biome_suitability(
        const BiomeData& biome_def,
        float temperature,
        float precipitation, 
        float elevation,
        Aridity aridity,
        Continentality continentality,
        const Tile& tile,
        const auto& map
    ) {
        float score = 1.0F;
        
        // Temperature suitability
        if (temperature < biome_def.temperature_range_min || 
            temperature > biome_def.temperature_range_max) {
            float temp_penalty = std::min(
                std::abs(temperature - biome_def.temperature_range_min),
                std::abs(temperature - biome_def.temperature_range_max)
            ) * 0.05F;
            score -= temp_penalty;
        }
        
        // Precipitation suitability
        if (precipitation < biome_def.precipitation_range_min || 
            precipitation > biome_def.precipitation_range_max) {
            float precip_penalty = std::min(
                std::abs(precipitation - biome_def.precipitation_range_min),
                std::abs(precipitation - biome_def.precipitation_range_max)
            ) * 0.0005F;
            score -= precip_penalty;
        }
        
        // Elevation preference
        float elevation_diff = std::abs(elevation - biome_def.elevation_preference);
        score -= elevation_diff * 0.2F;
        
        // Water proximity requirement
        if (biome_def.requires_water_nearby) {
            float water_distance = find_nearest_water_distance(tile, map, 5);
            if (water_distance > 3.0F) {
                score -= 0.5F;
            }
        }
        
        // Aridity considerations for desert biomes
        if (biome_def.biome == BiomeType::Desert && aridity.value() > 0.5F) {
            score -= 0.3F;
        }
        
        // Continentality effects
        float continental_modifier = calculate_continental_modifier(biome_def.biome, continentality);
        score += continental_modifier;
        
        return std::max(score, 0.0F);
    }
    
    static float calculate_continental_modifier(BiomeType biome, Continentality continentality) {
        switch (biome) {
            case BiomeType::TemperateGrassland:
            case BiomeType::BorealForest:
                return continentality.value() * 0.2F; // Continental biomes benefit from continentality
                
            case BiomeType::TemperateRainforest:
            case BiomeType::MediterraneanScrub:
                return (1.0F - continentality.value()) * 0.2F; // Maritime biomes benefit from ocean proximity
                
            default:
                return 0.0F;
        }
    }

    static void apply_biome_effects(Tile& tile, BiomeType biome) {
        // Update terrain type based on biome
        TerrainType terrain = biome_to_terrain(biome);
        tile.set_terrain(terrain);
        
        // Adjust fertility based on biome characteristics
        std::uint8_t fertility = calculate_biome_fertility(biome);
        tile.set_fertility(fertility);
        
        // Add biome-specific features
        apply_biome_features(tile, biome);
    }
    
    static TerrainType biome_to_terrain(BiomeType biome) {
        switch (biome) {
            case BiomeType::IceCap:
                return TerrainType::Snow;
            case BiomeType::Tundra:
            case BiomeType::ParklandTundra:
                return TerrainType::Tundra;
            case BiomeType::BorealForest:
            case BiomeType::TemperateRainforest:
            case BiomeType::TemperateDeciduousForest:
            case BiomeType::MontaneForest:
                return TerrainType::Forest;
            case BiomeType::TropicalRainforest:
                return TerrainType::Jungle;
            case BiomeType::TropicalDryForest:
                return TerrainType::Forest; // Dry forests are different from rainforests
            case BiomeType::TemperateGrassland:
            case BiomeType::TropicalGrassland:
            case BiomeType::Savanna:
                return TerrainType::Grassland;
            case BiomeType::Desert:
            case BiomeType::SemiDesert:
                return TerrainType::Desert;
            case BiomeType::Alpine:
                return TerrainType::Mountains;
            case BiomeType::Wetlands:
                return TerrainType::Marsh;
            case BiomeType::MediterraneanScrub:
            case BiomeType::WoodlandSavanna:
            default:
                return TerrainType::Plains;
        }
    }
    
    static std::uint8_t calculate_biome_fertility(BiomeType biome) {
        switch (biome) {
            case BiomeType::TropicalRainforest:
                return 220;
            case BiomeType::TemperateRainforest:
            case BiomeType::Wetlands:
                return 200;
            case BiomeType::TemperateGrassland:
            case BiomeType::TropicalGrassland:
                return 180;
            case BiomeType::TemperateDeciduousForest:
                return 160;
            case BiomeType::Savanna:
                return 170; // Savanna is more fertile than temperate forest
            case BiomeType::BorealForest:
            case BiomeType::TropicalDryForest:
                return 120;
            case BiomeType::MediterraneanScrub:
            case BiomeType::WoodlandSavanna:
                return 100;
            case BiomeType::SemiDesert:
            case BiomeType::MontaneForest:
                return 80;
            case BiomeType::Tundra:
            case BiomeType::ParklandTundra:
                return 60;
            case BiomeType::Alpine:
                return 40;
            case BiomeType::Desert:
                return 20;
            case BiomeType::IceCap:
            default:
                return 10;
        }
    }
    
    static void apply_biome_features(Tile& tile, BiomeType biome) {
        using Tiles::Feature;
        
        switch (biome) {
            case BiomeType::TropicalRainforest:
            case BiomeType::TemperateRainforest:
                tile.add_feature(Feature::Forest);
                break;
                
            case BiomeType::Wetlands:
                tile.add_feature(Feature::Marsh);
                break;
                
            case BiomeType::IceCap:
            case BiomeType::Alpine:
                tile.add_feature(Feature::Ice);
                break;
                
            case BiomeType::Desert:
            case BiomeType::Savanna:
            default:
                // No special features for these biomes currently
                // Future: could add oases for deserts, special vegetation for savannas
                break;
        }
    }
};

} // namespace Manifest::World::Generation

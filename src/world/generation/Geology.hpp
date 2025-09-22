#pragma once

#include <algorithm>
#include <array>

#include "../../core/math/Vector.hpp"
#include "../../core/types/Types.hpp"

namespace Manifest::World::Generation {

using Core::Math::Vec2f;
using Core::Types::HexCoordinate;
using Core::Types::Quantity;

// Strong types for geological classification
struct GeologicalAgeTag {};
struct MineralDensityTag {};
struct MetamorphicGradeTag {};
struct HydrothermalActivityTag {};

using GeologicalAge = Quantity<GeologicalAgeTag, float>;
using MineralDensity = Quantity<MineralDensityTag, float>;
using MetamorphicGrade = Quantity<MetamorphicGradeTag, float>;
using HydrothermalActivity = Quantity<HydrothermalActivityTag, float>;

// Simplified resource types for geological classification
enum class Resource : std::uint8_t {
    None, Iron, Coal, Oil, Gold, Wheat, Cattle, Fish, Stone, Luxury, Strategic
};

// Simplified terrain types
enum class Terrain : std::uint8_t {
    Ocean, Coast, Plains, Grassland, Hills, Mountains, Desert, Tundra, Snow
};

// Plate boundary types for geological classification
enum class Boundary : std::uint8_t { Convergent, Divergent, Transform };

// Geological formations based on tectonic processes
enum class Formation : std::uint8_t {
    // Igneous formations
    Granite,           // Continental intrusive, rare metals
    Basalt,            // Oceanic extrusive, common metals
    Volcanic,          // Active volcanism, sulfur/minerals  
    Plutonic,          // Deep intrusive, precious metals
    
    // Sedimentary formations
    Limestone,         // Marine sediments, building stone
    Sandstone,         // Terrestrial sediments, oil/gas
    Shale,             // Fine sediments, oil/gas/rare earths
    Conglomerate,      // Coarse sediments, placer deposits
    
    // Metamorphic formations
    Schist,            // Medium grade, various minerals
    Gneiss,            // High grade, rare metals
    Marble,            // Metamorphosed limestone, stone
    Quartzite,         // Metamorphosed sandstone, silicon
    
    // Special formations
    Hydrothermal,      // Hot springs/geysers, precious metals
    Kimberlite,        // Diamond pipes, diamonds
    Pegmatite,         // Large crystals, rare minerals
    Alluvial           // River deposits, placer gold
};

struct alignas(64) FormationData {
    Formation formation{};
    GeologicalAge age_preference{0.0F};
    MineralDensity base_density{0.0F};
    MetamorphicGrade metamorphic_level{0.0F};
    HydrothermalActivity thermal_activity{0.0F};
    std::array<Resource, 4> primary_resources{};
    std::array<float, 4> resource_probabilities{};
    bool requires_plate_boundary{false};
    bool coastal_preference{false};
};

class Geology {
    static constexpr std::array<FormationData, 16> formation_definitions = {{
        // Igneous formations (high-temperature crystallization)
        {Formation::Granite, GeologicalAge{100.0F}, MineralDensity{0.3F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.2F},
         {{Resource::Stone, Resource::Gold, Resource::Strategic, Resource::None}}, 
         {{0.8F, 0.15F, 0.05F, 0.0F}}, false, false},
        
        {Formation::Basalt, GeologicalAge{50.0F}, MineralDensity{0.4F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.1F},
         {{Resource::Iron, Resource::Stone, Resource::Coal, Resource::None}},
         {{0.6F, 0.7F, 0.2F, 0.0F}}, false, true},
         
        {Formation::Volcanic, GeologicalAge{5.0F}, MineralDensity{0.6F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.9F},
         {{Resource::Strategic, Resource::Gold, Resource::Iron, Resource::Stone}},
         {{0.3F, 0.2F, 0.4F, 0.5F}}, true, false},
         
        {Formation::Plutonic, GeologicalAge{150.0F}, MineralDensity{0.2F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.3F},
         {{Resource::Gold, Resource::Strategic, Resource::Stone, Resource::None}},
         {{0.25F, 0.15F, 0.6F, 0.0F}}, false, false},
        
        // Sedimentary formations (surface processes)
        {Formation::Limestone, GeologicalAge{75.0F}, MineralDensity{0.1F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.0F},
         {{Resource::Stone, Resource::None, Resource::None, Resource::None}},
         {{0.9F, 0.0F, 0.0F, 0.0F}}, false, true},
         
        {Formation::Sandstone, GeologicalAge{60.0F}, MineralDensity{0.2F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.0F},
         {{Resource::Oil, Resource::Stone, Resource::None, Resource::None}},
         {{0.4F, 0.6F, 0.0F, 0.0F}}, false, false},
         
        {Formation::Shale, GeologicalAge{80.0F}, MineralDensity{0.3F}, MetamorphicGrade{0.1F}, HydrothermalActivity{0.0F},
         {{Resource::Oil, Resource::Coal, Resource::Strategic, Resource::None}},
         {{0.5F, 0.4F, 0.1F, 0.0F}}, false, false},
         
        {Formation::Conglomerate, GeologicalAge{40.0F}, MineralDensity{0.4F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.0F},
         {{Resource::Gold, Resource::Stone, Resource::Iron, Resource::None}},
         {{0.3F, 0.5F, 0.2F, 0.0F}}, false, false},
        
        // Metamorphic formations (pressure/temperature altered)
        {Formation::Schist, GeologicalAge{120.0F}, MineralDensity{0.5F}, MetamorphicGrade{0.6F}, HydrothermalActivity{0.2F},
         {{Resource::Iron, Resource::Strategic, Resource::Stone, Resource::None}},
         {{0.4F, 0.2F, 0.4F, 0.0F}}, false, false},
         
        {Formation::Gneiss, GeologicalAge{200.0F}, MineralDensity{0.3F}, MetamorphicGrade{0.9F}, HydrothermalActivity{0.1F},
         {{Resource::Strategic, Resource::Gold, Resource::Stone, Resource::None}},
         {{0.2F, 0.3F, 0.5F, 0.0F}}, false, false},
         
        {Formation::Marble, GeologicalAge{100.0F}, MineralDensity{0.1F}, MetamorphicGrade{0.7F}, HydrothermalActivity{0.0F},
         {{Resource::Stone, Resource::Luxury, Resource::None, Resource::None}},
         {{0.8F, 0.2F, 0.0F, 0.0F}}, false, false},
         
        {Formation::Quartzite, GeologicalAge{90.0F}, MineralDensity{0.2F}, MetamorphicGrade{0.5F}, HydrothermalActivity{0.0F},
         {{Resource::Stone, Resource::Strategic, Resource::None, Resource::None}},
         {{0.7F, 0.3F, 0.0F, 0.0F}}, false, false},
        
        // Special formations (unique geological processes)
        {Formation::Hydrothermal, GeologicalAge{10.0F}, MineralDensity{0.8F}, MetamorphicGrade{0.3F}, HydrothermalActivity{1.0F},
         {{Resource::Gold, Resource::Strategic, Resource::Luxury, Resource::None}},
         {{0.4F, 0.3F, 0.3F, 0.0F}}, true, false},
         
        {Formation::Kimberlite, GeologicalAge{1000.0F}, MineralDensity{0.1F}, MetamorphicGrade{1.0F}, HydrothermalActivity{0.0F},
         {{Resource::Luxury, Resource::Strategic, Resource::None, Resource::None}},
         {{0.8F, 0.2F, 0.0F, 0.0F}}, false, false},
         
        {Formation::Pegmatite, GeologicalAge{150.0F}, MineralDensity{0.9F}, MetamorphicGrade{0.2F}, HydrothermalActivity{0.4F},
         {{Resource::Strategic, Resource::Gold, Resource::Luxury, Resource::None}},
         {{0.5F, 0.3F, 0.2F, 0.0F}}, false, false},
         
        {Formation::Alluvial, GeologicalAge{1.0F}, MineralDensity{0.6F}, MetamorphicGrade{0.0F}, HydrothermalActivity{0.0F},
         {{Resource::Gold, Resource::Iron, Resource::Stone, Resource::None}},
         {{0.5F, 0.3F, 0.2F, 0.0F}}, false, true}
    }};

public:
    // Core geological classification based on simple inputs
    static constexpr Formation classify_formation(float plate_age,
                                                 bool is_oceanic_plate,
                                                 Boundary boundary_type,
                                                 float elevation_normalized,
                                                 Terrain terrain_type,
                                                 bool is_coastal,
                                                 float noise_value) noexcept {
        
        // Primary classification by tectonic setting
        if (boundary_type == Boundary::Convergent) {
            if (is_oceanic_plate && elevation_normalized > 0.7F) {
                return Formation::Volcanic;
            }
            if (!is_oceanic_plate && elevation_normalized > 0.6F) {
                return noise_value > 0.0F ? Formation::Granite : Formation::Schist;
            }
        }
        
        if (boundary_type == Boundary::Divergent) {
            return is_oceanic_plate ? Formation::Basalt : Formation::Sandstone;
        }
        
        // Age-based classification for stable areas
        if (plate_age > 150.0F) {
            if (elevation_normalized > 0.8F) {
                return Formation::Gneiss;
            }
            if (noise_value > 0.8F) {
                return Formation::Kimberlite; // Rare ancient formations
            }
        }
        
        // Terrain-based classification
        switch (terrain_type) {
            case Terrain::Mountains:
                return plate_age > 100.0F ? Formation::Granite : Formation::Volcanic;
            case Terrain::Hills:
                return Formation::Schist;
            case Terrain::Plains:
                return is_coastal ? Formation::Sandstone : Formation::Shale;
            case Terrain::Coast:
                return Formation::Alluvial;
            case Terrain::Ocean:
                return Formation::Basalt;
            default:
                break;
        }
        
        // Hydrothermal formations near volcanic activity
        if (elevation_normalized > 0.5F && noise_value > 0.7F) {
            return Formation::Hydrothermal;
        }
        
        // Default formations by elevation
        if (elevation_normalized > 0.6F) {
            return Formation::Granite;
        }
        if (elevation_normalized > 0.3F) {
            return Formation::Sandstone;
        }
        return Formation::Shale;
    }
    
    // Calculate resource distribution for a formation
    static constexpr std::array<std::pair<Resource, float>, 4> 
    calculate_resource_distribution(Formation formation,
                                  float geological_age_factor,
                                  float mineral_concentration,
                                  float hydrothermal_factor) noexcept {
        
        const auto& data = get_formation_data(formation);
        std::array<std::pair<Resource, float>, 4> distribution{};
        
        #pragma unroll
        for (std::size_t i = 0; i < 4; ++i) {
            Resource resource = data.primary_resources[i];
            if (resource == Resource::None) {
                distribution[i] = {Resource::None, 0.0F};
                continue;
            }
            
            float base_probability = data.resource_probabilities[i];
            
            // Age factor - older formations more concentrated
            float age_modifier = 1.0F + (geological_age_factor * data.age_preference.value() * 0.01F);
            
            // Mineral density modifier
            float density_modifier = 1.0F + (mineral_concentration * data.base_density.value());
            
            // Hydrothermal activity modifier
            float thermal_modifier = 1.0F + (hydrothermal_factor * data.thermal_activity.value());
            
            float final_probability = base_probability * age_modifier * density_modifier * thermal_modifier;
            distribution[i] = {resource, std::clamp(final_probability, 0.0F, 1.0F)};
        }
        
        return distribution;
    }
    
    // Get formation data
    static constexpr const FormationData& get_formation_data(Formation formation) noexcept {
        const auto* found_formation = std::find_if(formation_definitions.begin(), formation_definitions.end(),
                              [formation](const FormationData& data) { return data.formation == formation; });
        return found_formation != formation_definitions.end() ? *found_formation : formation_definitions[0];
    }
    
    // Simple coordinate-based geological classification
    static constexpr Formation generate_geology([[maybe_unused]] const HexCoordinate& coord,
                                              [[maybe_unused]] float map_radius,
                                              float plate_age,
                                              bool is_oceanic_plate,
                                              Boundary boundary_type,
                                              std::uint8_t elevation_byte,
                                              Terrain terrain_type,
                                              bool is_coastal,
                                              float noise_value) noexcept {
        
        float elevation_normalized = static_cast<float>(elevation_byte) / 255.0F;
        
        return classify_formation(plate_age, is_oceanic_plate, boundary_type, 
                                elevation_normalized, terrain_type, is_coastal, noise_value);
    }
};

} // namespace Manifest::World::Generation
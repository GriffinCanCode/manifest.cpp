#include "Culture.hpp"
#include "Tile.hpp"
#include "Map.hpp"
#include <cmath>

namespace Manifest::World::Tiles {

CulturalInfluence CulturalSpread::calculate_influence(const SpreadSource& source, TileId target) const {
    if (!map_) {
        return {};
    }
    
    float distance = calculate_distance(source.source_tile, target);
    
    // Check if within spread radius
    if (distance > static_cast<float>(source.spread_radius)) {
        return {};
    }
    
    const Tile* target_tile = map_->get_tile(target);
    if (!target_tile) {
        return {};
    }
    
    // Calculate base influence with distance decay
    float distance_factor = std::exp(-source.distance_decay * distance);
    float terrain_modifier = calculate_terrain_modifier(target_tile, source.trait);
    
    float influence_strength = source.base_strength.value() * distance_factor * terrain_modifier;
    
    CulturalInfluence influence;
    influence.source_nation = source.nation;
    influence.trait = source.trait;
    influence.strength = Influence{influence_strength};
    influence.established_turn = 0; // Will be set by caller
    
    return influence;
}

void CulturalSpread::simulate_spread(GameTurn current_turn) {
    if (!map_) {
        return;
    }
    
    for (const auto& source : sources_) {
        // Get all tiles within spread radius
        auto affected_tiles = get_tiles_in_radius(source.source_tile, source.spread_radius);
        
        for (TileId tile_id : affected_tiles) {
            if (tile_id == source.source_tile) {
                continue; // Skip source tile
            }
            
            auto influence = calculate_influence(source, tile_id);
            if (influence.strength.value() > 0.1) { // Only significant influences
                influence.established_turn = current_turn;
                
                // Apply influence to tile (this would integrate with CulturalSystem)
                // For now, this is a placeholder
                (void)influence; // Prevent unused warning
            }
        }
    }
}

float CulturalSpread::calculate_distance(TileId from, TileId to) const {
    if (!map_) {
        return std::numeric_limits<float>::infinity();
    }
    
    const Tile* from_tile = map_->get_tile(from);
    const Tile* to_tile = map_->get_tile(to);
    
    if (!from_tile || !to_tile) {
        return std::numeric_limits<float>::infinity();
    }
    
    const auto& from_coord = from_tile->coordinate();
    const auto& to_coord = to_tile->coordinate();
    
    // Hex grid distance
    return static_cast<float>((std::abs(from_coord.q - to_coord.q) + 
                              std::abs(from_coord.q + from_coord.r - to_coord.q - to_coord.r) + 
                              std::abs(from_coord.r - to_coord.r)) / 2);
}

std::vector<TileId> CulturalSpread::get_tiles_in_radius(TileId center, std::uint32_t radius) const {
    std::vector<TileId> tiles;
    
    if (!map_) {
        return tiles;
    }
    
    const Tile* center_tile = map_->get_tile(center);
    if (!center_tile) {
        return tiles;
    }
    
    const auto& center_coord = center_tile->coordinate();
    
    // Iterate through hex grid in radius
    for (std::int32_t q = -static_cast<std::int32_t>(radius); 
         q <= static_cast<std::int32_t>(radius); ++q) {
        
        std::int32_t r1 = std::max(-static_cast<std::int32_t>(radius), 
                                   -q - static_cast<std::int32_t>(radius));
        std::int32_t r2 = std::min(static_cast<std::int32_t>(radius), 
                                   -q + static_cast<std::int32_t>(radius));
        
        for (std::int32_t r = r1; r <= r2; ++r) {
            std::int32_t s = -q - r;
            
            HexCoordinate coord{center_coord.q + q, center_coord.r + r, center_coord.s + s};
            
            // Find tile at this coordinate (simplified - would use proper map lookup)
            // This is a placeholder implementation
            if (std::abs(q) + std::abs(r) + std::abs(s) <= static_cast<std::int32_t>(radius * 2)) {
                // Would get actual tile ID from map coordinate lookup
                // tiles.push_back(map->get_tile_at_coordinate(coord));
            }
        }
    }
    
    return tiles;
}

float CulturalSpread::calculate_terrain_modifier(const Tile* tile, CulturalTrait trait) const {
    if (!tile) {
        return 0.0F;
    }
    
    float modifier = 1.0F;
    
    switch (trait) {
        case CulturalTrait::Religious:
            // Mountains and hills boost religious spread (monasteries, isolation)
            if (tile->terrain() == TerrainType::Mountains || 
                tile->terrain() == TerrainType::Hills) {
                modifier *= 1.2F;
            }
            break;
            
        case CulturalTrait::Commercial:
            // Coastal and plains boost commercial culture
            if (tile->terrain() == TerrainType::Coast || 
                tile->terrain() == TerrainType::Plains) {
                modifier *= 1.3F;
            }
            // Rivers boost trade
            if (tile->has_feature(Feature::River)) {
                modifier *= 1.2F;
            }
            break;
            
        case CulturalTrait::Agricultural:
            // Fertile lands boost agricultural culture
            if (tile->terrain() == TerrainType::Grassland || 
                tile->terrain() == TerrainType::Plains) {
                modifier *= 1.4F;
            }
            // Deserts reduce agricultural spread
            if (tile->terrain() == TerrainType::Desert) {
                modifier *= 0.5F;
            }
            break;
            
        case CulturalTrait::Maritime:
            // Water tiles boost maritime culture
            if (tile->is_water()) {
                modifier *= 2.0F;
            }
            // Inland tiles reduce maritime culture
            if (tile->terrain() == TerrainType::Mountains || 
                tile->terrain() == TerrainType::Desert) {
                modifier *= 0.3F;
            }
            break;
            
        case CulturalTrait::Scientific:
            // Cities and developed areas boost scientific culture
            // This would check for improvements like universities, libraries
            if (tile->improvement() != ImprovementType::None) {
                modifier *= 1.1F;
            }
            break;
            
        case CulturalTrait::Military:
            // Strategic terrain boosts military culture
            if (tile->terrain() == TerrainType::Hills || 
                tile->terrain() == TerrainType::Mountains) {
                modifier *= 1.2F;
            }
            if (tile->improvement() == ImprovementType::Fort) {
                modifier *= 1.5F;
            }
            break;
            
        case CulturalTrait::Industrial:
            // Resources and improvements boost industrial culture
            if (tile->resource() == ResourceType::Iron || 
                tile->resource() == ResourceType::Coal) {
                modifier *= 1.3F;
            }
            if (tile->improvement() == ImprovementType::Mine) {
                modifier *= 1.2F;
            }
            break;
            
        case CulturalTrait::Artistic:
            // Natural wonders and beautiful terrain boost artistic culture
            if (tile->has_feature(Feature::NaturalWonder)) {
                modifier *= 1.5F;
            }
            if (tile->terrain() == TerrainType::Forest || 
                tile->terrain() == TerrainType::Coast) {
                modifier *= 1.1F;
            }
            break;
    }
    
    // Roads help all cultural spread
    if (tile->improvement() == ImprovementType::Road) {
        modifier *= 1.1F;
    } else if (tile->improvement() == ImprovementType::Railroad) {
        modifier *= 1.2F;
    }
    
    // Difficult terrain generally slows spread
    if (tile->terrain() == TerrainType::Marsh || 
        tile->terrain() == TerrainType::Jungle) {
        modifier *= 0.8F;
    }
    
    return modifier;
}

} // namespace Manifest::World::Tiles

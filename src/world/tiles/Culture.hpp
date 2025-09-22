#pragma once

#include <unordered_map>
#include <vector>

#include "../../core/types/Types.hpp"

namespace Manifest::World::Tiles {

using Core::Types::TileId;
using Core::Types::NationId;
using Core::Types::Influence;
using Core::Types::HexCoordinate;
using Core::Types::GameTurn;

// Forward declarations
class Tile;
class TileMap;

// Cultural trait representing aspects of civilization
enum class CulturalTrait : std::uint8_t {
    Religious,      // Religious influence
    Artistic,       // Cultural/artistic influence
    Scientific,     // Academic/scientific influence
    Commercial,     // Trade/economic influence
    Military,       // Military/martial influence
    Agricultural,   // Farming/rural influence
    Industrial,     // Manufacturing influence
    Maritime        // Seafaring influence
};

// Cultural influence data for a specific nation and trait
struct CulturalInfluence {
    NationId source_nation{NationId::invalid()};
    CulturalTrait trait{CulturalTrait::Religious};
    Influence strength{0.0};
    GameTurn established_turn{0};
    float decay_rate{0.05F}; // Per turn decay
    
    // Update influence over time
    void decay(GameTurn current_turn) noexcept {
        GameTurn age = current_turn - established_turn;
        float decay_factor = std::exp(-decay_rate * static_cast<float>(age));
        strength = Influence{strength.value() * decay_factor};
    }
    
    constexpr bool is_significant() const noexcept {
        return strength.value() > 1.0;
    }
};

// Cultural profile of a tile
class TileCulture {
    std::vector<CulturalInfluence> influences_;
    NationId dominant_culture_{NationId::invalid()};
    
    // Cached totals for performance
    mutable std::unordered_map<NationId, Influence> nation_totals_;
    mutable bool cache_valid_{false};
    
public:
    // Add or update cultural influence
    void add_influence(const CulturalInfluence& influence) {
        auto it = std::find_if(influences_.begin(), influences_.end(),
            [&](const CulturalInfluence& existing) {
                return existing.source_nation == influence.source_nation &&
                       existing.trait == influence.trait;
            });
        
        if (it != influences_.end()) {
            it->strength += influence.strength;
            it->established_turn = influence.established_turn; // Update timestamp
        } else {
            influences_.push_back(influence);
        }
        
        invalidate_cache();
        update_dominant_culture();
    }
    
    // Remove influence
    bool remove_influence(NationId nation, CulturalTrait trait) {
        auto it = std::remove_if(influences_.begin(), influences_.end(),
            [&](const CulturalInfluence& inf) {
                return inf.source_nation == nation && inf.trait == trait;
            });
        
        if (it != influences_.end()) {
            influences_.erase(it, influences_.end());
            invalidate_cache();
            update_dominant_culture();
            return true;
        }
        
        return false;
    }
    
    // Query influences
    const std::vector<CulturalInfluence>& influences() const noexcept {
        return influences_;
    }
    
    std::vector<CulturalInfluence> influences_by_nation(NationId nation) const {
        std::vector<CulturalInfluence> result;
        std::copy_if(influences_.begin(), influences_.end(), std::back_inserter(result),
            [nation](const CulturalInfluence& inf) {
                return inf.source_nation == nation;
            });
        return result;
    }
    
    Influence total_influence(NationId nation) const {
        update_cache_if_needed();
        auto it = nation_totals_.find(nation);
        return it != nation_totals_.end() ? it->second : Influence{0.0};
    }
    
    Influence trait_influence(CulturalTrait trait) const {
        Influence total{0.0};
        for (const auto& inf : influences_) {
            if (inf.trait == trait) {
                total += inf.strength;
            }
        }
        return total;
    }
    
    // Dominant culture
    NationId dominant_culture() const noexcept {
        return dominant_culture_;
    }
    
    bool is_culturally_contested() const noexcept {
        update_cache_if_needed();
        
        if (nation_totals_.size() < 2) {
            return false;
        }
        
        // Find top two influences
        std::vector<std::pair<NationId, Influence>> sorted_influences;
        for (const auto& [nation, influence] : nation_totals_) {
            sorted_influences.emplace_back(nation, influence);
        }
        
        std::sort(sorted_influences.begin(), sorted_influences.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
        
        if (sorted_influences.size() >= 2) {
            double ratio = sorted_influences[0].second.value() / 
                          sorted_influences[1].second.value();
            return ratio < 2.0; // Less than 2:1 ratio is considered contested
        }
        
        return false;
    }
    
    // Update over time
    void update(GameTurn current_turn) {
        bool changed = false;
        
        // Apply decay to all influences
        for (auto& influence : influences_) {
            Influence old_strength = influence.strength;
            influence.decay(current_turn);
            
            if (old_strength != influence.strength) {
                changed = true;
            }
        }
        
        // Remove insignificant influences
        auto it = std::remove_if(influences_.begin(), influences_.end(),
            [](const CulturalInfluence& inf) {
                return !inf.is_significant();
            });
        
        if (it != influences_.end()) {
            influences_.erase(it, influences_.end());
            changed = true;
        }
        
        if (changed) {
            invalidate_cache();
            update_dominant_culture();
        }
    }
    
private:
    void invalidate_cache() const noexcept {
        cache_valid_ = false;
        nation_totals_.clear();
    }
    
    void update_cache_if_needed() const {
        if (!cache_valid_) {
            for (const auto& influence : influences_) {
                nation_totals_[influence.source_nation] += influence.strength;
            }
            cache_valid_ = true;
        }
    }
    
    void update_dominant_culture() {
        update_cache_if_needed();
        
        if (nation_totals_.empty()) {
            dominant_culture_ = NationId::invalid();
            return;
        }
        
        auto max_it = std::max_element(nation_totals_.begin(), nation_totals_.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
        
        dominant_culture_ = max_it->first;
    }
};

// Cultural spread simulation
class CulturalSpread {
    struct SpreadSource {
        TileId source_tile{TileId::invalid()};
        NationId nation{NationId::invalid()};
        CulturalTrait trait{CulturalTrait::Religious};
        Influence base_strength{0.0};
        std::uint32_t spread_radius{5};
        float distance_decay{0.2F};
    };
    
    std::vector<SpreadSource> sources_;
    const TileMap* map_;
    
public:
    explicit CulturalSpread(const TileMap* tilemap) noexcept : map_{tilemap} {}
    
    // Add cultural source (city, wonder, etc.)
    void add_source(TileId tile_id, NationId nation, CulturalTrait trait,
                   Influence strength, std::uint32_t radius = 5) {
        SpreadSource source;
        source.source_tile = tile_id;
        source.nation = nation;
        source.trait = trait;
        source.base_strength = strength;
        source.spread_radius = radius;
        
        sources_.push_back(source);
    }
    
    void remove_source(TileId tile_id, NationId nation, CulturalTrait trait) {
        auto it = std::remove_if(sources_.begin(), sources_.end(),
            [&](const SpreadSource& source) {
                return source.source_tile == tile_id &&
                       source.nation == nation &&
                       source.trait == trait;
            });
        
        sources_.erase(it, sources_.end());
    }
    
    // Calculate influence spread to target tile
    CulturalInfluence calculate_influence(const SpreadSource& source, TileId target) const;
    
    // Simulate spread for one turn
    void simulate_spread(GameTurn current_turn);
    
    // Get all sources
    const std::vector<SpreadSource>& sources() const noexcept { return sources_; }
    
private:
    float calculate_distance(TileId from, TileId to) const;
    
    std::vector<TileId> get_tiles_in_radius(TileId center, std::uint32_t radius) const;
    
    float calculate_terrain_modifier(const Tile* tile, CulturalTrait trait) const;
};

// Global cultural system managing all cultural interactions
class CulturalSystem {
    std::unordered_map<TileId, TileCulture> tile_cultures_;
    CulturalSpread spread_system_;
    GameTurn current_turn_{0};
    
public:
    explicit CulturalSystem(const TileMap* tilemap) : spread_system_{tilemap} {}
    
    // Tile culture management
    TileCulture* get_tile_culture(TileId tile_id) {
        auto it = tile_cultures_.find(tile_id);
        return it != tile_cultures_.end() ? &it->second : nullptr;
    }
    
    const TileCulture* get_tile_culture(TileId tile_id) const {
        auto it = tile_cultures_.find(tile_id);
        return it != tile_cultures_.end() ? &it->second : nullptr;
    }
    
    void ensure_tile_culture(TileId tile_id) {
        if (tile_cultures_.find(tile_id) == tile_cultures_.end()) {
            tile_cultures_[tile_id] = TileCulture{};
        }
    }
    
    // Cultural source management
    void add_cultural_source(TileId tile_id, NationId nation, CulturalTrait trait,
                           Influence strength, std::uint32_t radius = 5) {
        spread_system_.add_source(tile_id, nation, trait, strength, radius);
    }
    
    void remove_cultural_source(TileId tile_id, NationId nation, CulturalTrait trait) {
        spread_system_.remove_source(tile_id, nation, trait);
    }
    
    // Update system
    void update(GameTurn turn) {
        current_turn_ = turn;
        
        // Update all tile cultures (decay)
        for (auto& [tile_id, culture] : tile_cultures_) {
            culture.update(current_turn_);
        }
        
        // Simulate cultural spread
        spread_system_.simulate_spread(current_turn_);
        
        // Clean up empty cultures
        auto it = std::remove_if(tile_cultures_.begin(), tile_cultures_.end(),
            [](const auto& pair) {
                return pair.second.influences().empty();
            });
        
        if (it != tile_cultures_.end()) {
            tile_cultures_.erase(it, tile_cultures_.end());
        }
    }
    
    // Analysis
    std::vector<TileId> get_culturally_influenced_tiles(NationId nation) const {
        std::vector<TileId> tiles;
        
        for (const auto& [tile_id, culture] : tile_cultures_) {
            if (culture.total_influence(nation).value() > 0.0) {
                tiles.push_back(tile_id);
            }
        }
        
        return tiles;
    }
    
    std::vector<TileId> get_contested_tiles() const {
        std::vector<TileId> tiles;
        
        for (const auto& [tile_id, culture] : tile_cultures_) {
            if (culture.is_culturally_contested()) {
                tiles.push_back(tile_id);
            }
        }
        
        return tiles;
    }
    
    GameTurn current_turn() const noexcept { return current_turn_; }
    CulturalSpread& spread_system() noexcept { return spread_system_; }
};

} // namespace Manifest::World::Tiles

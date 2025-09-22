#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../core/types/Types.hpp"

namespace Manifest::World::Tiles {

using namespace Core::Types;

// Ownership claim with strength and type
struct OwnershipClaim {
    NationId claimant{NationId::invalid()};
    Influence strength{0.0};
    GameTurn established_turn{0};
    
    enum class Type : std::uint8_t {
        Legal,      // Formal territorial claim
        Cultural,   // Cultural influence
        Economic,   // Trade/resource control
        Military    // Military occupation
    } type{Type::Legal};
    
    constexpr bool operator==(const OwnershipClaim& other) const noexcept {
        return claimant == other.claimant && 
               strength == other.strength &&
               type == other.type;
    }
    
    constexpr bool is_valid() const noexcept {
        return claimant.is_valid() && strength.value() > 0.0;
    }
};

// Multi-claimant ownership system
class TileOwnership {
    std::vector<OwnershipClaim> claims_;
    NationId primary_owner_{NationId::invalid()};
    
    // Cached values for performance
    mutable Influence total_influence_{0.0};
    mutable bool cache_valid_{false};
    
public:
    // Add or update claim
    void add_claim(const OwnershipClaim& claim) {
        auto it = std::find_if(claims_.begin(), claims_.end(),
            [&](const OwnershipClaim& existing) {
                return existing.claimant == claim.claimant && 
                       existing.type == claim.type;
            });
        
        if (it != claims_.end()) {
            *it = claim;
        } else {
            claims_.push_back(claim);
        }
        
        invalidate_cache();
        update_primary_owner();
    }
    
    // Remove claim
    bool remove_claim(NationId claimant, OwnershipClaim::Type type) {
        auto it = std::remove_if(claims_.begin(), claims_.end(),
            [&](const OwnershipClaim& claim) {
                return claim.claimant == claimant && claim.type == type;
            });
        
        if (it != claims_.end()) {
            claims_.erase(it, claims_.end());
            invalidate_cache();
            update_primary_owner();
            return true;
        }
        
        return false;
    }
    
    // Query claims
    const std::vector<OwnershipClaim>& claims() const noexcept { 
        return claims_; 
    }
    
    std::vector<OwnershipClaim> claims_by_nation(NationId nation) const {
        std::vector<OwnershipClaim> result;
        std::copy_if(claims_.begin(), claims_.end(), std::back_inserter(result),
            [nation](const OwnershipClaim& claim) {
                return claim.claimant == nation;
            });
        return result;
    }
    
    Influence claim_strength(NationId nation) const noexcept {
        Influence total{0.0};
        for (const auto& claim : claims_) {
            if (claim.claimant == nation) {
                total += claim.strength;
            }
        }
        return total;
    }
    
    // Primary ownership
    NationId primary_owner() const noexcept { 
        return primary_owner_; 
    }
    
    bool is_contested() const noexcept {
        return claims_.size() > 1;
    }
    
    bool has_claim(NationId nation) const noexcept {
        return std::any_of(claims_.begin(), claims_.end(),
            [nation](const OwnershipClaim& claim) {
                return claim.claimant == nation;
            });
    }
    
    // Influence calculations
    Influence total_influence() const noexcept {
        if (!cache_valid_) {
            total_influence_ = Influence{0.0};
            for (const auto& claim : claims_) {
                total_influence_ += claim.strength;
            }
            cache_valid_ = true;
        }
        return total_influence_;
    }
    
    float influence_ratio(NationId nation) const noexcept {
        auto total = total_influence();
        if (total.value() <= 0.0) return 0.0f;
        
        auto nation_influence = claim_strength(nation);
        return static_cast<float>(nation_influence.value() / total.value());
    }
    
private:
    void invalidate_cache() const noexcept {
        cache_valid_ = false;
    }
    
    void update_primary_owner() {
        if (claims_.empty()) {
            primary_owner_ = NationId::invalid();
            return;
        }
        
        // Find nation with highest total claim strength
        std::unordered_map<NationId, Influence> nation_totals;
        for (const auto& claim : claims_) {
            nation_totals[claim.claimant] += claim.strength;
        }
        
        auto max_it = std::max_element(nation_totals.begin(), nation_totals.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
        
        primary_owner_ = max_it != nation_totals.end() ? max_it->first : NationId::invalid();
    }
};

// Influence zones and cultural spread
class InfluenceZone {
    HexCoordinate center_;
    std::uint32_t radius_;
    NationId source_nation_;
    Influence base_strength_;
    float decay_rate_{0.1f};
    
public:
    InfluenceZone(const HexCoordinate& center, std::uint32_t radius, 
                  NationId nation, Influence strength) noexcept
        : center_{center}, radius_{radius}, source_nation_{nation}, base_strength_{strength} {}
    
    // Calculate influence at given coordinate
    Influence influence_at(const HexCoordinate& coord) const noexcept {
        float distance = hex_distance(center_, coord);
        if (distance > radius_) return Influence{0.0};
        
        // Exponential decay with distance
        float decay_factor = std::exp(-decay_rate_ * distance);
        return Influence{base_strength_.value() * decay_factor};
    }
    
    bool affects_tile(const HexCoordinate& coord) const noexcept {
        return hex_distance(center_, coord) <= radius_;
    }
    
    // Accessors
    const HexCoordinate& center() const noexcept { return center_; }
    std::uint32_t radius() const noexcept { return radius_; }
    NationId source_nation() const noexcept { return source_nation_; }
    Influence base_strength() const noexcept { return base_strength_; }
    
    // Modify zone
    void set_strength(Influence strength) noexcept { base_strength_ = strength; }
    void set_decay_rate(float rate) noexcept { decay_rate_ = rate; }
    
private:
    static float hex_distance(const HexCoordinate& a, const HexCoordinate& b) noexcept {
        return static_cast<float>((std::abs(a.q - b.q) + 
                                  std::abs(a.q + a.r - b.q - b.r) + 
                                  std::abs(a.r - b.r)) / 2);
    }
};

// Global influence system managing all zones
class InfluenceSystem {
    std::vector<InfluenceZone> zones_;
    std::unordered_set<TileId> affected_tiles_;
    
public:
    void add_zone(const InfluenceZone& zone) {
        zones_.push_back(zone);
        // Would update affected_tiles_ in full implementation
    }
    
    void remove_zones_by_nation(NationId nation) {
        auto it = std::remove_if(zones_.begin(), zones_.end(),
            [nation](const InfluenceZone& zone) {
                return zone.source_nation() == nation;
            });
        zones_.erase(it, zones_.end());
    }
    
    // Calculate total influence at coordinate
    std::vector<std::pair<NationId, Influence>> influences_at(
        const HexCoordinate& coord) const {
        
        std::unordered_map<NationId, Influence> nation_influences;
        
        for (const auto& zone : zones_) {
            auto influence = zone.influence_at(coord);
            if (influence.value() > 0.0) {
                nation_influences[zone.source_nation()] += influence;
            }
        }
        
        std::vector<std::pair<NationId, Influence>> result;
        result.reserve(nation_influences.size());
        
        for (const auto& [nation, influence] : nation_influences) {
            result.emplace_back(nation, influence);
        }
        
        // Sort by influence strength
        std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });
        
        return result;
    }
    
    // Update all zones (called each turn)
    void update_influences() {
        // In full implementation, would:
        // 1. Decay influence over time
        // 2. Spread influence to neighboring tiles
        // 3. Update tile ownership claims based on influence
    }
    
    const std::vector<InfluenceZone>& zones() const noexcept { return zones_; }
};

} // namespace Manifest::World::Tiles

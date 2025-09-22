#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "../../core/types/Types.hpp"

namespace Manifest::World::Tiles {

using Core::Types::TileId;
using Core::Types::Production;

// Forward declarations
class Tile;
class TileMap;

// Rule-based system for tile improvements and prerequisites
template<typename T>
concept Prerequisite = requires(const T& prereq, const Tile& tile, const TileMap& map) {
    { prereq.check(tile, map) } -> std::convertible_to<bool>;
    { prereq.description() } -> std::convertible_to<std::string_view>;
};

// Basic prerequisite types
class TerrainPrereq {
    TerrainType required_terrain_;
public:
    explicit constexpr TerrainPrereq(TerrainType terrain) noexcept 
        : required_terrain_{terrain} {}
    
    bool check(const Tile& tile, const TileMap&) const noexcept {
        return tile.terrain() == required_terrain_;
    }
    
    std::string_view description() const noexcept {
        switch (required_terrain_) {
            case TerrainType::Hills: return "Requires hills terrain";
            case TerrainType::Mountains: return "Requires mountainous terrain";
            case TerrainType::Coast: return "Requires coastal location";
            default: return "Requires specific terrain";
        }
    }
};

class ResourcePrereq {
    ResourceType required_resource_;
public:
    explicit constexpr ResourcePrereq(ResourceType resource) noexcept 
        : required_resource_{resource} {}
    
    bool check(const Tile& tile, const TileMap&) const noexcept {
        return tile.resource() == required_resource_;
    }
    
    std::string_view description() const noexcept {
        switch (required_resource_) {
            case ResourceType::Iron: return "Requires iron deposits";
            case ResourceType::Stone: return "Requires stone quarry";
            case ResourceType::Fish: return "Requires fish resources";
            default: return "Requires specific resource";
        }
    }
};

class TechPrereq {
    std::uint32_t tech_id_;
public:
    explicit constexpr TechPrereq(std::uint32_t tech_id) noexcept 
        : tech_id_{tech_id} {}
    
    bool check(const Tile& tile, const TileMap& map) const noexcept;
    
    std::string_view description() const noexcept {
        return "Requires technology advancement";
    }
};

class NeighborPrereq {
    ImprovementType required_neighbor_;
    std::uint8_t min_count_;
public:
    explicit constexpr NeighborPrereq(ImprovementType neighbor, std::uint8_t count = 1) noexcept 
        : required_neighbor_{neighbor}, min_count_{count} {}
    
    bool check(const Tile& tile, const TileMap& map) const noexcept;
    
    static std::string_view description() noexcept {
        return "Requires neighboring improvements";
    }
};

// Improvement rule definition
class ImprovementRule {
public:
    using PrereqCheck = std::function<bool(const Tile&, const TileMap&)>;
    
private:
    ImprovementType type_;
    std::vector<PrereqCheck> prerequisites_;
    std::vector<std::string> descriptions_;
    Production cost_{0.0};
    std::uint32_t build_time_{1};
    
public:
    explicit ImprovementRule(ImprovementType type) noexcept : type_{type} {}
    
    template<Prerequisite P>
    ImprovementRule& requires(P&& prereq) {
        prerequisites_.emplace_back([captured_prereq = std::forward<P>(prereq)]
                                  (const Tile& tile, const TileMap& map) {
            return captured_prereq.check(tile, map);
        });
        descriptions_.emplace_back(std::string{prereq.description()});
        return *this;
    }
    
    ImprovementRule& costs(Production amount) noexcept {
        cost_ = amount;
        return *this;
    }
    
    ImprovementRule& build_time(std::uint32_t turns) noexcept {
        build_time_ = turns;
        return *this;
    }
    
    bool can_build(const Tile& tile, const TileMap& map) const noexcept {
        return std::all_of(prerequisites_.begin(), prerequisites_.end(),
                          [&](const auto& check) { return check(tile, map); });
    }
    
    ImprovementType type() const noexcept { return type_; }
    Production cost() const noexcept { return cost_; }
    std::uint32_t build_time() const noexcept { return build_time_; }
    const std::vector<std::string>& requirements() const noexcept { return descriptions_; }
};

// Rule registry for all improvements
class RuleRegistry {
    std::unordered_map<ImprovementType, ImprovementRule> rules_;
    
public:
    RuleRegistry();
    
    const ImprovementRule* get_rule(ImprovementType type) const noexcept {
        auto it = rules_.find(type);
        return it != rules_.end() ? &it->second : nullptr;
    }
    
    bool can_build(ImprovementType type, const Tile& tile, const TileMap& map) const noexcept {
        const auto* rule = get_rule(type);
        return rule && rule->can_build(tile, map);
    }
    
    std::vector<ImprovementType> available_improvements(const Tile& tile, const TileMap& map) const;
    
    // For testing and configuration
    void add_rule(ImprovementRule rule) {
        auto rule_type = rule.type();
        rules_[rule_type] = std::move(rule);
    }
};

// Global registry instance
RuleRegistry& get_improvement_rules();

} // namespace Manifest::World::Tiles

#pragma once

#include "Province.hpp"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <optional>
#include <functional>
#include <queue>
#include <any>

namespace Manifest::World::Tiles {

class Region {
    RegionId id_{RegionId::invalid()};
    std::vector<ProvinceId> provinces_;
    Vec2f center_{};
    std::unordered_set<RegionId> neighbors_;
    
    // Aggregate properties
    TerrainType dominant_terrain_{TerrainType::Ocean};
    std::size_t total_tiles_{0};
    float total_area_{0.0f};
    
    // Climate aggregates
    float average_elevation_{0.0f};
    float average_temperature_{0.0f};
    float average_rainfall_{0.0f};
    float average_fertility_{0.0f};
    
    // Economic totals
    Production total_food_{};
    Production total_production_{};
    Production total_science_{};
    Production total_culture_{};
    Production total_faith_{};
    Money total_trade_{};
    
    // Administrative
    NationId controlling_nation_{NationId::invalid()};
    std::unordered_map<NationId, float> nation_influence_; // Nation -> influence percentage
    bool contested_{false};
    bool coastal_{false};
    
    mutable bool needs_update_{true};
    
public:
    explicit Region(RegionId id) : id_{id} {}
    
    // Basic properties
    RegionId id() const noexcept { return id_; }
    const Vec2f& center() const noexcept { return center_; }
    
    // Province management
    void add_province(ProvinceId province_id) {
        provinces_.push_back(province_id);
        needs_update_ = true;
    }
    
    void remove_province(ProvinceId province_id) {
        provinces_.erase(std::remove(provinces_.begin(), provinces_.end(), province_id), 
                        provinces_.end());
        needs_update_ = true;
    }
    
    const std::vector<ProvinceId>& provinces() const noexcept { return provinces_; }
    std::size_t province_count() const noexcept { return provinces_.size(); }
    
    // Neighbor management
    void add_neighbor(RegionId neighbor) {
        neighbors_.insert(neighbor);
    }
    
    void remove_neighbor(RegionId neighbor) {
        neighbors_.erase(neighbor);
    }
    
    const std::unordered_set<RegionId>& neighbors() const noexcept { 
        return neighbors_; 
    }
    
    // Aggregate properties
    TerrainType dominant_terrain() const { update_if_needed(); return dominant_terrain_; }
    std::size_t total_tiles() const { update_if_needed(); return total_tiles_; }
    float total_area() const { update_if_needed(); return total_area_; }
    
    float average_elevation() const { update_if_needed(); return average_elevation_; }
    float average_temperature() const { update_if_needed(); return average_temperature_; }
    float average_rainfall() const { update_if_needed(); return average_rainfall_; }
    float average_fertility() const { update_if_needed(); return average_fertility_; }
    
    // Economic totals
    Production total_food() const { update_if_needed(); return total_food_; }
    Production total_production() const { update_if_needed(); return total_production_; }
    Production total_science() const { update_if_needed(); return total_science_; }
    Production total_culture() const { update_if_needed(); return total_culture_; }
    Production total_faith() const { update_if_needed(); return total_faith_; }
    Money total_trade() const { update_if_needed(); return total_trade_; }
    
    // Administrative
    NationId controlling_nation() const noexcept { return controlling_nation_; }
    void set_controlling_nation(NationId nation) noexcept { controlling_nation_ = nation; }
    
    const std::unordered_map<NationId, float>& nation_influence() const noexcept {
        return nation_influence_;
    }
    
    void set_nation_influence(NationId nation, float influence) {
        nation_influence_[nation] = std::clamp(influence, 0.0f, 1.0f);
        
        // Update contested status
        contested_ = nation_influence_.size() > 1 && 
                    std::any_of(nation_influence_.begin(), nation_influence_.end(),
                               [](const auto& pair) { return pair.second > 0.1f; });
    }
    
    bool is_contested() const noexcept { return contested_; }
    bool is_coastal() const { update_if_needed(); return coastal_; }
    
    // Update from constituent provinces
    void update_from_provinces(const ProvinceMap& province_map) {
        if (!needs_update_) return;
        
        reset_aggregates();
        
        if (provinces_.empty()) {
            needs_update_ = false;
            return;
        }
        
        // Accumulate data from all provinces
        Vec2f center_sum{};
        float weight_sum = 0.0f;
        std::unordered_map<TerrainType, std::size_t> terrain_counts;
        
        for (ProvinceId province_id : provinces_) {
            const Province* province = province_map.get_province(province_id);
            if (!province) continue;
            
            float weight = province->area(); // Use area as weight
            weight_sum += weight;
            
            // Weighted center calculation
            center_sum += province->center() * weight;
            
            // Aggregate properties
            total_tiles_ += province->tile_count();
            total_area_ += province->area();
            
            // Weighted averages
            average_elevation_ += province->average_elevation() * weight;
            average_temperature_ += province->average_temperature() * weight;
            average_rainfall_ += province->average_rainfall() * weight;
            average_fertility_ += province->average_fertility() * weight;
            
            // Economic totals
            total_food_ += province->total_food();
            total_production_ += province->total_production();
            total_science_ += province->total_science();
            total_culture_ += province->total_culture();
            total_faith_ += province->total_faith();
            total_trade_ += province->total_trade();
            
            // Terrain analysis
            terrain_counts[province->dominant_terrain()] += province->tile_count();
            
            // Check for coastal provinces
            if (province->is_coastal()) {
                coastal_ = true;
            }
        }
        
        // Finalize weighted averages
        if (weight_sum > 0.0f) {
            center_ = center_sum / weight_sum;
            average_elevation_ /= weight_sum;
            average_temperature_ /= weight_sum;
            average_rainfall_ /= weight_sum;
            average_fertility_ /= weight_sum;
        }
        
        // Determine dominant terrain
        if (!terrain_counts.empty()) {
            auto max_terrain = std::max_element(terrain_counts.begin(), terrain_counts.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; });
            dominant_terrain_ = max_terrain->first;
        }
        
        needs_update_ = false;
    }
    
    // Utility functions
    bool contains_province(ProvinceId province_id) const {
        return std::find(provinces_.begin(), provinces_.end(), province_id) != provinces_.end();
    }
    
    bool is_valid() const noexcept {
        return id_.is_valid() && !provinces_.empty();
    }
    
    // Strategic value calculation
    float calculate_strategic_value() const {
        float value = 0.0f;
        
        // Base value from economic output
        value += total_food().value() * 0.1f;
        value += total_production().value() * 0.2f;
        value += total_science().value() * 0.3f;
        value += total_culture().value() * 0.15f;
        value += total_trade().value() * 0.2f;
        
        // Bonuses
        if (coastal_) value *= 1.2f; // Coastal regions are more valuable
        if (contested_) value *= 0.8f; // Contested regions are harder to utilize
        
        // Terrain bonuses
        switch (dominant_terrain_) {
            case TerrainType::Grassland:
            case TerrainType::Plains:
                value *= 1.1f; // Good for agriculture
                break;
            case TerrainType::Hills:
            case TerrainType::Mountains:
                value *= 1.05f; // Good for production
                break;
            case TerrainType::Desert:
            case TerrainType::Tundra:
                value *= 0.9f; // Harsh terrain
                break;
            default:
                break;
        }
        
        return value;
    }
    
private:
    void reset_aggregates() {
        total_tiles_ = 0;
        total_area_ = 0.0f;
        average_elevation_ = 0.0f;
        average_temperature_ = 0.0f;
        average_rainfall_ = 0.0f;
        average_fertility_ = 0.0f;
        total_food_ = Production{};
        total_production_ = Production{};
        total_science_ = Production{};
        total_culture_ = Production{};
        total_faith_ = Production{};
        total_trade_ = Money{};
        coastal_ = false;
    }
    
    void update_if_needed() const {
        // Note: This would need access to province map, simplified for now
        // In practice, would call update_from_provinces when needed
    }
};

class RegionMap {
    std::unordered_map<RegionId, std::unique_ptr<Region>> regions_;
    std::unordered_map<ProvinceId, RegionId> province_to_region_;
    RegionId next_id_{RegionId{1}};
    
public:
    RegionMap() = default;
    ~RegionMap() = default;
    
    // Non-copyable but movable
    RegionMap(const RegionMap&) = delete;
    RegionMap& operator=(const RegionMap&) = delete;
    RegionMap(RegionMap&&) = default;
    RegionMap& operator=(RegionMap&&) = default;
    
    // Region creation and management
    RegionId create_region() {
        RegionId id = next_id_++;
        auto region = std::make_unique<Region>(id);
        regions_[id] = std::move(region);
        return id;
    }
    
    bool remove_region(RegionId id) {
        auto it = regions_.find(id);
        if (it == regions_.end()) return false;
        
        // Remove all province mappings
        for (ProvinceId province_id : it->second->provinces()) {
            province_to_region_.erase(province_id);
        }
        
        regions_.erase(it);
        return true;
    }
    
    // Region access
    Region* get_region(RegionId id) noexcept {
        auto it = regions_.find(id);
        return it != regions_.end() ? it->second.get() : nullptr;
    }
    
    const Region* get_region(RegionId id) const noexcept {
        return const_cast<RegionMap*>(this)->get_region(id);
    }
    
    RegionId get_region_for_province(ProvinceId province_id) const noexcept {
        auto it = province_to_region_.find(province_id);
        return it != province_to_region_.end() ? it->second : RegionId::invalid();
    }
    
    // Province assignment
    bool assign_province_to_region(ProvinceId province_id, RegionId region_id) {
        Region* region = get_region(region_id);
        if (!region) return false;
        
        // Remove from previous region if assigned
        RegionId old_region = get_region_for_province(province_id);
        if (old_region.is_valid() && old_region != region_id) {
            Region* old = get_region(old_region);
            if (old) old->remove_province(province_id);
        }
        
        // Add to new region
        region->add_province(province_id);
        province_to_region_[province_id] = region_id;
        
        return true;
    }
    
    // Bulk operations
    void update_all_regions(const ProvinceMap& province_map) {
        for (auto& [id, region] : regions_) {
            region->update_from_provinces(province_map);
        }
        
        // Update neighbor relationships
        update_region_neighbors(province_map);
    }
    
    void for_each_region(std::function<void(Region&)> func) {
        for (auto& [id, region] : regions_) {
            func(*region);
        }
    }
    
    void for_each_region(std::function<void(const Region&)> func) const {
        for (const auto& [id, region] : regions_) {
            func(*region);
        }
    }
    
    // Statistics
    std::size_t region_count() const noexcept { return regions_.size(); }
    
    void clear() {
        regions_.clear();
        province_to_region_.clear();
        next_id_ = RegionId{1};
    }
    
    // Auto-generation from province map
    void generate_regions_from_provinces(const ProvinceMap& province_map, 
                                         std::size_t target_count = 8) {
        clear();
        
        // Select region centers (major provinces)
        auto seed_provinces = select_region_seeds(province_map, target_count);
        
        // Create initial regions from seeds
        for (ProvinceId seed : seed_provinces) {
            RegionId region_id = create_region();
            assign_province_to_region(seed, region_id);
        }
        
        // Grow regions using geographic proximity and compatibility
        grow_regions(province_map);
        
        // Update all region data
        update_all_regions(province_map);
    }
    
private:
    std::vector<ProvinceId> select_region_seeds(const ProvinceMap& province_map, 
                                                std::size_t count) {
        std::vector<std::pair<ProvinceId, float>> candidates;
        
        // Score provinces based on suitability as region centers
        province_map.for_each_province([&](const Province& province) {
            float score = calculate_region_seed_score(province);
            candidates.emplace_back(province.id(), score);
        });
        
        // Sort by score and select with distance constraints
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<ProvinceId> seeds;
        const float MIN_SEED_DISTANCE = 15.0f;
        
        for (const auto& candidate : candidates) {
            if (seeds.size() >= count) break;
            
            ProvinceId candidate_id = candidate.first;
            const Province* candidate_province = province_map.get_province(candidate_id);
            if (!candidate_province) continue;
            
            // Check distance to existing seeds
            bool too_close = false;
            for (ProvinceId existing_seed : seeds) {
                const Province* existing_province = province_map.get_province(existing_seed);
                if (!existing_province) continue;
                
                float distance = distance(candidate_province->center(),
                                        existing_province->center());
                if (distance < MIN_SEED_DISTANCE) {
                    too_close = true;
                    break;
                }
            }
            
            if (!too_close) {
                seeds.push_back(candidate_id);
            }
        }
        
        return seeds;
    }
    
    float calculate_region_seed_score(const Province& province) {
        float score = 0.0f;
        
        // Larger, more developed provinces make better region centers
        score += province.tile_count() * 0.1f;
        score += province.total_production().value() * 0.2f;
        score += province.total_culture().value() * 0.15f;
        
        // Central location bonus (moderate elevation and fertility)
        score += province.average_fertility() * 2.0f;
        
        float elevation = province.average_elevation();
        if (elevation > 0.2f && elevation < 0.7f) {
            score += 1.0f; // Moderate elevation preferred
        }
        
        // Coastal bonus
        if (province.is_coastal()) {
            score += 0.5f;
        }
        
        // Terrain bonuses
        switch (province.dominant_terrain()) {
            case TerrainType::Grassland:
            case TerrainType::Plains:
                score += 1.5f;
                break;
            case TerrainType::Hills:
                score += 1.0f;
                break;
            case TerrainType::Coast:
                score += 1.0f;
                break;
            default:
                break;
        }
        
        return score;
    }
    
    void grow_regions(const ProvinceMap& province_map) {
        std::queue<ProvinceId> growth_queue;
        
        // Initialize queue with all assigned provinces
        for (const auto& [province_id, region_id] : province_to_region_) {
            growth_queue.push(province_id);
        }
        
        // Grow regions using breadth-first expansion
        while (!growth_queue.empty()) {
            ProvinceId current_province = growth_queue.front();
            growth_queue.pop();
            
            RegionId current_region = get_region_for_province(current_province);
            if (!current_region.is_valid()) continue;
            
            const Province* province = province_map.get_province(current_province);
            if (!province) continue;
            
            // Try to expand to neighboring provinces
            for (ProvinceId neighbor_id : province->neighbors()) {
                if (!neighbor_id.is_valid()) continue;
                
                // Skip if already assigned
                if (get_region_for_province(neighbor_id).is_valid()) continue;
                
                const Province* neighbor = province_map.get_province(neighbor_id);
                if (!neighbor) continue;
                
                // Check if suitable for this region
                if (should_assign_to_region(*neighbor, current_region)) {
                    assign_province_to_region(neighbor_id, current_region);
                    growth_queue.push(neighbor_id);
                }
            }
        }
    }
    
    bool should_assign_to_region(const Province& province, RegionId region_id) {
        const Region* region = get_region(region_id);
        if (!region) return false;
        
        // Geographic and terrain compatibility
        TerrainType province_terrain = province.dominant_terrain();
        TerrainType region_terrain = region->dominant_terrain();
        
        // Similar terrain types preferred
        if (province_terrain == region_terrain) return true;
        
        // Compatible terrain combinations
        return are_compatible_terrains(province_terrain, region_terrain);
    }
    
    bool are_compatible_terrains(TerrainType a, TerrainType b) {
        // Define regional terrain compatibility (more flexible than province level)
        if (a == b) return true;
        
        // Land-based compatibility
        static const std::unordered_set<TerrainType> grassland_group = {
            TerrainType::Grassland, TerrainType::Plains, TerrainType::Hills
        };
        
        static const std::unordered_set<TerrainType> mountain_group = {
            TerrainType::Mountains, TerrainType::Hills, TerrainType::Tundra
        };
        
        static const std::unordered_set<TerrainType> forest_group = {
            TerrainType::Forest, TerrainType::Hills, TerrainType::Plains
        };
        
        return (grassland_group.contains(a) && grassland_group.contains(b)) ||
               (mountain_group.contains(a) && mountain_group.contains(b)) ||
               (forest_group.contains(a) && forest_group.contains(b));
    }
    
    void update_region_neighbors(const ProvinceMap& province_map) {
        // Clear existing neighbor relationships
        for (auto& [id, region] : regions_) {
            region->neighbors().clear();
        }
        
        // Find adjacent regions through province neighbors
        for (const auto& [id, region] : regions_) {
            std::unordered_set<RegionId> region_neighbors;
            
            for (ProvinceId province_id : region->provinces()) {
                const Province* province = province_map.get_province(province_id);
                if (!province) continue;
                
                for (ProvinceId neighbor_id : province->neighbors()) {
                    if (!neighbor_id.is_valid()) continue;
                    
                    RegionId neighbor_region = get_region_for_province(neighbor_id);
                    if (neighbor_region.is_valid() && neighbor_region != id) {
                        region_neighbors.insert(neighbor_region);
                    }
                }
            }
            
            // Add all found neighbors
            for (RegionId neighbor : region_neighbors) {
                region->add_neighbor(neighbor);
            }
        }
    }
};

// Complete hierarchical system
class HierarchicalMap {
    TileMap tile_map_;
    ProvinceMap province_map_;
    RegionMap region_map_;
    
public:
    HierarchicalMap() = default;
    
    // Access to individual layers
    TileMap& tiles() noexcept { return tile_map_; }
    const TileMap& tiles() const noexcept { return tile_map_; }
    
    ProvinceMap& provinces() noexcept { return province_map_; }
    const ProvinceMap& provinces() const noexcept { return province_map_; }
    
    RegionMap& regions() noexcept { return region_map_; }
    const RegionMap& regions() const noexcept { return region_map_; }
    
    // Generate complete hierarchy
    void generate_hierarchy(std::size_t target_provinces = 50, std::size_t target_regions = 8) {
        // Generate provinces from tiles
        province_map_.generate_provinces_from_tiles(tile_map_, target_provinces);
        
        // Generate regions from provinces
        region_map_.generate_regions_from_provinces(province_map_, target_regions);
    }
    
    // Update entire hierarchy
    void update_all() {
        province_map_.update_all_provinces(tile_map_);
        region_map_.update_all_regions(province_map_);
    }
    
    // Query functions that traverse hierarchy
    RegionId get_region_for_tile(TileId tile_id) const {
        ProvinceId province = province_map_.get_province_for_tile(tile_id);
        return province.is_valid() ? 
               region_map_.get_region_for_province(province) : 
               RegionId::invalid();
    }
    
    std::vector<TileId> get_tiles_in_region(RegionId region_id) const {
        std::vector<TileId> tiles;
        
        const Region* region = region_map_.get_region(region_id);
        if (!region) return tiles;
        
        for (ProvinceId province_id : region->provinces()) {
            const Province* province = province_map_.get_province(province_id);
            if (!province) continue;
            
            for (TileId tile_id : province->tiles()) {
                tiles.push_back(tile_id);
            }
        }
        
        return tiles;
    }
};

} // namespace Manifest::World::Tiles

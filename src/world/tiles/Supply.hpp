#pragma once

#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../../core/types/Types.hpp"

namespace Manifest::World::Tiles {

using Core::Types::TileId;
using Core::Types::Production;
using Core::Types::Money;

// Forward declarations
class Tile;
class TileMap;

// Supply node representing production/consumption point
struct SupplyNode {
    TileId tile_id{TileId::invalid()};
    
    enum class Type : std::uint8_t {
        Producer,   // Produces resources
        Consumer,   // Consumes resources  
        Storage,    // Stores resources temporarily
        Hub         // Distribution center
    } type{Type::Producer};
    
    std::uint8_t resource_type{0}; // ResourceType placeholder
    Production capacity{0.0};
    Production current_supply{0.0};
    Production demand{0.0};
    
    constexpr bool has_surplus() const noexcept {
        return current_supply > demand;
    }
    
    constexpr Production surplus() const noexcept {
        return Production{std::max(0.0, current_supply.value() - demand.value())};
    }
    
    constexpr Production deficit() const noexcept {
        return Production{std::max(0.0, demand.value() - current_supply.value())};
    }
};

// Supply chain connection between nodes
struct SupplyRoute {
    TileId from{TileId::invalid()};
    TileId to{TileId::invalid()};
    std::vector<TileId> path;
    
    float distance{0.0F};
    Money transport_cost{0.0};
    Production flow_capacity{0.0};
    Production current_flow{0.0};
    
    bool active{true};
    bool congested{false};
    
    constexpr float efficiency() const noexcept {
        if (flow_capacity.value() <= 0.0) {
            return 0.0F;
        }
        return static_cast<float>(current_flow.value() / flow_capacity.value());
    }
    
    constexpr bool is_saturated() const noexcept {
        return current_flow >= flow_capacity;
    }
};

// Pathfinding for supply chains using A* algorithm
class SupplyPathfinder {
public:
    struct PathNode {
        TileId tile_id{TileId::invalid()};
        float g_cost{0.0F};  // Cost from start
        float h_cost{0.0F};  // Heuristic cost to end
        float f_cost() const noexcept { return g_cost + h_cost; }
        TileId parent{TileId::invalid()};
        
        bool operator>(const PathNode& other) const noexcept {
            return f_cost() > other.f_cost();
        }
    };
    
private:
    const TileMap* map_{nullptr};
    
public:
    explicit SupplyPathfinder(const TileMap* tilemap) noexcept : map_{tilemap} {}
    
    std::vector<TileId> find_path(TileId start, TileId goal) const;
    
    float calculate_movement_cost(TileId from, TileId to) const;
    
    float heuristic_distance(TileId from, TileId to) const;
    
private:
    std::vector<TileId> reconstruct_path(const std::unordered_map<TileId, TileId>& came_from,
                                        TileId current) const;
};

// Supply chain network managing all connections
class SupplyNetwork {
    std::unordered_map<TileId, SupplyNode> nodes_;
    std::vector<SupplyRoute> routes_;
    std::unordered_map<TileId, std::vector<std::size_t>> node_routes_; // node -> route indices
    
    SupplyPathfinder pathfinder_;
    
public:
    explicit SupplyNetwork(const TileMap* tilemap) : pathfinder_{tilemap} {}
    
    // Node management
    void add_node(const SupplyNode& node) {
        nodes_[node.tile_id] = node;
        node_routes_[node.tile_id]; // Ensure entry exists
    }
    
    void remove_node(TileId tile_id) {
        auto it = nodes_.find(tile_id);
        if (it != nodes_.end()) {
            // Remove all routes connected to this node
            auto& route_indices = node_routes_[tile_id];
            for (auto route_idx : route_indices) {
                routes_[route_idx].active = false;
            }
            route_indices.clear();
            
            nodes_.erase(it);
        }
    }
    
    SupplyNode* get_node(TileId tile_id) {
        auto it = nodes_.find(tile_id);
        return it != nodes_.end() ? &it->second : nullptr;
    }
    
    // Route management
    bool create_route(TileId from, TileId to, Production capacity = Production{100.0});
    
    void remove_route(TileId from, TileId to);
    
    const std::vector<SupplyRoute>& routes() const noexcept { return routes_; }
    
    // Network analysis
    std::vector<TileId> find_suppliers(TileId consumer, std::uint8_t resource_type) const;
    
    std::vector<TileId> find_consumers(TileId producer, std::uint8_t resource_type) const;
    
    // Flow simulation
    void simulate_supply_flow();
    
    // Network optimization
    void optimize_routes();
    
    // Statistics
    struct NetworkStats {
        std::size_t total_nodes{0};
        std::size_t active_routes{0};
        std::size_t congested_routes{0};
        Production total_supply{0.0};
        Production total_demand{0.0};
        Money total_transport_cost{0.0};
        float average_efficiency{0.0F};
    };
    
    NetworkStats calculate_statistics() const;
    
private:
    void balance_supply_demand();
    
    void update_route_costs();
    
    float calculate_transport_cost(const SupplyRoute& route) const;
};

// Supply chain manager for a specific resource type
class ResourceSupplyChain {
    std::uint8_t resource_type_;
    SupplyNetwork* network_;
    
    std::unordered_set<TileId> producers_;
    std::unordered_set<TileId> consumers_;
    std::unordered_set<TileId> storage_facilities_;
    
public:
    ResourceSupplyChain(std::uint8_t resource_type, SupplyNetwork* network) noexcept
        : resource_type_{resource_type}, network_{network} {}
    
    void add_producer(TileId tile_id, Production capacity) {
        SupplyNode node;
        node.tile_id = tile_id;
        node.type = SupplyNode::Type::Producer;
        node.resource_type = resource_type_;
        node.capacity = capacity;
        node.current_supply = capacity; // Start at full capacity
        
        network_->add_node(node);
        producers_.insert(tile_id);
    }
    
    void add_consumer(TileId tile_id, Production demand) {
        SupplyNode node;
        node.tile_id = tile_id;
        node.type = SupplyNode::Type::Consumer;
        node.resource_type = resource_type_;
        node.demand = demand;
        
        network_->add_node(node);
        consumers_.insert(tile_id);
    }
    
    void add_storage(TileId tile_id, Production capacity) {
        SupplyNode node;
        node.tile_id = tile_id;
        node.type = SupplyNode::Type::Storage;
        node.resource_type = resource_type_;
        node.capacity = capacity;
        
        network_->add_node(node);
        storage_facilities_.insert(tile_id);
    }
    
    // Automatically create optimal supply routes
    void establish_supply_routes();
    
    // Calculate total supply/demand balance for this resource
    struct Balance {
        Production total_production{0.0};
        Production total_consumption{0.0};
        Production net_surplus{0.0};
        bool balanced{false};
    };
    
    Balance calculate_balance() const;
    
    // Find critical bottlenecks in supply chain
    std::vector<TileId> find_bottlenecks() const;
    
    const std::unordered_set<TileId>& producers() const noexcept { return producers_; }
    const std::unordered_set<TileId>& consumers() const noexcept { return consumers_; }
    const std::unordered_set<TileId>& storage() const noexcept { return storage_facilities_; }
};

} // namespace Manifest::World::Tiles

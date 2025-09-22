#include "Supply.hpp"
#include "Tile.hpp"
#include "Map.hpp"
#include <algorithm>
#include <cmath>

namespace Manifest::World::Tiles {

std::vector<TileId> SupplyPathfinder::find_path(TileId start, TileId goal) const {
    if (!map_ || !start.is_valid() || !goal.is_valid()) {
        return {};
    }
    
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> open_set;
    std::unordered_set<TileId> closed_set;
    std::unordered_map<TileId, TileId> came_from;
    std::unordered_map<TileId, float> g_score;
    
    // Initialize start node
    PathNode start_node;
    start_node.tile_id = start;
    start_node.g_cost = 0.0F;
    start_node.h_cost = heuristic_distance(start, goal);
    
    open_set.push(start_node);
    g_score[start] = 0.0F;
    
    while (!open_set.empty()) {
        PathNode current = open_set.top();
        open_set.pop();
        
        if (current.tile_id == goal) {
            return reconstruct_path(came_from, goal);
        }
        
        closed_set.insert(current.tile_id);
        
        const Tile* current_tile = map_->get_tile(current.tile_id);
        if (!current_tile) {
            continue;
        }
        
        // Check all neighbors
        for (const auto& neighbor_id : current_tile->neighbors()) {
            if (!neighbor_id.is_valid() || closed_set.count(neighbor_id)) {
                continue;
            }
            
            float tentative_g_score = g_score[current.tile_id] + 
                                    calculate_movement_cost(current.tile_id, neighbor_id);
            
            auto g_it = g_score.find(neighbor_id);
            if (g_it == g_score.end() || tentative_g_score < g_it->second) {
                came_from[neighbor_id] = current.tile_id;
                g_score[neighbor_id] = tentative_g_score;
                
                PathNode neighbor_node;
                neighbor_node.tile_id = neighbor_id;
                neighbor_node.g_cost = tentative_g_score;
                neighbor_node.h_cost = heuristic_distance(neighbor_id, goal);
                neighbor_node.parent = current.tile_id;
                
                open_set.push(neighbor_node);
            }
        }
    }
    
    return {}; // No path found
}

float SupplyPathfinder::calculate_movement_cost(TileId from, TileId to) const {
    if (!map_) {
        return std::numeric_limits<float>::infinity();
    }
    
    const Tile* from_tile = map_->get_tile(from);
    const Tile* to_tile = map_->get_tile(to);
    
    if (!from_tile || !to_tile) {
        return std::numeric_limits<float>::infinity();
    }
    
    float base_cost = 1.0F;
    
    // Factor in terrain difficulty for supply transport
    base_cost *= to_tile->movement_cost();
    
    // Roads reduce transport costs
    if (to_tile->improvement() == ImprovementType::Road) {
        base_cost *= 0.5F;
    } else if (to_tile->improvement() == ImprovementType::Railroad) {
        base_cost *= 0.2F;
    }
    
    // Water tiles are impassable for land-based supply
    if (to_tile->is_water()) {
        base_cost *= 3.0F; // Requires ships/bridges
    }
    
    return base_cost;
}

float SupplyPathfinder::heuristic_distance(TileId from, TileId to) const {
    if (!map_) {
        return 0.0F;
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

std::vector<TileId> SupplyPathfinder::reconstruct_path(
    const std::unordered_map<TileId, TileId>& came_from, TileId current) const {
    
    std::vector<TileId> path;
    path.push_back(current);
    
    while (came_from.find(current) != came_from.end()) {
        current = came_from.at(current);
        path.push_back(current);
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

bool SupplyNetwork::create_route(TileId from, TileId to, Production capacity) {
    // Check if nodes exist
    if (nodes_.find(from) == nodes_.end() || nodes_.find(to) == nodes_.end()) {
        return false;
    }
    
    // Find path between nodes
    auto path = pathfinder_.find_path(from, to);
    if (path.empty()) {
        return false;
    }
    
    // Create route
    SupplyRoute route;
    route.from = from;
    route.to = to;
    route.path = std::move(path);
    route.flow_capacity = capacity;
    route.distance = static_cast<float>(route.path.size());
    route.transport_cost = Money{calculate_transport_cost(route)};
    
    std::size_t route_index = routes_.size();
    routes_.push_back(route);
    
    // Update node route mappings
    node_routes_[from].push_back(route_index);
    node_routes_[to].push_back(route_index);
    
    return true;
}

void SupplyNetwork::remove_route(TileId from, TileId to) {
    for (std::size_t i = 0; i < routes_.size(); ++i) {
        if (routes_[i].from == from && routes_[i].to == to) {
            routes_[i].active = false;
            
            // Remove from node mappings
            auto& from_routes = node_routes_[from];
            auto& to_routes = node_routes_[to];
            
            from_routes.erase(std::remove(from_routes.begin(), from_routes.end(), i), 
                            from_routes.end());
            to_routes.erase(std::remove(to_routes.begin(), to_routes.end(), i), 
                          to_routes.end());
            break;
        }
    }
}

std::vector<TileId> SupplyNetwork::find_suppliers(TileId consumer, std::uint8_t resource_type) const {
    std::vector<TileId> suppliers;
    
    for (const auto& [tile_id, node] : nodes_) {
        if (node.type == SupplyNode::Type::Producer && 
            node.resource_type == resource_type && 
            node.has_surplus()) {
            suppliers.push_back(tile_id);
        }
    }
    
    // Sort by distance to consumer (approximate)
    std::sort(suppliers.begin(), suppliers.end(), 
        [this, consumer](TileId a, TileId b) {
            return pathfinder_.heuristic_distance(a, consumer) < 
                   pathfinder_.heuristic_distance(b, consumer);
        });
    
    return suppliers;
}

std::vector<TileId> SupplyNetwork::find_consumers(TileId producer, std::uint8_t resource_type) const {
    std::vector<TileId> consumers;
    
    for (const auto& [tile_id, node] : nodes_) {
        if (node.type == SupplyNode::Type::Consumer && 
            node.resource_type == resource_type && 
            node.deficit().value() > 0.0) {
            consumers.push_back(tile_id);
        }
    }
    
    return consumers;
}

void SupplyNetwork::simulate_supply_flow() {
    // Reset all flow values
    for (auto& route : routes_) {
        route.current_flow = Production{0.0};
        route.congested = false;
    }
    
    balance_supply_demand();
    update_route_costs();
}

void SupplyNetwork::balance_supply_demand() {
    // Simple supply allocation algorithm
    for (auto& [tile_id, node] : nodes_) {
        if (node.type == SupplyNode::Type::Consumer && node.deficit().value() > 0.0) {
            auto suppliers = find_suppliers(tile_id, node.resource_type);
            
            Production remaining_demand = node.deficit();
            
            for (TileId supplier_id : suppliers) {
                if (remaining_demand.value() <= 0.0) {
                    break;
                }
                
                auto* supplier = get_node(supplier_id);
                if (!supplier || supplier->surplus().value() <= 0.0) {
                    continue;
                }
                
                Production flow_amount = Production{
                    std::min(remaining_demand.value(), supplier->surplus().value())
                };
                
                // Find route between supplier and consumer
                for (auto& route : routes_) {
                    if (route.from == supplier_id && route.to == tile_id && route.active) {
                        Production available_capacity = Production{
                            route.flow_capacity.value() - route.current_flow.value()
                        };
                        
                        Production actual_flow = Production{
                            std::min(flow_amount.value(), available_capacity.value())
                        };
                        
                        route.current_flow += actual_flow;
                        supplier->current_supply -= actual_flow;
                        node.current_supply += actual_flow;
                        remaining_demand -= actual_flow;
                        
                        if (route.is_saturated()) {
                            route.congested = true;
                        }
                        
                        break;
                    }
                }
            }
        }
    }
}

void SupplyNetwork::update_route_costs() {
    for (auto& route : routes_) {
        route.transport_cost = Money{calculate_transport_cost(route)};
    }
}

float SupplyNetwork::calculate_transport_cost(const SupplyRoute& route) const {
    float base_cost = route.distance * 0.1F; // Base cost per tile
    
    // Congestion increases costs
    if (route.congested) {
        base_cost *= 1.5F;
    }
    
    // Efficiency affects costs
    float efficiency = route.efficiency();
    if (efficiency > 0.8F) {
        base_cost *= 1.2F; // High utilization increases wear/costs
    }
    
    return base_cost;
}

SupplyNetwork::NetworkStats SupplyNetwork::calculate_statistics() const {
    NetworkStats stats;
    stats.total_nodes = nodes_.size();
    
    Production total_supply{0.0};
    Production total_demand{0.0};
    Money total_cost{0.0};
    float total_efficiency = 0.0F;
    std::size_t active_count = 0;
    std::size_t congested_count = 0;
    
    for (const auto& [tile_id, node] : nodes_) {
        total_supply += node.current_supply;
        total_demand += node.demand;
    }
    
    for (const auto& route : routes_) {
        if (route.active) {
            ++active_count;
            total_cost += route.transport_cost;
            total_efficiency += route.efficiency();
            
            if (route.congested) {
                ++congested_count;
            }
        }
    }
    
    stats.active_routes = active_count;
    stats.congested_routes = congested_count;
    stats.total_supply = total_supply;
    stats.total_demand = total_demand;
    stats.total_transport_cost = total_cost;
    stats.average_efficiency = active_count > 0 ? total_efficiency / static_cast<float>(active_count) : 0.0F;
    
    return stats;
}

void ResourceSupplyChain::establish_supply_routes() {
    // Connect each consumer to nearest suppliers
    for (TileId consumer_id : consumers_) {
        auto suppliers = network_->find_suppliers(consumer_id, resource_type_);
        
        // Connect to closest 2-3 suppliers for redundancy
        std::size_t connections = std::min(suppliers.size(), std::size_t{3});
        for (std::size_t i = 0; i < connections; ++i) {
            network_->create_route(suppliers[i], consumer_id);
        }
    }
    
    // Connect storage facilities to producers and consumers
    for (TileId storage_id : storage_facilities_) {
        // Connect to nearby producers
        auto nearby_producers = network_->find_suppliers(storage_id, resource_type_);
        if (!nearby_producers.empty()) {
            network_->create_route(nearby_producers[0], storage_id);
        }
        
        // Connect to nearby consumers
        auto nearby_consumers = network_->find_consumers(storage_id, resource_type_);
        for (std::size_t i = 0; i < std::min(nearby_consumers.size(), std::size_t{2}); ++i) {
            network_->create_route(storage_id, nearby_consumers[i]);
        }
    }
}

ResourceSupplyChain::Balance ResourceSupplyChain::calculate_balance() const {
    Balance balance;
    
    for (TileId producer_id : producers_) {
        const auto* node = network_->get_node(producer_id);
        if (node) {
            balance.total_production += node->capacity;
        }
    }
    
    for (TileId consumer_id : consumers_) {
        const auto* node = network_->get_node(consumer_id);
        if (node) {
            balance.total_consumption += node->demand;
        }
    }
    
    balance.net_surplus = Production{
        balance.total_production.value() - balance.total_consumption.value()
    };
    
    balance.balanced = std::abs(balance.net_surplus.value()) < 1.0; // Within 1 unit
    
    return balance;
}

std::vector<TileId> ResourceSupplyChain::find_bottlenecks() const {
    std::vector<TileId> bottlenecks;
    
    // Find consumers with high unmet demand
    for (TileId consumer_id : consumers_) {
        const auto* node = network_->get_node(consumer_id);
        if (node && node->deficit().value() > node->demand.value() * 0.5) {
            bottlenecks.push_back(consumer_id);
        }
    }
    
    return bottlenecks;
}

} // namespace Manifest::World::Tiles

#include <iostream>
#include <chrono>
#include <vector>
#include <cassert>
#include "../../src/world/tiles/Map.hpp"

// Simple performance test without gtest dependency
using namespace Manifest::World::Tiles;

void test_large_world_generation() {
    std::cout << "Running large world generation test..." << std::endl;
    
    TileMap map;
    
    // Generate a world with basic tiles
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create a grid of tiles (simulating world generation)
    const int RADIUS = 50;
    int tiles_created = 0;
    
    for (int q = -RADIUS; q <= RADIUS; ++q) {
        int r1 = std::max(-RADIUS, -q - RADIUS);
        int r2 = std::min(RADIUS, -q + RADIUS);
        
        for (int r = r1; r <= r2; ++r) {
            int s = -q - r;
            HexCoordinate coord{q, r, s};
            
            if (coord.is_valid()) {
                TileId id = map.create_tile(coord);
                if (id.is_valid()) {
                    tiles_created++;
                }
            }
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Generated " << tiles_created << " tiles in " 
              << elapsed.count() << " μs" << std::endl;
    
    // Basic assertions
    assert(tiles_created > 1000);
    assert(elapsed.count() < 5000000); // 5 seconds
    
    std::cout << "Large world generation test PASSED" << std::endl;
}

void test_tile_access_performance() {
    std::cout << "Running tile access performance test..." << std::endl;
    
    TileMap map;
    
    // Create some tiles
    std::vector<TileId> tile_ids;
    for (int i = 0; i < 100; ++i) {
        HexCoordinate coord{i, -i/2, -i + i/2};
        if (coord.is_valid()) {
            TileId id = map.create_tile(coord);
            if (id.is_valid()) {
                tile_ids.push_back(id);
            }
        }
    }
    
    if (tile_ids.empty()) {
        std::cout << "No tiles created, skipping access test" << std::endl;
        return;
    }
    
    constexpr int ITERATIONS = 10000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        TileId id = tile_ids[i % tile_ids.size()];
        const Tile* tile = map.get_tile(id);
        if (tile) {
            volatile auto terrain = tile->terrain(); // Prevent optimization
            (void)terrain;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Accessed " << ITERATIONS << " tiles in " 
              << elapsed.count() << " μs" << std::endl;
    
    // Basic assertion
    assert(elapsed.count() < 10000); // 10ms
    
    std::cout << "Tile access performance test PASSED" << std::endl;
}

int main() {
    try {
        test_large_world_generation();
        test_tile_access_performance();
        
        std::cout << "All performance tests PASSED!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
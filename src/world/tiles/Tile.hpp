#pragma once

#include <array>
#include <bitset>

#include "../../core/types/Types.hpp"

namespace Manifest {
namespace World {
namespace Tiles {

using namespace Core::Types;

// Terrain types
enum class TerrainType : std::uint8_t {
    Ocean,
    Coast,
    Plains,
    Grassland,
    Hills,
    Mountains,
    Desert,
    Tundra,
    Snow,
    Forest,
    Jungle,
    Marsh,
    Lake,
    River
};

// Resource types
enum class ResourceType : std::uint8_t {
    None,
    Iron,
    Coal,
    Oil,
    Gold,
    Wheat,
    Cattle,
    Fish,
    Stone,
    Luxury,
    Strategic
};

// Improvement types
enum class ImprovementType : std::uint8_t {
    None,
    Farm,
    Mine,
    Quarry,
    Plantation,
    Camp,
    Pasture,
    FishingBoats,
    Road,
    Railroad,
    Fort
};

// Tile features (can have multiple)
enum class Feature : std::uint8_t {
    River,
    Forest,
    Jungle,
    Marsh,
    Oasis,
    Floodplains,
    NaturalWonder,
    Volcano,
    Reef,
    Ice
};

// Tile properties using bitset for memory efficiency
using Features = std::bitset<16>;

struct TileData {
    TileId id{TileId::invalid()};
    HexCoordinate coordinate{};
    TerrainType terrain{TerrainType::Ocean};
    ResourceType resource{ResourceType::None};
    ImprovementType improvement{ImprovementType::None};
    Features features{};

    // Ownership and control
    NationId owner{NationId::invalid()};
    CityId controlling_city{CityId::invalid()};

    // Yields
    Production food_yield{};
    Production production_yield{};
    Production science_yield{};
    Production culture_yield{};
    Production faith_yield{};
    Money trade_yield{};

    // Physical properties
    std::uint8_t elevation{};
    std::uint8_t rainfall{};
    std::uint8_t temperature{};
    std::uint8_t fertility{};

    // State flags
    bool explored{false};
    bool visible{false};
    bool passable{true};

    bool operator==(const TileData& other) const noexcept {
        return id == other.id && coordinate == other.coordinate && terrain == other.terrain &&
               resource == other.resource && improvement == other.improvement &&
               features == other.features && elevation == other.elevation &&
               rainfall == other.rainfall && temperature == other.temperature &&
               explored == other.explored && visible == other.visible && passable == other.passable;
    }

    bool operator!=(const TileData& other) const noexcept { return !(*this == other); }
};

class Tile {
    TileData data_;
    std::array<TileId, 6> neighbors_{};

   public:
    explicit Tile(TileId id, const HexCoordinate& coord) : data_{.id = id, .coordinate = coord} {
        neighbors_.fill(TileId::invalid());
        calculate_base_yields();
    }

    // Basic accessors
    TileId id() const noexcept { return data_.id; }
    const HexCoordinate& coordinate() const noexcept { return data_.coordinate; }

    // Terrain
    TerrainType terrain() const noexcept { return data_.terrain; }
    void set_terrain(TerrainType terrain) noexcept {
        data_.terrain = terrain;
        calculate_base_yields();
    }

    // Resources
    ResourceType resource() const noexcept { return data_.resource; }
    void set_resource(ResourceType resource) noexcept { data_.resource = resource; }

    // Improvements
    ImprovementType improvement() const noexcept { return data_.improvement; }
    void set_improvement(ImprovementType improvement) noexcept {
        data_.improvement = improvement;
        calculate_base_yields();
    }

    // Features
    const Features& features() const noexcept { return data_.features; }
    bool has_feature(Feature feature) const noexcept {
        return data_.features.test(static_cast<std::size_t>(feature));
    }
    void add_feature(Feature feature) noexcept {
        data_.features.set(static_cast<std::size_t>(feature));
        calculate_base_yields();
    }
    void remove_feature(Feature feature) noexcept {
        data_.features.reset(static_cast<std::size_t>(feature));
        calculate_base_yields();
    }

    // Ownership
    NationId owner() const noexcept { return data_.owner; }
    void set_owner(NationId owner) noexcept { data_.owner = owner; }

    CityId controlling_city() const noexcept { return data_.controlling_city; }
    void set_controlling_city(CityId city) noexcept { data_.controlling_city = city; }

    // Yields
    Production food_yield() const noexcept { return data_.food_yield; }
    Production production_yield() const noexcept { return data_.production_yield; }
    Production science_yield() const noexcept { return data_.science_yield; }
    Production culture_yield() const noexcept { return data_.culture_yield; }
    Production faith_yield() const noexcept { return data_.faith_yield; }
    Money trade_yield() const noexcept { return data_.trade_yield; }

    // Physical properties
    std::uint8_t elevation() const noexcept { return data_.elevation; }
    void set_elevation(std::uint8_t elevation) noexcept { data_.elevation = elevation; }

    std::uint8_t rainfall() const noexcept { return data_.rainfall; }
    void set_rainfall(std::uint8_t rainfall) noexcept { data_.rainfall = rainfall; }

    std::uint8_t temperature() const noexcept { return data_.temperature; }
    void set_temperature(std::uint8_t temperature) noexcept { data_.temperature = temperature; }

    std::uint8_t fertility() const noexcept { return data_.fertility; }
    void set_fertility(std::uint8_t fertility) noexcept { data_.fertility = fertility; }

    // Visibility and exploration
    bool is_explored() const noexcept { return data_.explored; }
    void set_explored(bool explored = true) noexcept { data_.explored = explored; }

    bool is_visible() const noexcept { return data_.visible; }
    void set_visible(bool visible = true) noexcept { data_.visible = visible; }

    bool is_passable() const noexcept { return data_.passable; }
    void set_passable(bool passable) noexcept { data_.passable = passable; }

    // Neighbors
    const std::array<TileId, 6>& neighbors() const noexcept { return neighbors_; }
    void set_neighbor(Direction direction, TileId neighbor) noexcept {
        neighbors_[static_cast<std::size_t>(direction)] = neighbor;
    }
    TileId neighbor(Direction direction) const noexcept {
        return neighbors_[static_cast<std::size_t>(direction)];
    }

    // Utility functions
    bool is_water() const noexcept {
        return data_.terrain == TerrainType::Ocean || data_.terrain == TerrainType::Coast ||
               data_.terrain == TerrainType::Lake;
    }

    bool is_land() const noexcept { return !is_water(); }

    bool can_build_improvement(ImprovementType improvement) const noexcept {
        // Basic rules - would be expanded with more complex logic
        switch (improvement) {
            case ImprovementType::Farm:
                return is_land() && data_.terrain != TerrainType::Mountains;
            case ImprovementType::Mine:
                return data_.terrain == TerrainType::Hills ||
                       data_.terrain == TerrainType::Mountains;
            case ImprovementType::FishingBoats:
                return is_water();
            default:
                return is_land();
        }
    }

    float movement_cost() const noexcept {
        float cost = 1.0f;

        switch (data_.terrain) {
            case TerrainType::Hills:
                cost = 2.0f;
                break;
            case TerrainType::Mountains:
                cost = 3.0f;
                break;
            case TerrainType::Forest:
                cost = 2.0f;
                break;
            case TerrainType::Jungle:
                cost = 2.0f;
                break;
            case TerrainType::Marsh:
                cost = 2.0f;
                break;
            default:
                break;
        }

        return cost;
    }

    // Serialization support
    const TileData& data() const noexcept { return data_; }
    void set_data(const TileData& data) noexcept {
        data_ = data;
        calculate_base_yields();
    }

   private:
    void calculate_base_yields() noexcept {
        // Reset yields
        data_.food_yield = Production{0.0};
        data_.production_yield = Production{0.0};
        data_.science_yield = Production{0.0};
        data_.culture_yield = Production{0.0};
        data_.faith_yield = Production{0.0};
        data_.trade_yield = Money{0.0};

        // Base terrain yields
        switch (data_.terrain) {
            case TerrainType::Grassland:
                data_.food_yield = Production{2.0};
                break;
            case TerrainType::Plains:
                data_.food_yield = Production{1.0};
                data_.production_yield = Production{1.0};
                break;
            case TerrainType::Hills:
                data_.production_yield = Production{2.0};
                break;
            case TerrainType::Mountains:
                data_.production_yield = Production{1.0};
                break;
            case TerrainType::Forest:
                data_.food_yield = Production{1.0};
                data_.production_yield = Production{1.0};
                break;
            case TerrainType::Ocean:
                data_.food_yield = Production{1.0};
                data_.trade_yield = Money{1.0};
                break;
            case TerrainType::Coast:
                data_.food_yield = Production{2.0};
                data_.trade_yield = Money{1.0};
                break;
            default:
                break;
        }

        // Resource bonuses
        if (data_.resource != ResourceType::None) {
            switch (data_.resource) {
                case ResourceType::Wheat:
                    data_.food_yield += Production{1.0};
                    break;
                case ResourceType::Iron:
                    data_.production_yield += Production{1.0};
                    break;
                case ResourceType::Gold:
                    data_.trade_yield += Money{2.0};
                    break;
                default:
                    break;
            }
        }

        // Improvement bonuses
        switch (data_.improvement) {
            case ImprovementType::Farm:
                data_.food_yield += Production{1.0};
                break;
            case ImprovementType::Mine:
                data_.production_yield += Production{1.0};
                break;
            case ImprovementType::Quarry:
                data_.production_yield += Production{1.0};
                break;
            default:
                break;
        }
    }
};

// Type traits for template constraints
template <typename T>
struct is_tile_like {
    static constexpr bool value = false;
};

template <>
struct is_tile_like<Tile> {
    static constexpr bool value = true;
};

}  // namespace Tiles
}  // namespace World
}  // namespace Manifest

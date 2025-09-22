#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Manifest {
namespace Core {
namespace Types {

// Strong typing for game IDs to prevent mixing different entity types
template <typename Tag, typename ValueType = std::uint32_t>
class StrongId {
    ValueType value_;

   public:
    using value_type = ValueType;
    using tag_type = Tag;

    // Allow implicit construction from default value, explicit otherwise
    constexpr StrongId() noexcept : value_{} {}
    constexpr explicit StrongId(ValueType value) noexcept : value_{value} {}

    constexpr ValueType value() const noexcept { return value_; }

    constexpr bool operator==(const StrongId& other) const noexcept {
        return value_ == other.value_;
    }
    constexpr bool operator!=(const StrongId& other) const noexcept {
        return value_ != other.value_;
    }
    constexpr bool operator<(const StrongId& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator<=(const StrongId& other) const noexcept {
        return value_ <= other.value_;
    }
    constexpr bool operator>(const StrongId& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator>=(const StrongId& other) const noexcept {
        return value_ >= other.value_;
    }

    constexpr StrongId& operator++() noexcept {
        ++value_;
        return *this;
    }
    constexpr StrongId operator++(int) noexcept {
        auto old = *this;
        ++value_;
        return old;
    }

    static constexpr StrongId invalid() noexcept {
        return StrongId{std::numeric_limits<ValueType>::max()};
    }

    constexpr bool is_valid() const noexcept {
        return value_ != std::numeric_limits<ValueType>::max();
    }
};

// Game entity ID types
struct TileTag {};
struct ProvinceTag {};
struct RegionTag {};
struct ContinentTag {};
struct CityTag {};
struct UnitTag {};
struct NationTag {};
struct PlayerTag {};
struct BuildingTag {};
struct ResourceTag {};
struct PlateTag {};

using TileId = StrongId<TileTag, std::uint32_t>;
using ProvinceId = StrongId<ProvinceTag, std::uint32_t>;
using RegionId = StrongId<RegionTag, std::uint32_t>;
using ContinentId = StrongId<ContinentTag, std::uint32_t>;
using CityId = StrongId<CityTag, std::uint32_t>;
using UnitId = StrongId<UnitTag, std::uint32_t>;
using NationId = StrongId<NationTag, std::uint32_t>;
using PlayerId = StrongId<PlayerTag, std::uint32_t>;
using BuildingId = StrongId<BuildingTag, std::uint32_t>;
using ResourceId = StrongId<ResourceTag, std::uint32_t>;
using PlateId = StrongId<PlateTag, std::uint32_t>;

// Numeric types with specific semantics
template <typename Tag, typename T = double>
class Quantity {
    static_assert(std::is_floating_point<T>::value, "T must be a floating point type");

    T value_;

   public:
    using value_type = T;
    using tag_type = Tag;

    constexpr explicit Quantity(T value = {}) noexcept : value_{value} {}

    constexpr T value() const noexcept { return value_; }

    constexpr bool operator==(const Quantity& other) const noexcept {
        return value_ == other.value_;
    }
    constexpr bool operator!=(const Quantity& other) const noexcept {
        return value_ != other.value_;
    }
    constexpr bool operator<(const Quantity& other) const noexcept { return value_ < other.value_; }
    constexpr bool operator<=(const Quantity& other) const noexcept {
        return value_ <= other.value_;
    }
    constexpr bool operator>(const Quantity& other) const noexcept { return value_ > other.value_; }
    constexpr bool operator>=(const Quantity& other) const noexcept {
        return value_ >= other.value_;
    }

    constexpr Quantity operator+(const Quantity& other) const noexcept {
        return Quantity{value_ + other.value_};
    }

    constexpr Quantity operator-(const Quantity& other) const noexcept {
        return Quantity{value_ - other.value_};
    }

    constexpr Quantity operator*(T scalar) const noexcept { return Quantity{value_ * scalar}; }

    constexpr Quantity operator/(T scalar) const noexcept { return Quantity{value_ / scalar}; }

    constexpr Quantity& operator+=(const Quantity& other) noexcept {
        value_ += other.value_;
        return *this;
    }

    constexpr Quantity& operator-=(const Quantity& other) noexcept {
        value_ -= other.value_;
        return *this;
    }

    constexpr Quantity& operator*=(T scalar) noexcept {
        value_ *= scalar;
        return *this;
    }

    constexpr Quantity& operator/=(T scalar) noexcept {
        value_ /= scalar;
        return *this;
    }
};

// Economic types
struct MoneyTag {};
struct PopulationTag {};
struct ProductionTag {};
struct ScienceTag {};
struct CultureTag {};
struct FaithTag {};
struct InfluenceTag {};

using Money = Quantity<MoneyTag>;
using Population = Quantity<PopulationTag>;
using Production = Quantity<ProductionTag>;
using Science = Quantity<ScienceTag>;
using Culture = Quantity<CultureTag>;
using Faith = Quantity<FaithTag>;
using Influence = Quantity<InfluenceTag>;

// Position and coordinate types
struct Coordinate {
    std::int32_t x{}, y{}, z{};

    constexpr bool operator==(const Coordinate& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }
    constexpr bool operator!=(const Coordinate& other) const noexcept { return !(*this == other); }
};

struct HexCoordinate {
    std::int32_t q{}, r{}, s{};

    constexpr bool operator==(const HexCoordinate& other) const noexcept {
        return q == other.q && r == other.r && s == other.s;
    }
    constexpr bool operator!=(const HexCoordinate& other) const noexcept {
        return !(*this == other);
    }

    // Hex coordinate constraint: q + r + s = 0
    constexpr bool is_valid() const noexcept { return q + r + s == 0; }
};

// Time types
using GameTurn = std::uint32_t;
using GameYear = std::int32_t;
using Timestamp = std::uint64_t;

// Common enums
enum class Layer : std::uint8_t { Underground, Surface, Air, Space };

enum class Direction : std::uint8_t { North, Northeast, Southeast, South, Southwest, Northwest };

// Bit manipulation utilities
template <typename T>
constexpr T set_bit(T value, std::uint8_t bit) noexcept {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integral type");
    return value | (T{1} << bit);
}

template <typename T>
constexpr T clear_bit(T value, std::uint8_t bit) noexcept {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integral type");
    return value & ~(T{1} << bit);
}

template <typename T>
constexpr bool test_bit(T value, std::uint8_t bit) noexcept {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integral type");
    return (value & (T{1} << bit)) != 0;
}

template <typename T>
constexpr T toggle_bit(T value, std::uint8_t bit) noexcept {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned integral type");
    return value ^ (T{1} << bit);
}

}  // namespace Types
}  // namespace Core
}  // namespace Manifest

// Hash function for StrongId
namespace std {
template <typename Tag, typename ValueType>
struct hash<Manifest::Core::Types::StrongId<Tag, ValueType>> {
    std::size_t operator()(const Manifest::Core::Types::StrongId<Tag, ValueType>& id) const noexcept {
        return std::hash<ValueType>{}(id.value());
    }
};
}  // namespace std
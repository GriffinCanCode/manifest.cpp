#pragma once

#include "../../core/types/Modern.hpp"

namespace Manifest::Render::CameraSystem {

// Helper functions to avoid Result constructor ambiguity
template<typename T, typename E>
auto success(const T& value) {
    return Core::Modern::Result<T, E>{value};
}

template<typename T, typename E>
auto success(T&& value) {
    return Core::Modern::Result<std::decay_t<T>, E>{std::forward<T>(value)};
}

template<typename T, typename E>
auto success() {
    return Core::Modern::Result<T, E>{};
}

template<typename T, typename E>
auto error(E&& error_value) {
    // Create a Result in error state by constructing with error type
    Core::Modern::Result<T, E> result{std::forward<E>(error_value)};
    return result;
}

} // namespace Manifest::Render::CameraSystem

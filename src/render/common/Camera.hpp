#pragma once

// COMPATIBILITY HEADER: Camera class moved to core/graphics/Camera.hpp
// This header provides backward compatibility during the transition
#include "../../core/graphics/Camera.hpp"

namespace Manifest::Render {
    // Maintain backward compatibility with the old namespace
    using Camera = Core::Graphics::Camera;
}

#pragma once

// Simple compatibility layer for OpenGL module
// Just use the existing types from the common renderer interface

#include "../../core/types/Modern.hpp"

namespace Manifest {
namespace Render {
namespace OpenGL {

// Alias common types to avoid namespace issues
using ::Manifest::Core::Modern::byte;

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest
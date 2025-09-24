//==============================================================================
// MANIFEST ENGINE RENDERER FACTORY - OPENGL ONLY
//==============================================================================
// 
// CRITICAL: This factory was converted from Vulkan to OpenGL during migration
// 
// ✅ WORKING CONFIGURATION:
// - OpenGL 4.1 Core Profile (macOS compatible)
// - Simplified vertex attribute setup
// - Direct NDC coordinate mapping in shaders
// - Instance-based rendering for hex world
//
// ⚠️  DO NOT attempt to re-add Vulkan support without understanding the
//    complex integration requirements that caused the original breakage!
//==============================================================================

#include "common/Renderer.hpp"
#include "opengl/OpenGL.hpp"
#include "../ui/window/Surface.hpp"

namespace Manifest::Render {

std::unique_ptr<Renderer> create_renderer(API api) {
    //==========================================================================
    // DEFENSIVE: Always return OpenGL renderer regardless of requested API
    //==========================================================================
    // This prevents accidental Vulkan requests from breaking the application
    // All API types now safely fall back to the working OpenGL implementation
    //==========================================================================
    
    switch (api) {
        case API::OpenGL:
            // Primary OpenGL path - this is our main working renderer
            return std::make_unique<OpenGL::Core>();
        case API::Vulkan:
            // DEFENSIVE: Vulkan support permanently removed - always fall back to OpenGL
            // This prevents crashes if old code still requests Vulkan
            return std::make_unique<OpenGL::Core>();
        default:
            // DEFENSIVE: Unknown APIs default to working OpenGL implementation
            return std::make_unique<OpenGL::Core>();
    }
}

}  // namespace Manifest::Render

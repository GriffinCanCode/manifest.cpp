#pragma once

#include "../../core/types/Modern.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"
#include "../common/Renderer.hpp"
#include "../common/Types.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

using namespace Core::Math;

// Forward declarations
class RenderPass;
struct PassContext;

// Result template using modern compatibility layer
template <typename T>
using Result = Core::Modern::Result<T, RendererError>;

/**
 * Base class for all render passes in the multi-pass rendering pipeline.
 * Follows modern 2025 design principles: RAII, modularity, and minimal overhead.
 */
class RenderPass {
   public:
    virtual ~RenderPass() = default;

    // Non-copyable, movable
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = default;
    RenderPass& operator=(RenderPass&&) = default;

    /**
     * Initialize the render pass with the given renderer
     */
    [[nodiscard]] virtual Result<void> initialize(Renderer* renderer) = 0;

    /**
     * Shutdown and cleanup resources
     */
    virtual void shutdown() = 0;

    /**
     * Execute the render pass
     */
    [[nodiscard]] virtual Result<void> execute(const PassContext& context) = 0;

    /**
     * Get the pass name for debugging
     */
    [[nodiscard]] virtual std::string_view get_name() const noexcept = 0;

    /**
     * Check if pass is initialized and ready
     */
    [[nodiscard]] bool is_initialized() const noexcept { return is_initialized_; }

    /**
     * Get execution order priority (lower = earlier)
     */
    [[nodiscard]] virtual std::uint32_t get_priority() const noexcept { return 1000; }

   protected:
    RenderPass() = default;

    Renderer* renderer_{nullptr};
    bool is_initialized_{false};
};

/**
 * Context passed to each render pass containing shared frame data
 */
struct PassContext {
    // Camera and view data
    Mat4f view_matrix{};
    Mat4f projection_matrix{};
    Mat4f view_projection_matrix{};
    Vec3f camera_position{};
    Vec3f camera_direction{};

    // Lighting data
    Vec3f sun_direction{0.0f, -1.0f, 0.0f};
    Vec3f sun_color{1.0f, 0.95f, 0.8f};
    Vec3f ambient_color{0.2f, 0.3f, 0.5f};
    float sun_intensity{1.0f};

    // Timing
    float delta_time{0.0f};
    float elapsed_time{0.0f};
    std::uint32_t frame_number{0};

    // Viewport
    Viewport viewport{};

    // Previous frame data for temporal effects
    Mat4f prev_view_projection_matrix{};
    Vec3f prev_camera_position{};

    // Output targets (set by render graph)
    RenderTargetHandle main_target{};
    RenderTargetHandle depth_target{};
    RenderTargetHandle shadow_target{};
};

/**
 * Pass execution result with performance metrics
 */
struct PassResult {
    std::uint32_t draw_calls{0};
    std::uint32_t vertices_rendered{0};
    float gpu_time_ms{0.0f};
    std::string debug_info{};
};

/**
 * Factory function for creating render passes
 */
enum class PassType : std::uint8_t { Shadow, Main, PostProcess };

[[nodiscard]] std::unique_ptr<RenderPass> create_render_pass(PassType type);

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest

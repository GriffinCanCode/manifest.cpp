#pragma once

#include "../passes/RenderGraph.hpp"
#include "../passes/ShadowPass.hpp" 
#include "../passes/MainPass.hpp"
#include "../passes/PostProcessPass.hpp"
#include "../common/Renderer.hpp"
#include "../../core/math/Matrix.hpp"
#include "../../core/math/Vector.hpp"
#include <memory>

namespace Manifest {
namespace Render {
namespace Examples {

using namespace Core::Math;
using namespace Passes;

/**
 * Complete multi-pass rendering demo showcasing the new 2025 render pipeline.
 * Demonstrates shadow mapping, main scene rendering, and post-processing.
 */
class MultiPassDemo {
public:
    MultiPassDemo() = default;
    ~MultiPassDemo() = default;
    
    // Non-copyable, movable
    MultiPassDemo(const MultiPassDemo&) = delete;
    MultiPassDemo& operator=(const MultiPassDemo&) = delete;
    MultiPassDemo(MultiPassDemo&&) = default;
    MultiPassDemo& operator=(MultiPassDemo&&) = default;
    
    /**
     * Initialize the multi-pass demo with a renderer
     */
    [[nodiscard]] Result<void> initialize(std::unique_ptr<Renderer> renderer);
    
    /**
     * Shutdown and cleanup
     */
    void shutdown();
    
    /**
     * Render a frame using the multi-pass pipeline
     */
    [[nodiscard]] Result<void> render_frame(float delta_time);
    
    /**
     * Update camera parameters
     */
    void set_camera(const Vec3f& position, const Vec3f& target, const Vec3f& up);
    
    /**
     * Update lighting parameters
     */  
    void set_lighting(const Vec3f& sun_direction, const Vec3f& sun_color, float intensity = 1.0f);
    
    /**
     * Enable or disable specific post-processing effects
     */
    void set_post_process_effect(PostProcessPass::Effect effect, bool enabled);
    
    /**
     * Check if demo is initialized
     */
    [[nodiscard]] bool is_initialized() const noexcept { return render_graph_.is_initialized(); }
    
    /**
     * Get frame statistics
     */
    [[nodiscard]] const RenderGraph::FrameStats& get_frame_stats() const noexcept {
        return render_graph_.get_frame_stats();
    }

private:
    // Core components
    std::unique_ptr<Renderer> renderer_;
    RenderGraph render_graph_;
    
    // Pass references (owned by render graph)
    ShadowPass* shadow_pass_{nullptr};
    MainPass* main_pass_{nullptr};
    PostProcessPass* post_process_pass_{nullptr};
    
    // Demo state
    Mat4f view_matrix_{Mat4f::identity()};
    Mat4f projection_matrix_{Mat4f::perspective(45.0f, 16.0f/9.0f, 1.0f, 1000.0f)};
    Vec3f camera_position_{0.0f, 10.0f, 10.0f};
    Vec3f camera_target_{0.0f, 0.0f, 0.0f};
    Vec3f camera_up_{0.0f, 1.0f, 0.0f};
    
    Vec3f sun_direction_{-0.3f, -0.7f, -0.6f};
    Vec3f sun_color_{1.0f, 0.95f, 0.8f};
    Vec3f ambient_color_{0.2f, 0.3f, 0.5f};
    float sun_intensity_{1.0f};
    
    float elapsed_time_{0.0f};
    std::uint32_t frame_number_{0};
    
    [[nodiscard]] Result<void> setup_render_passes();
    PassContext create_pass_context(float delta_time);
    void update_matrices();
};

} // namespace Examples
} // namespace Render
} // namespace Manifest

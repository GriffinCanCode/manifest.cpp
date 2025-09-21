#include "MultiPassDemo.hpp"
#include "../common/Renderer.hpp"
#include <cmath>

namespace Manifest {
namespace Render {
namespace Examples {

Result<void> MultiPassDemo::initialize(std::unique_ptr<Renderer> renderer) {
    if (!renderer) {
        return std::unexpected(RendererError::InvalidState);
    }
    
    renderer_ = std::move(renderer);
    
    // Initialize render graph
    if (auto result = render_graph_.initialize(renderer_.get()); !result) {
        return result;
    }
    
    // Setup render passes
    return setup_render_passes();
}

void MultiPassDemo::shutdown() {
    render_graph_.shutdown();
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
    
    shadow_pass_ = nullptr;
    main_pass_ = nullptr;
    post_process_pass_ = nullptr;
}

Result<void> MultiPassDemo::render_frame(float delta_time) {
    if (!is_initialized()) {
        return std::unexpected(RendererError::InvalidState);
    }
    
    elapsed_time_ += delta_time;
    frame_number_++;
    
    // Update camera matrices
    update_matrices();
    
    // Create pass context for this frame
    PassContext context = create_pass_context(delta_time);
    
    // Execute render graph
    return render_graph_.execute(context);
}

void MultiPassDemo::set_camera(const Vec3f& position, const Vec3f& target, const Vec3f& up) {
    camera_position_ = position;
    camera_target_ = target;
    camera_up_ = up;
    update_matrices();
}

void MultiPassDemo::set_lighting(const Vec3f& sun_direction, const Vec3f& sun_color, float intensity) {
    sun_direction_ = sun_direction.normalize();
    sun_color_ = sun_color;
    sun_intensity_ = intensity;
}

void MultiPassDemo::set_post_process_effect(PostProcessPass::Effect effect, bool enabled) {
    if (post_process_pass_) {
        post_process_pass_->set_effect_enabled(effect, enabled);
    }
}

Result<void> MultiPassDemo::setup_render_passes() {
    // Add shadow pass
    shadow_pass_ = render_graph_.add_pass<ShadowPass>();
    if (!shadow_pass_) {
        return std::unexpected(RendererError::InitializationFailed);
    }
    
    // Add main pass
    main_pass_ = render_graph_.add_pass<MainPass>();
    if (!main_pass_) {
        return std::unexpected(RendererError::InitializationFailed);
    }
    
    // Link main pass to shadow pass for shadow mapping
    main_pass_->set_shadow_pass(shadow_pass_);
    
    // Add post-process pass
    post_process_pass_ = render_graph_.add_pass<PostProcessPass>();
    if (!post_process_pass_) {
        return std::unexpected(RendererError::InitializationFailed);
    }
    
    // Configure post-processing effects
    post_process_pass_->set_effect_enabled(PostProcessPass::Effect::TAA, true);
    post_process_pass_->set_effect_enabled(PostProcessPass::Effect::ToneMapping, true);
    post_process_pass_->set_taa_parameters(0.9f, 0.1f);
    post_process_pass_->set_tone_mapping(1.0f, 2.2f);
    
    return {};
}

PassContext MultiPassDemo::create_pass_context(float delta_time) {
    PassContext context{};
    
    // Camera data
    context.view_matrix = view_matrix_;
    context.projection_matrix = projection_matrix_;
    context.view_projection_matrix = projection_matrix_ * view_matrix_;
    context.camera_position = camera_position_;
    context.camera_direction = (camera_target_ - camera_position_).normalize();
    
    // Lighting data
    context.sun_direction = sun_direction_;
    context.sun_color = sun_color_;
    context.ambient_color = ambient_color_;
    context.sun_intensity = sun_intensity_;
    
    // Timing data
    context.delta_time = delta_time;
    context.elapsed_time = elapsed_time_;
    context.frame_number = frame_number_;
    
    // Viewport (would be set from actual window size)
    context.viewport = Viewport{
        .position = {0.0f, 0.0f},
        .size = {1920.0f, 1080.0f},
        .depth_range = {0.0f, 1.0f}
    };
    
    // Previous frame data for temporal effects (simplified)
    static Mat4f prev_vp = context.view_projection_matrix;
    static Vec3f prev_pos = context.camera_position;
    
    context.prev_view_projection_matrix = prev_vp;
    context.prev_camera_position = prev_pos;
    
    prev_vp = context.view_projection_matrix;
    prev_pos = context.camera_position;
    
    return context;
}

void MultiPassDemo::update_matrices() {
    view_matrix_ = Mat4f::look_at(camera_position_, camera_target_, camera_up_);
}

} // namespace Examples
} // namespace Render  
} // namespace Manifest

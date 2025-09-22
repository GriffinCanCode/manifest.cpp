#include "PostProcessPass.hpp"

#include <cstring>
#include <span>

#include "../common/Renderer.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

Result<void> PostProcessPass::initialize(Renderer* renderer) {
    if (!renderer) {
        return RendererError::InvalidState;
    }

    renderer_ = renderer;

    // Create post-process resources
    if (auto result = create_postprocess_resources(); !result) {
        return result;
    }

    if (auto result = create_postprocess_shaders(); !result) {
        return result;
    }

    if (auto result = create_postprocess_pipeline(); !result) {
        return result;
    }

    is_initialized_ = true;
    return {};
}

void PostProcessPass::shutdown() {
    if (!renderer_) return;

    if (postprocess_pipeline_.is_valid()) {
        renderer_->destroy_pipeline(postprocess_pipeline_);
    }
    if (postprocess_fragment_shader_.is_valid()) {
        renderer_->destroy_shader(postprocess_fragment_shader_);
    }
    if (fullscreen_vertex_shader_.is_valid()) {
        renderer_->destroy_shader(fullscreen_vertex_shader_);
    }
    if (postprocess_uniforms_buffer_.is_valid()) {
        renderer_->destroy_buffer(postprocess_uniforms_buffer_);
    }
    if (temp_render_target_.is_valid()) {
        renderer_->destroy_render_target(temp_render_target_);
    }
    if (history_texture_.is_valid()) {
        renderer_->destroy_texture(history_texture_);
    }
    if (temp_texture_.is_valid()) {
        renderer_->destroy_texture(temp_texture_);
    }

    is_initialized_ = false;
}

Result<void> PostProcessPass::execute(const PassContext& context) {
    if (!is_initialized_) {
        return RendererError::InvalidState;
    }

    // Update uniforms for this frame
    update_uniforms(context);

    // Upload uniforms to GPU
    if (auto result = upload_uniforms(); !result) {
        return result;
    }

    // Begin post-process pass
    renderer_->begin_render_pass(temp_render_target_, Vec4f{0.0f, 0.0f, 0.0f, 0.0f});

    // Set viewport
    renderer_->set_viewport(context.viewport);

    // Bind post-process pipeline
    renderer_->bind_pipeline(postprocess_pipeline_);
    renderer_->bind_uniform_buffer(postprocess_uniforms_buffer_, 0);

    // Bind input textures (main scene output and history for TAA)
    if (context.main_target.is_valid()) {
        // In a complete implementation, we'd get the color attachment from main_target
        // For now, this is a placeholder
    }

    if (history_texture_.is_valid()) {
        renderer_->bind_texture(history_texture_, 1);
    }

    // Draw fullscreen quad (no vertex buffer needed - generated in vertex shader)
    DrawCommand draw_cmd{.vertex_count = 3,  // Triangle trick for fullscreen quad
                         .instance_count = 1,
                         .first_vertex = 0,
                         .first_instance = 0};

    renderer_->draw(draw_cmd);

    renderer_->end_render_pass();

    // Update frame index for TAA
    frame_index_++;

    return {};
}

Result<void> PostProcessPass::create_postprocess_resources() {
    // Create temporary render texture (would match main render target resolution)
    TextureDesc temp_tex_desc{.width = 1920,  // Would get from main render target
                              .height = 1080,
                              .depth = 1,
                              .mip_levels = 1,
                              .array_layers = 1,
                              .format = TextureFormat::RGBA16_FLOAT,
                              .render_target = true,
                              .debug_name = "postprocess_temp"};

    auto temp_tex_result = renderer_->create_texture(temp_tex_desc);
    if (!temp_tex_result) {
        return temp_tex_result.error();
    }
    temp_texture_ = *temp_tex_result;

    // Create history texture for TAA
    TextureDesc history_tex_desc = temp_tex_desc;
    history_tex_desc.debug_name = "taa_history";

    auto history_tex_result = renderer_->create_texture(history_tex_desc);
    if (!history_tex_result) {
        return history_tex_result.error();
    }
    history_texture_ = *history_tex_result;

    // Create render target
    std::array<TextureHandle, 1> color_attachments{temp_texture_};
    auto rt_result = renderer_->create_render_target(color_attachments, {});
    if (!rt_result) {
        return rt_result.error();
    }
    temp_render_target_ = *rt_result;

    // Create uniforms buffer
    BufferDesc uniforms_desc{.size = sizeof(PostProcessUniforms),
                             .usage = BufferUsage::Uniform,
                             .host_visible = true,
                             .debug_name = "postprocess_uniforms"};

    auto buffer_result = renderer_->create_buffer(uniforms_desc);
    if (!buffer_result) {
        return buffer_result.error();
    }
    postprocess_uniforms_buffer_ = *buffer_result;

    return {};
}

Result<void> PostProcessPass::create_postprocess_shaders() {
    // Fullscreen triangle vertex shader
    std::string vert_source = R"(
#version 460 core

out vec2 v_texcoord;

void main() {
    // Fullscreen triangle trick
    v_texcoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(v_texcoord * 2.0 - 1.0, 0.0, 1.0);
}
)";

    // Post-process fragment shader with TAA and tone mapping
    std::string frag_source = R"(
#version 460 core

layout(binding = 0) uniform PostProcessUniforms {
    vec2 screen_size;
    vec2 texel_size;
    
    mat4 prev_view_projection_matrix;
    mat4 current_view_projection_matrix;
    
    vec2 jitter_offset;
    float taa_blend_factor;
    float taa_threshold;
    
    float exposure;
    float gamma;
    vec2 _padding1;
    
    vec3 color_balance;
    float saturation;
    
    float contrast;
    float brightness;
    uint enabled_effects;
    float _padding2;
} u_postprocess;

layout(binding = 0) uniform sampler2D u_main_scene;
layout(binding = 1) uniform sampler2D u_history;

in vec2 v_texcoord;
out vec4 frag_color;

const uint EFFECT_TAA = 1u;
const uint EFFECT_TONE_MAPPING = 2u;
const uint EFFECT_COLOR_GRADING = 4u;

vec3 tone_map_aces(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

vec3 apply_color_grading(vec3 color) {
    // Apply color balance
    color *= u_postprocess.color_balance;
    
    // Apply brightness and contrast
    color = (color - 0.5) * u_postprocess.contrast + 0.5 + u_postprocess.brightness;
    
    // Apply saturation
    float luminance = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luminance), color, u_postprocess.saturation);
    
    return color;
}

void main() {
    vec3 current_color = texture(u_main_scene, v_texcoord).rgb;
    
    // Apply TAA if enabled
    if ((u_postprocess.enabled_effects & EFFECT_TAA) != 0u) {
        vec3 history_color = texture(u_history, v_texcoord).rgb;
        
        // Simple TAA blend - a full implementation would do motion vector-based reprojection
        float blend = u_postprocess.taa_blend_factor;
        current_color = mix(current_color, history_color, blend);
    }
    
    // Apply exposure
    current_color *= u_postprocess.exposure;
    
    // Apply tone mapping if enabled
    if ((u_postprocess.enabled_effects & EFFECT_TONE_MAPPING) != 0u) {
        current_color = tone_map_aces(current_color);
    }
    
    // Apply color grading if enabled
    if ((u_postprocess.enabled_effects & EFFECT_COLOR_GRADING) != 0u) {
        current_color = apply_color_grading(current_color);
    }
    
    // Apply gamma correction
    current_color = pow(current_color, vec3(1.0 / u_postprocess.gamma));
    
    frag_color = vec4(current_color, 1.0);
}
)";

    // Create vertex shader
    const std::byte* vert_ptr = reinterpret_cast<const std::byte*>(vert_source.data());
    ShaderDesc vert_desc{.stage = ShaderStage::Vertex,
                         .bytecode = std::span<const std::byte>{vert_ptr, vert_source.size()},
                         .entry_point = "main",
                         .debug_name = "fullscreen_vertex"};

    auto vert_result = renderer_->create_shader(vert_desc);
    if (!vert_result) {
        return vert_result.error();
    }
    fullscreen_vertex_shader_ = *vert_result;

    // Create fragment shader
    const std::byte* frag_ptr = reinterpret_cast<const std::byte*>(frag_source.data());
    ShaderDesc frag_desc{.stage = ShaderStage::Fragment,
                         .bytecode = std::span<const std::byte>{frag_ptr, frag_source.size()},
                         .entry_point = "main",
                         .debug_name = "postprocess_fragment"};

    auto frag_result = renderer_->create_shader(frag_desc);
    if (!frag_result) {
        return frag_result.error();
    }
    postprocess_fragment_shader_ = *frag_result;

    return {};
}

Result<void> PostProcessPass::create_postprocess_pipeline() {
    // No vertex attributes needed for fullscreen pass
    static const ShaderHandle shaders[] = {fullscreen_vertex_shader_, postprocess_fragment_shader_};

    PipelineDesc pipeline_desc{
        .shaders = shaders,
        .vertex_bindings = {},  // No vertex buffers
        .render_state = RenderState{.topology = PrimitiveTopology::Triangles,
                                    .blend_mode = BlendMode::None,
                                    .cull_mode = CullMode::None,  // No culling for fullscreen
                                    .depth_test = DepthTest::Always,
                                    .depth_write = false,
                                    .wireframe = false},
        .render_target = temp_render_target_,
        .debug_name = "postprocess_pipeline"};

    auto pipeline_result = renderer_->create_pipeline(pipeline_desc);
    if (!pipeline_result) {
        return pipeline_result.error();
    }
    postprocess_pipeline_ = *pipeline_result;

    return {};
}

Vec2f PostProcessPass::get_taa_jitter() const {
    if ((enabled_effects_ & static_cast<std::uint32_t>(Effect::TAA)) == 0) {
        return Vec2f{0.0f, 0.0f};
    }

    return TAA_JITTER_PATTERN[frame_index_ % TAA_JITTER_PATTERN.size()];
}

void PostProcessPass::update_uniforms(const PassContext& context) {
    uniforms_.screen_size = Vec2f{context.viewport.size.x(), context.viewport.size.y()};
    uniforms_.texel_size =
        Vec2f{1.0f / context.viewport.size.x(), 1.0f / context.viewport.size.y()};

    uniforms_.prev_view_projection_matrix = context.prev_view_projection_matrix;
    uniforms_.current_view_projection_matrix = context.view_projection_matrix;

    uniforms_.jitter_offset = get_taa_jitter();
    uniforms_.enabled_effects = enabled_effects_;
}

Result<void> PostProcessPass::upload_uniforms() {
    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(&uniforms_);
    std::span<const std::byte> data{data_ptr, sizeof(PostProcessUniforms)};
    return renderer_->update_buffer(postprocess_uniforms_buffer_, 0, data);
}

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest

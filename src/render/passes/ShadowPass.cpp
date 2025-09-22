#include "ShadowPass.hpp"

#include <algorithm>
#include <cstddef>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <span>

#include "../common/Renderer.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

Result<void> ShadowPass::initialize(Renderer* renderer) {
    if (!renderer) {
        return RendererError::InvalidState;
    }

    renderer_ = renderer;

    // Create shadow mapping resources
    if (auto result = create_shadow_resources(); !result) {
        return result;
    }

    if (auto result = create_shadow_shaders(); !result) {
        return result;
    }

    if (auto result = create_shadow_pipeline(); !result) {
        return result;
    }

    is_initialized_ = true;
    return {};
}

void ShadowPass::shutdown() {
    if (!renderer_) return;

    if (shadow_pipeline_.is_valid()) {
        renderer_->destroy_pipeline(shadow_pipeline_);
    }
    if (shadow_fragment_shader_.is_valid()) {
        renderer_->destroy_shader(shadow_fragment_shader_);
    }
    if (shadow_vertex_shader_.is_valid()) {
        renderer_->destroy_shader(shadow_vertex_shader_);
    }
    if (shadow_uniforms_buffer_.is_valid()) {
        renderer_->destroy_buffer(shadow_uniforms_buffer_);
    }
    if (shadow_render_target_.is_valid()) {
        renderer_->destroy_render_target(shadow_render_target_);
    }
    if (shadow_texture_.is_valid()) {
        renderer_->destroy_texture(shadow_texture_);
    }

    is_initialized_ = false;
}

Result<void> ShadowPass::execute(const PassContext& context) {
    if (!is_initialized_) {
        return RendererError::InvalidState;
    }

    // Calculate cascade matrices based on camera frustum
    calculate_cascade_matrices(context);

    // Upload shadow uniforms
    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(&shadow_uniforms_);
    std::span<const std::byte> data{data_ptr, sizeof(ShadowUniforms)};
    if (auto result = renderer_->update_buffer(shadow_uniforms_buffer_, 0, data); !result) {
        return result;
    }

    // Begin shadow pass
    renderer_->begin_render_pass(shadow_render_target_, Vec4f{1.0f, 1.0f, 1.0f, 1.0f});

    // Set viewport for shadow map
    Viewport shadow_viewport{
        .position = {0.0f, 0.0f},
        .size = {static_cast<float>(SHADOW_MAP_SIZE), static_cast<float>(SHADOW_MAP_SIZE)},
        .depth_range = {0.0f, 1.0f}};
    renderer_->set_viewport(shadow_viewport);

    // Bind shadow pipeline
    renderer_->bind_pipeline(shadow_pipeline_);
    renderer_->bind_uniform_buffer(shadow_uniforms_buffer_, 0);

    // Render shadow casters here
    // Note: In a complete implementation, this would render all shadow-casting geometry
    // For now, this is a placeholder that sets up the shadow mapping infrastructure

    renderer_->end_render_pass();

    return {};
}

Result<void> ShadowPass::create_shadow_resources() {
    // Create shadow map texture
    TextureDesc shadow_tex_desc{.width = SHADOW_MAP_SIZE,
                                .height = SHADOW_MAP_SIZE,
                                .depth = CASCADE_COUNT,  // Array texture for cascades
                                .mip_levels = 1,
                                .array_layers = 1,
                                .format = TextureFormat::D32_FLOAT,
                                .render_target = true,
                                .debug_name = "shadow_map"};

    auto tex_result = renderer_->create_texture(shadow_tex_desc);
    if (!tex_result) {
        return tex_result.error();
    }
    shadow_texture_ = *tex_result;

    // Create shadow render target
    std::array<TextureHandle, 0> color_attachments{};  // No color attachments for depth-only pass
    auto rt_result = renderer_->create_render_target(color_attachments, shadow_texture_);
    if (!rt_result) {
        return rt_result.error();
    }
    shadow_render_target_ = *rt_result;

    // Create shadow uniforms buffer
    BufferDesc uniforms_desc{.size = sizeof(ShadowUniforms),
                             .usage = BufferUsage::Uniform,
                             .host_visible = true,
                             .debug_name = "shadow_uniforms"};

    auto buffer_result = renderer_->create_buffer(uniforms_desc);
    if (!buffer_result) {
        return buffer_result.error();
    }
    shadow_uniforms_buffer_ = *buffer_result;

    return {};
}

Result<void> ShadowPass::create_shadow_shaders() {
    // Load shadow vertex shader
    Core::Modern::fs::path shader_dir = "assets/shaders";

    // Simple depth-only vertex shader
    std::string vert_source = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float elevation;
layout(location = 3) in uint terrain;

layout(binding = 0) uniform ShadowUniforms {
    mat4 cascade_matrices[4];
    vec4 cascade_splits;
    vec3 light_direction;
    float shadow_bias;
} u_shadow;

void main() {
    // Generate hex vertices procedurally (simplified for shadow pass)
    vec3 hex_vertices[7] = vec3[](
        vec3(0.0, 0.0, 0.0),  // Center
        vec3(1.0, 0.0, 0.0), vec3(0.5, 0.0, 0.866), vec3(-0.5, 0.0, 0.866),
        vec3(-1.0, 0.0, 0.0), vec3(-0.5, 0.0, -0.866), vec3(0.5, 0.0, -0.866)
    );
    
    vec3 local_pos = hex_vertices[gl_VertexID];
    vec3 world_pos = position + local_pos;
    world_pos.y += elevation * 10.0; // Scale elevation
    
    // Use first cascade for now (full implementation would select appropriate cascade)
    gl_Position = u_shadow.cascade_matrices[0] * vec4(world_pos, 1.0);
}
)";

    // Simple depth-only fragment shader
    std::string frag_source = R"(
#version 460 core

void main() {
    // Depth is written automatically
}
)";

    // Create vertex shader
    const std::byte* vert_ptr = reinterpret_cast<const std::byte*>(vert_source.data());
    ShaderDesc vert_desc{.stage = ShaderStage::Vertex,
                         .bytecode = std::span<const std::byte>{vert_ptr, vert_source.size()},
                         .entry_point = "main",
                         .debug_name = "shadow_vertex"};

    auto vert_result = renderer_->create_shader(vert_desc);
    if (!vert_result) {
        return vert_result.error();
    }
    shadow_vertex_shader_ = *vert_result;

    // Create fragment shader
    const std::byte* frag_ptr = reinterpret_cast<const std::byte*>(frag_source.data());
    ShaderDesc frag_desc{.stage = ShaderStage::Fragment,
                         .bytecode = std::span<const std::byte>{frag_ptr, frag_source.size()},
                         .entry_point = "main",
                         .debug_name = "shadow_fragment"};

    auto frag_result = renderer_->create_shader(frag_desc);
    if (!frag_result) {
        return frag_result.error();
    }
    shadow_fragment_shader_ = *frag_result;

    return {};
}

Result<void> ShadowPass::create_shadow_pipeline() {
    // Define vertex attributes for shadow pass (same as hex renderer)
    static const VertexAttribute shadow_attributes[] = {
        {.location = 0, .format = AttributeFormat::Float3, .offset = 0, .name = "position"},
        {.location = 1,
         .format = AttributeFormat::Float4,
         .offset = sizeof(Vec3f),
         .name = "color"},
        {.location = 2,
         .format = AttributeFormat::Float,
         .offset = sizeof(Vec3f) + sizeof(Vec4f),
         .name = "elevation"},
        {.location = 3,
         .format = AttributeFormat::UInt,
         .offset = sizeof(Vec3f) + sizeof(Vec4f) + sizeof(float),
         .name = "terrain"}};

    static const VertexBinding shadow_binding{.binding = 0,
                                              .stride = sizeof(Vec3f) + sizeof(Vec4f) +
                                                        sizeof(float) + sizeof(std::uint32_t),
                                              .attributes = shadow_attributes};

    static const ShaderHandle shaders[] = {shadow_vertex_shader_, shadow_fragment_shader_};
    static const VertexBinding bindings[] = {shadow_binding};

    PipelineDesc pipeline_desc{.shaders = shaders,
                               .vertex_bindings = bindings,
                               .render_state = RenderState{.topology = PrimitiveTopology::Triangles,
                                                           .blend_mode = BlendMode::None,
                                                           .cull_mode = CullMode::Back,
                                                           .depth_test = DepthTest::Less,
                                                           .depth_write = true,
                                                           .wireframe = false},
                               .render_target = shadow_render_target_,
                               .debug_name = "shadow_pipeline"};

    auto pipeline_result = renderer_->create_pipeline(pipeline_desc);
    if (!pipeline_result) {
        return pipeline_result.error();
    }
    shadow_pipeline_ = *pipeline_result;

    return {};
}

void ShadowPass::calculate_cascade_matrices(const PassContext& context) {
    // Update light direction
    shadow_uniforms_.light_direction = context.sun_direction.normalize();

    // Calculate cascade splits in view space
    float near_plane = 1.0f;
    [[maybe_unused]] float far_plane = 1000.0f;

    for (std::uint32_t i = 0; i < CASCADE_COUNT; ++i) {
        float split_near = (i == 0) ? near_plane : cascade_splits_[i - 1];
        float split_far = cascade_splits_[i];

        // Calculate frustum corners for this cascade
        auto frustum_corners =
            calculate_frustum_corners(context.view_projection_matrix, split_near, split_far);

        // Create orthographic projection that tightly fits the frustum
        Mat4f light_view_proj =
            create_orthographic_projection(shadow_uniforms_.light_direction, frustum_corners);

        shadow_uniforms_.cascades[i].light_view_projection = light_view_proj;
        shadow_uniforms_.cascades[i].split_distance = split_far;
    }

    // Store cascade splits for use in main pass
    shadow_uniforms_.cascade_splits =
        Vec4f{cascade_splits_[0], cascade_splits_[1], cascade_splits_[2], cascade_splits_[3]};
}

Mat4f ShadowPass::create_orthographic_projection(const Vec3f& light_dir,
                                                 const std::array<Vec3f, 8>& frustum_corners) {
    // Create light view matrix
    Vec3f up = (std::abs(light_dir.dot(Vec3f{0, 1, 0})) < 0.99f) ? Vec3f{0, 1, 0} : Vec3f{1, 0, 0};
    Vec3f right = light_dir.cross(up).normalize();
    up = right.cross(light_dir).normalize();

    Mat4f light_view = Mat4f::look_at(Vec3f{0, 0, 0}, light_dir, up);

    // Transform frustum corners to light space
    Vec3f min_bounds{FLT_MAX, FLT_MAX, FLT_MAX};
    Vec3f max_bounds{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (const Vec3f& corner : frustum_corners) {
        Vec3f light_space_corner = light_view.transform_point(corner);
        min_bounds = min_bounds.min(light_space_corner);
        max_bounds = max_bounds.max(light_space_corner);
    }

    // Create orthographic projection
    Mat4f light_proj = Mat4f::orthographic(min_bounds.x(), max_bounds.x(), min_bounds.y(),
                                           max_bounds.y(), -max_bounds.z(), -min_bounds.z());

    return light_proj * light_view;
}

std::array<Vec3f, 8> ShadowPass::calculate_frustum_corners(const Mat4f& view_proj, [[maybe_unused]] float near_dist,
                                                           [[maybe_unused]] float far_dist) {
    // This is a simplified implementation
    // A full implementation would properly calculate frustum corners from the view projection
    // matrix
    Mat4f inv_view_proj = view_proj.inverse();

    std::array<Vec3f, 8> corners;
    std::array<Vec3f, 8> ndc_corners = {{
        {-1, -1, 0},
        {1, -1, 0},
        {1, 1, 0},
        {-1, 1, 0},  // Near plane
        {-1, -1, 1},
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1}  // Far plane
    }};

    for (std::size_t i = 0; i < 8; ++i) {
        Vec4f world_corner =
            inv_view_proj * Vec4f{ndc_corners[i].x(), ndc_corners[i].y(), ndc_corners[i].z(), 1.0f};
        if (world_corner.w() != 0.0f) {
            world_corner /= world_corner.w();
        }
        corners[i] = Vec3f{world_corner.x(), world_corner.y(), world_corner.z()};
    }

    return corners;
}

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest

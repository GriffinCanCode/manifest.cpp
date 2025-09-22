#include "MainPass.hpp"
#include "../../core/log/Log.hpp"

#include <cstring>
#include <span>

#include "../common/Renderer.hpp"

namespace Manifest {
namespace Render {
namespace Passes {

// Module-specific logger for main rendering pass
MODULE_LOGGER("MainPass");

Result<void> MainPass::initialize(Renderer* renderer) {
    logger_->info("Initializing main rendering pass");
    
    if (!renderer) {
        logger_->error("Cannot initialize MainPass: renderer is null");
        return RendererError::InvalidState;
    }

    renderer_ = renderer;

    // Initialize hex renderer with the same renderer
    hex_renderer_ = std::make_unique<ProceduralHexRenderer>();
    // Note: We need to create a new renderer instance or modify ProceduralHexRenderer
    // to accept a renderer pointer instead of owning it
    // For now, this is a design placeholder

    // Create main pass resources
    logger_->debug("Creating main pass resources");
    if (auto result = create_main_resources(); !result) {
        logger_->error("Failed to create main pass resources");
        return result;
    }

    logger_->debug("Creating main pass shaders");
    if (auto result = create_main_shaders(); !result) {
        logger_->error("Failed to create main pass shaders");
        return result;
    }

    logger_->debug("Creating main pass pipeline");
    if (auto result = create_main_pipeline(); !result) {
        logger_->error("Failed to create main pass pipeline");
        return result;
    }

    is_initialized_ = true;
    logger_->info("Main rendering pass initialized successfully");
    return {};
}

void MainPass::shutdown() {
    if (!renderer_) return;

    if (hex_renderer_) {
        hex_renderer_->shutdown();
        hex_renderer_.reset();
    }

    if (main_pipeline_.is_valid()) {
        renderer_->destroy_pipeline(main_pipeline_);
    }
    if (main_fragment_shader_.is_valid()) {
        renderer_->destroy_shader(main_fragment_shader_);
    }
    if (main_vertex_shader_.is_valid()) {
        renderer_->destroy_shader(main_vertex_shader_);
    }
    if (main_uniforms_buffer_.is_valid()) {
        renderer_->destroy_buffer(main_uniforms_buffer_);
    }

    is_initialized_ = false;
}

Result<void> MainPass::execute(const PassContext& context) {
    if (!is_initialized_) {
        logger_->error("Cannot execute MainPass: not initialized");
        return RendererError::InvalidState;
    }

    SCOPED_LOGGER_TRACE(*logger_, "Executing main pass - frame {}", context.frame_number);

    // Update uniforms with current frame data
    logger_->trace("Updating uniforms for frame {}", context.frame_number);
    update_uniforms(context);

    // Upload uniforms to GPU
    if (auto result = upload_uniforms(); !result) {
        logger_->error("Failed to upload uniforms to GPU");
        return result;
    }

    // Begin main render pass
    renderer_->begin_render_pass(context.main_target,
                                 Vec4f{0.2f, 0.3f, 0.5f, 1.0f});  // Sky blue clear

    // Set main viewport
    renderer_->set_viewport(context.viewport);

    // Bind main pipeline and uniforms
    renderer_->bind_pipeline(main_pipeline_);
    renderer_->bind_uniform_buffer(main_uniforms_buffer_, 0);

    // Bind shadow map if available
    if (shadow_pass_ && shadow_pass_->get_shadow_map().is_valid()) {
        renderer_->bind_texture(shadow_pass_->get_shadow_map(), 1);
    }

    // Use hex renderer for actual geometry rendering
    if (hex_renderer_) {
        logger_->trace("Updating hex renderer globals and lighting");
        // Update hex renderer with current camera settings
        hex_renderer_->update_globals(context.view_projection_matrix, context.camera_position,
                                      context.elapsed_time);
        hex_renderer_->update_lighting(context.sun_direction, context.sun_color,
                                       context.ambient_color, context.sun_intensity);

        // Render hex geometry
        logger_->trace("Rendering hex geometry");
        if (auto result = hex_renderer_->render(); !result) {
            logger_->error("Failed to render hex geometry");
            renderer_->end_render_pass();
            return result;
        }
    } else {
        logger_->warn("No hex renderer available for geometry rendering");
    }

    renderer_->end_render_pass();

    return {};
}

Result<void> MainPass::create_main_resources() {
    // Create main uniforms buffer
    BufferDesc uniforms_desc{.size = sizeof(MainUniforms),
                             .usage = BufferUsage::Uniform,
                             .host_visible = true,
                             .debug_name = "main_uniforms"};

    auto buffer_result = renderer_->create_buffer(uniforms_desc);
    if (!buffer_result) {
        return buffer_result.error();
    }
    main_uniforms_buffer_ = *buffer_result;

    return {};
}

Result<void> MainPass::create_main_shaders() {
    // Enhanced vertex shader with shadow mapping support
    std::string vert_source = R"(
#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float elevation;
layout(location = 3) in uint terrain;

layout(binding = 0) uniform MainUniforms {
    mat4 view_projection_matrix;
    vec3 camera_position;
    float _padding1;
    
    vec3 sun_direction;
    float sun_intensity;
    
    vec3 sun_color;
    float _padding2;
    
    vec3 ambient_color;
    float time;
    
    mat4 shadow_matrices[4];
    vec4 cascade_splits;
    float shadow_bias;
    vec3 _padding3;
} u_main;

out vec3 v_world_pos;
out vec3 v_normal;
out vec4 v_color;
out vec4 v_shadow_coords[4];

void main() {
    // Generate hex vertices procedurally
    vec3 hex_vertices[7] = vec3[](
        vec3(0.0, 0.0, 0.0),  // Center
        vec3(1.0, 0.0, 0.0), vec3(0.5, 0.0, 0.866), vec3(-0.5, 0.0, 0.866),
        vec3(-1.0, 0.0, 0.0), vec3(-0.5, 0.0, -0.866), vec3(0.5, 0.0, -0.866)
    );
    
    vec3 local_pos = hex_vertices[gl_VertexID];
    vec3 world_pos = position + local_pos;
    world_pos.y += elevation * 10.0; // Scale elevation
    
    v_world_pos = world_pos;
    v_normal = vec3(0.0, 1.0, 0.0); // Flat normals for now
    v_color = color;
    
    // Calculate shadow coordinates for all cascades
    for (int i = 0; i < 4; ++i) {
        v_shadow_coords[i] = u_main.shadow_matrices[i] * vec4(world_pos, 1.0);
    }
    
    gl_Position = u_main.view_projection_matrix * vec4(world_pos, 1.0);
}
)";

    // Enhanced fragment shader with shadow mapping
    std::string frag_source = R"(
#version 460 core

layout(binding = 0) uniform MainUniforms {
    mat4 view_projection_matrix;
    vec3 camera_position;
    float _padding1;
    
    vec3 sun_direction;
    float sun_intensity;
    
    vec3 sun_color;
    float _padding2;
    
    vec3 ambient_color;
    float time;
    
    mat4 shadow_matrices[4];
    vec4 cascade_splits;
    float shadow_bias;
    vec3 _padding3;
} u_main;

layout(binding = 1) uniform sampler2DArray u_shadow_map;

in vec3 v_world_pos;
in vec3 v_normal;
in vec4 v_color;
in vec4 v_shadow_coords[4];

out vec4 frag_color;

float sample_shadow(vec3 shadow_coord, int cascade) {
    if (shadow_coord.x < 0.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < 0.0 || shadow_coord.y > 1.0 ||
        shadow_coord.z < 0.0 || shadow_coord.z > 1.0) {
        return 1.0;
    }
    
    float shadow_depth = texture(u_shadow_map, vec3(shadow_coord.xy, cascade)).r;
    float current_depth = shadow_coord.z;
    
    return (current_depth - u_main.shadow_bias) <= shadow_depth ? 1.0 : 0.5;
}

void main() {
    vec3 normal = normalize(v_normal);
    vec3 light_dir = normalize(-u_main.sun_direction);
    
    // Calculate basic lighting
    float n_dot_l = max(0.0, dot(normal, light_dir));
    vec3 diffuse = u_main.sun_color * u_main.sun_intensity * n_dot_l;
    vec3 ambient = u_main.ambient_color;
    
    // Calculate shadow factor (simplified - would select appropriate cascade in full implementation)
    float shadow_factor = 1.0;
    if (length(v_shadow_coords[0].xyz) > 0.0) {
        vec3 shadow_coord = v_shadow_coords[0].xyz / v_shadow_coords[0].w;
        shadow_coord = shadow_coord * 0.5 + 0.5;
        shadow_factor = sample_shadow(shadow_coord, 0);
    }
    
    vec3 final_color = v_color.rgb * (ambient + diffuse * shadow_factor);
    frag_color = vec4(final_color, v_color.a);
}
)";

    // Create vertex shader
    const std::byte* vert_ptr = reinterpret_cast<const std::byte*>(vert_source.data());
    ShaderDesc vert_desc{.stage = ShaderStage::Vertex,
                         .bytecode = std::span<const std::byte>{vert_ptr, vert_source.size()},
                         .entry_point = "main",
                         .debug_name = "main_vertex"};

    auto vert_result = renderer_->create_shader(vert_desc);
    if (!vert_result) {
        return vert_result.error();
    }
    main_vertex_shader_ = *vert_result;

    // Create fragment shader
    const std::byte* frag_ptr = reinterpret_cast<const std::byte*>(frag_source.data());
    ShaderDesc frag_desc{.stage = ShaderStage::Fragment,
                         .bytecode = std::span<const std::byte>{frag_ptr, frag_source.size()},
                         .entry_point = "main",
                         .debug_name = "main_fragment"};

    auto frag_result = renderer_->create_shader(frag_desc);
    if (!frag_result) {
        return frag_result.error();
    }
    main_fragment_shader_ = *frag_result;

    return {};
}

Result<void> MainPass::create_main_pipeline() {
    // Use same vertex attributes as hex renderer
    static const VertexAttribute main_attributes[] = {
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

    static const VertexBinding main_binding{.binding = 0,
                                            .stride = sizeof(Vec3f) + sizeof(Vec4f) +
                                                      sizeof(float) + sizeof(std::uint32_t),
                                            .attributes = main_attributes};

    static const ShaderHandle shaders[] = {main_vertex_shader_, main_fragment_shader_};
    static const VertexBinding bindings[] = {main_binding};

    PipelineDesc pipeline_desc{.shaders = shaders,
                               .vertex_bindings = bindings,
                               .render_state = RenderState{.topology = PrimitiveTopology::Triangles,
                                                           .blend_mode = BlendMode::None,
                                                           .cull_mode = CullMode::Back,
                                                           .depth_test = DepthTest::Less,
                                                           .depth_write = true,
                                                           .wireframe = false},
                               .render_target = {},  // Use default framebuffer
                               .debug_name = "main_pipeline"};

    auto pipeline_result = renderer_->create_pipeline(pipeline_desc);
    if (!pipeline_result) {
        return pipeline_result.error();
    }
    main_pipeline_ = *pipeline_result;

    return {};
}

void MainPass::update_uniforms(const PassContext& context) {
    main_uniforms_.view_projection_matrix = context.view_projection_matrix;
    main_uniforms_.camera_position = context.camera_position;

    main_uniforms_.sun_direction = context.sun_direction;
    main_uniforms_.sun_intensity = context.sun_intensity;
    main_uniforms_.sun_color = context.sun_color;
    main_uniforms_.ambient_color = context.ambient_color;
    main_uniforms_.time = context.elapsed_time;

    // Copy shadow data if available
    if (shadow_pass_) {
        const auto& shadow_uniforms = shadow_pass_->get_shadow_uniforms();
        for (std::size_t i = 0; i < ShadowPass::CASCADE_COUNT; ++i) {
            main_uniforms_.shadow_matrices[i] = shadow_uniforms.cascades[i].light_view_projection;
        }
        main_uniforms_.cascade_splits = shadow_uniforms.cascade_splits;
        main_uniforms_.shadow_bias = shadow_uniforms.shadow_bias;
    }
}

Result<void> MainPass::upload_uniforms() {
    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(&main_uniforms_);
    std::span<const std::byte> data{data_ptr, sizeof(MainUniforms)};
    return renderer_->update_buffer(main_uniforms_buffer_, 0, data);
}

}  // namespace Passes
}  // namespace Render
}  // namespace Manifest

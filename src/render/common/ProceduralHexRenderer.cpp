#include "ProceduralHexRenderer.hpp"

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <algorithm>

#include "../../core/math/Vector.hpp"
#include "Renderer.hpp"

namespace Manifest::Render {

Result<void> ProceduralHexRenderer::initialize(std::unique_ptr<Renderer> renderer) {
    if (!renderer) {
        return std::unexpected(RendererError::InvalidState);
    }

    renderer_ = std::move(renderer);

    // Initialize renderer
    if (auto result = renderer_->initialize(); !result) {
        return result;
    }

    // Create GPU resources
    if (auto result = create_shaders(); !result) {
        return result;
    }

    if (auto result = create_buffers(); !result) {
        return result;
    }

    if (auto result = create_pipeline(); !result) {
        return result;
    }

    // Initialize culling systems
    ensure_culling_systems();

    is_initialized_ = true;
    return {};
}

void ProceduralHexRenderer::shutdown() {
    if (!renderer_) return;

    if (render_pipeline_.is_valid()) {
        renderer_->destroy_pipeline(render_pipeline_);
    }
    if (fragment_shader_.is_valid()) {
        renderer_->destroy_shader(fragment_shader_);
    }
    if (vertex_shader_.is_valid()) {
        renderer_->destroy_shader(vertex_shader_);
    }
    if (lighting_uniforms_buffer_.is_valid()) {
        renderer_->destroy_buffer(lighting_uniforms_buffer_);
    }
    if (global_uniforms_buffer_.is_valid()) {
        renderer_->destroy_buffer(global_uniforms_buffer_);
    }
    if (instance_buffer_.is_valid()) {
        renderer_->destroy_buffer(instance_buffer_);
    }

    renderer_->shutdown();
    renderer_.reset();
    is_initialized_ = false;
}

void ProceduralHexRenderer::update_globals(const Mat4f& view_proj, const Vec3f& camera_pos,
                                           float time) {
    global_uniforms_.view_projection_matrix = view_proj;
    global_uniforms_.camera_position = camera_pos;
    global_uniforms_.time = time;
    
    // Update frustum culler with new view-projection matrix
    if (frustum_culler_ && culling_enabled_) {
        frustum_culler_->update_frustum(view_proj);
    }
}

void ProceduralHexRenderer::update_lighting(const Vec3f& sun_dir, const Vec3f& sun_color,
                                            const Vec3f& ambient_color, float sun_intensity) {
    lighting_uniforms_.sun_direction = sun_dir.normalize();
    lighting_uniforms_.sun_color = sun_color;
    lighting_uniforms_.ambient_color = ambient_color;
    lighting_uniforms_.sun_intensity = sun_intensity;
    lighting_uniforms_.camera_position = global_uniforms_.camera_position;
}

void ProceduralHexRenderer::set_world_tiles(const std::vector<const Tile*>& tiles) {
    all_tiles_.clear();
    all_tiles_.reserve(tiles.size());
    
    for (const Tile* tile : tiles) {
        if (tile) {
            all_tiles_.push_back(tile);
        }
    }
    
    spatial_hash_dirty_ = true;
}

void ProceduralHexRenderer::prepare_instances_culled() {
    current_instances_.clear();
    
    if (!culling_enabled_ || all_tiles_.empty()) {
        // Fallback to rendering all tiles without culling
        prepare_instances(all_tiles_);
        return;
    }
    
    ensure_culling_systems();
    update_spatial_hash();
    
    // Perform frustum culling using spatial hash
    if (spatial_hash_ && frustum_culler_) {
        spatial_hash_->query_frustum(*frustum_culler_, culling_query_result_);
        
        // Estimate instance count for better performance
        std::size_t visible_count = culling_query_result_.total_visible();
        current_instances_.reserve(std::min(visible_count, MAX_INSTANCES));
        
        // Convert visible tiles to instances
        auto process_tiles = [this](const std::vector<const Tile*>& tiles) {
            for (const Tile* tile : tiles) {
                if (!tile || current_instances_.size() >= MAX_INSTANCES) {
                    continue;
                }
                
                Vec3f world_pos = hex_coord_to_world(tile->coordinate(), global_uniforms_.hex_radius);
                float elevation = tile->elevation() / 255.0f;
                Vec4f color = get_terrain_color(tile->terrain(), elevation);
                
                current_instances_.push_back(
                    HexInstance{.position = world_pos,
                                .color = color,
                                .elevation = elevation,
                                .terrain = static_cast<std::uint32_t>(tile->terrain())});
            }
        };
        
        // Process fully visible tiles (inside frustum)
        process_tiles(culling_query_result_.visible_tiles);
        
        // Process partially visible tiles (intersecting frustum)
        process_tiles(culling_query_result_.intersecting_tiles);
    }
}

void ProceduralHexRenderer::prepare_instances(const std::vector<const Tile*>& tiles) {
    current_instances_.clear();
    current_instances_.reserve(std::min(tiles.size(), MAX_INSTANCES));

    for (const Tile* tile : tiles) {
        if (!tile || current_instances_.size() >= MAX_INSTANCES) {
            continue;
        }

        Vec3f world_pos = hex_coord_to_world(tile->coordinate(), global_uniforms_.hex_radius);
        float elevation = tile->elevation() / 255.0f;  // Normalize to 0-1
        Vec4f color = get_terrain_color(tile->terrain(), elevation);

        current_instances_.push_back(
            HexInstance{.position = world_pos,
                        .color = color,
                        .elevation = elevation,
                        .terrain = static_cast<std::uint32_t>(tile->terrain())});
    }
}

void ProceduralHexRenderer::add_instance(const Vec3f& position, const Vec4f& color, float elevation,
                                         std::uint32_t terrain_type) {
    if (current_instances_.size() >= MAX_INSTANCES) {
        return;
    }

    current_instances_.push_back(HexInstance{
        .position = position, .color = color, .elevation = elevation, .terrain = terrain_type});
}

void ProceduralHexRenderer::clear_instances() {
    current_instances_.clear();
}

Result<void> ProceduralHexRenderer::render() {
    if (!is_initialized_ || current_instances_.empty()) {
        return {};
    }

    // Upload instance data and uniforms
    if (auto result = upload_instances(); !result) {
        return result;
    }

    if (auto result = upload_uniforms(); !result) {
        return result;
    }

    // Begin frame
    renderer_->begin_frame();

    // Set pipeline
    renderer_->bind_pipeline(render_pipeline_);

    // Bind buffers
    renderer_->bind_vertex_buffer(instance_buffer_, 0);
    renderer_->bind_uniform_buffer(global_uniforms_buffer_, 0);
    renderer_->bind_uniform_buffer(lighting_uniforms_buffer_, 1);

    // Draw instanced (7 vertices per hex * number of instances)
    DrawCommand draw_cmd{.vertex_count = VERTICES_PER_HEX,
                         .instance_count = static_cast<std::uint32_t>(current_instances_.size()),
                         .first_vertex = 0,
                         .first_instance = 0};

    renderer_->draw(draw_cmd);

    // End frame
    renderer_->end_frame();

    return {};
}

Result<void> ProceduralHexRenderer::create_shaders() {
    // Load vertex shader
    std::filesystem::path shader_dir = "assets/shaders";
    std::ifstream vert_file(shader_dir / "hex_procedural.vert");
    if (!vert_file) {
        return std::unexpected(RendererError::CompilationFailed);
    }

    std::stringstream vert_buffer;
    vert_buffer << vert_file.rdbuf();
    std::string vert_source = vert_buffer.str();

    // Load fragment shader
    std::ifstream frag_file(shader_dir / "hex_procedural.frag");
    if (!frag_file) {
        return std::unexpected(RendererError::CompilationFailed);
    }

    std::stringstream frag_buffer;
    frag_buffer << frag_file.rdbuf();
    std::string frag_source = frag_buffer.str();

    // Create shader descriptions
    const std::byte* vert_ptr = reinterpret_cast<const std::byte*>(vert_source.data());
    const std::byte* frag_ptr = reinterpret_cast<const std::byte*>(frag_source.data());

    ShaderDesc vert_desc{.stage = ShaderStage::Vertex,
                         .bytecode = std::span<const std::byte>{vert_ptr, vert_source.size()},
                         .entry_point = "main",
                         .debug_name = "hex_procedural_vertex"};

    ShaderDesc frag_desc{.stage = ShaderStage::Fragment,
                         .bytecode = std::span<const std::byte>{frag_ptr, frag_source.size()},
                         .entry_point = "main",
                         .debug_name = "hex_procedural_fragment"};

    // Create shaders
    auto vert_result = renderer_->create_shader(vert_desc);
    if (!vert_result) {
        return std::unexpected(vert_result.error());
    }
    vertex_shader_ = *vert_result;

    auto frag_result = renderer_->create_shader(frag_desc);
    if (!frag_result) {
        return std::unexpected(frag_result.error());
    }
    fragment_shader_ = *frag_result;

    return {};
}

Result<void> ProceduralHexRenderer::create_buffers() {
    // Create instance buffer (dynamic, host-visible)
    BufferDesc instance_desc{.size = MAX_INSTANCES * HexInstance::size(),
                             .usage = BufferUsage::Vertex,
                             .host_visible = true,
                             .debug_name = "hex_instance_buffer"};

    auto instance_result = renderer_->create_buffer(instance_desc);
    if (!instance_result) {
        return std::unexpected(instance_result.error());
    }
    instance_buffer_ = *instance_result;

    // Create global uniforms buffer
    BufferDesc global_desc{.size = sizeof(GlobalUniforms),
                           .usage = BufferUsage::Uniform,
                           .host_visible = true,
                           .debug_name = "hex_global_uniforms"};

    auto global_result = renderer_->create_buffer(global_desc);
    if (!global_result) {
        return std::unexpected(global_result.error());
    }
    global_uniforms_buffer_ = *global_result;

    // Create lighting uniforms buffer
    BufferDesc lighting_desc{.size = sizeof(LightingUniforms),
                             .usage = BufferUsage::Uniform,
                             .host_visible = true,
                             .debug_name = "hex_lighting_uniforms"};

    auto lighting_result = renderer_->create_buffer(lighting_desc);
    if (!lighting_result) {
        return std::unexpected(lighting_result.error());
    }
    lighting_uniforms_buffer_ = *lighting_result;

    return {};
}

Result<void> ProceduralHexRenderer::create_pipeline() {
    // Define vertex attributes for instance data
    static const VertexAttribute instance_attributes[] = {
        {.location = 0,
         .format = AttributeFormat::Float3,
         .offset = 0,
         .name = "instance_position"},
        {.location = 1,
         .format = AttributeFormat::Float4,
         .offset = sizeof(Vec3f),
         .name = "instance_color"},
        {.location = 2,
         .format = AttributeFormat::Float,
         .offset = sizeof(Vec3f) + sizeof(Vec4f),
         .name = "instance_elevation"},
        {.location = 3,
         .format = AttributeFormat::UInt,
         .offset = sizeof(Vec3f) + sizeof(Vec4f) + sizeof(float),
         .name = "instance_terrain"}};

    static const VertexBinding instance_binding{.binding = 0,
                                                .stride =
                                                    static_cast<std::uint32_t>(HexInstance::size()),
                                                .attributes = instance_attributes};

    // Create pipeline
    static const ShaderHandle shaders[] = {vertex_shader_, fragment_shader_};
    static const VertexBinding bindings[] = {instance_binding};

    PipelineDesc pipeline_desc{.shaders = shaders,
                               .vertex_bindings = bindings,
                               .render_state = RenderState{.topology = PrimitiveTopology::Triangles,
                                                           .blend_mode = BlendMode::None,
                                                           .cull_mode = CullMode::Back,
                                                           .depth_test = DepthTest::Less,
                                                           .depth_write = true,
                                                           .wireframe = false},
                               .render_target = {},  // Use default framebuffer
                               .debug_name = "hex_procedural_pipeline"};

    auto pipeline_result = renderer_->create_pipeline(pipeline_desc);
    if (!pipeline_result) {
        return std::unexpected(pipeline_result.error());
    }
    render_pipeline_ = *pipeline_result;

    return {};
}

Result<void> ProceduralHexRenderer::upload_instances() {
    if (current_instances_.empty()) {
        return {};
    }

    std::size_t data_size = current_instances_.size() * HexInstance::size();
    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(current_instances_.data());
    std::span<const std::byte> data{data_ptr, data_size};

    return renderer_->update_buffer(instance_buffer_, 0, data.subspan(0, data_size));
}

Result<void> ProceduralHexRenderer::upload_uniforms() {
    // Upload global uniforms
    const std::byte* global_ptr = reinterpret_cast<const std::byte*>(&global_uniforms_);
    std::span<const std::byte> global_data{global_ptr, sizeof(GlobalUniforms)};
    if (auto result = renderer_->update_buffer(global_uniforms_buffer_, 0, global_data); !result) {
        return result;
    }

    // Upload lighting uniforms
    const std::byte* lighting_ptr = reinterpret_cast<const std::byte*>(&lighting_uniforms_);
    std::span<const std::byte> lighting_data{lighting_ptr, sizeof(LightingUniforms)};
    return renderer_->update_buffer(lighting_uniforms_buffer_, 0, lighting_data);
}

// Culling statistics methods
std::size_t ProceduralHexRenderer::get_tiles_tested() const noexcept {
    return culling_query_result_.total_tested;
}

std::size_t ProceduralHexRenderer::get_tiles_culled() const noexcept {
    return culling_query_result_.total_culled;
}

std::size_t ProceduralHexRenderer::get_tiles_rendered() const noexcept {
    return current_instances_.size();
}

// Private helper methods
void ProceduralHexRenderer::update_spatial_hash() {
    if (!spatial_hash_ || !spatial_hash_dirty_) {
        return;
    }
    
    // Update hex radius in spatial hash if changed
    spatial_hash_->set_hex_radius(global_uniforms_.hex_radius);
    
    // Rebuild spatial hash with current tiles
    if (!all_tiles_.empty()) {
        spatial_hash_->rebuild(all_tiles_);
    }
    
    spatial_hash_dirty_ = false;
}

void ProceduralHexRenderer::ensure_culling_systems() {
    if (!frustum_culler_) {
        frustum_culler_ = std::make_unique<FrustumCuller>();
    }
    
    if (!spatial_hash_) {
        // Create spatial hash with reasonable defaults:
        // - Base bucket size of 16 hex tiles 
        // - 3 hierarchical levels for multi-scale culling
        spatial_hash_ = std::make_unique<SpatialHash>(
            global_uniforms_.hex_radius, 
            16,  // base bucket size
            3    // number of levels
        );
        spatial_hash_dirty_ = true;
    }
}

}  // namespace Manifest::Render

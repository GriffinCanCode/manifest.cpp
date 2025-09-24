#include "ProceduralHexRenderer.hpp"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <algorithm>

#include "../../core/math/Vector.hpp"
#include "../../core/log/Log.hpp"
#include "Renderer.hpp"

namespace Manifest::Render {

Result<void> ProceduralHexRenderer::initialize(std::unique_ptr<Renderer> renderer) {
    LOG_DEBUG("ProceduralHexRenderer::initialize() starting");
    
    if (!renderer) {
        LOG_ERROR("ProceduralHexRenderer::initialize() - renderer is null");
        return RendererError::InvalidState;
    }

    renderer_ = std::move(renderer);

    // Renderer should already be initialized by the caller
    // No need to initialize again

    // Create GPU resources with debug logging
    LOG_DEBUG("ProceduralHexRenderer::initialize() - creating shaders");
    if (auto result = create_shaders(); !result) {
        LOG_ERROR("ProceduralHexRenderer::initialize() - shader creation failed");
        return result;
    }

    LOG_DEBUG("ProceduralHexRenderer::initialize() - creating buffers");
    if (auto result = create_buffers(); !result) {
        LOG_ERROR("ProceduralHexRenderer::initialize() - buffer creation failed");
        return result;
    }

    LOG_DEBUG("ProceduralHexRenderer::initialize() - creating pipeline");
    if (auto result = create_pipeline(); !result) {
        LOG_ERROR("ProceduralHexRenderer::initialize() - pipeline creation failed");
        return result;
    }

    // Initialize culling systems
    LOG_DEBUG("ProceduralHexRenderer::initialize() - ensuring culling systems");
    ensure_culling_systems();

    is_initialized_ = true;
    LOG_DEBUG("ProceduralHexRenderer::initialize() - completed successfully");
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
    // Note: No uniform buffers to clean up - using push constants
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
    Vec3f normalized_sun_dir = sun_dir;
    normalized_sun_dir.normalize();
    lighting_uniforms_.sun_direction = normalized_sun_dir;
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
        current_instances_.reserve(std::min(static_cast<std::uint32_t>(visible_count), MAX_INSTANCES));
        
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
                                .terrain = static_cast<float>(tile->terrain())});
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
    current_instances_.reserve(std::min(static_cast<std::uint32_t>(tiles.size()), MAX_INSTANCES));

    LOG_INFO("ProceduralHexRenderer::prepare_instances() - processing {} tiles", tiles.size());

    for (const Tile* tile : tiles) {
        if (!tile || current_instances_.size() >= MAX_INSTANCES) {
            continue;
        }

        Vec3f world_pos = hex_coord_to_world(tile->coordinate(), global_uniforms_.hex_radius);
        float elevation = tile->elevation() / 255.0f;  // Normalize to 0-1
        Vec4f color = get_terrain_color(tile->terrain(), elevation);
        
        // Use proper terrain colors now that rendering works

        // Log first few instances to verify data
        if (current_instances_.size() < 3) {
            LOG_INFO("Instance {}: pos=({}, {}, {}), color=({}, {}, {}, {}), terrain={}, elevation={}", 
                     current_instances_.size(), world_pos.x(), world_pos.y(), world_pos.z(),
                     color.x(), color.y(), color.z(), color.w(), 
                     static_cast<int>(tile->terrain()), tile->elevation());
        }

        current_instances_.push_back(
            HexInstance{.position = world_pos,
                        .color = color,
                        .elevation = elevation,
                        .terrain = static_cast<float>(tile->terrain())});
    }
    
    LOG_INFO("ProceduralHexRenderer::prepare_instances() - created {} instances", current_instances_.size());
}

void ProceduralHexRenderer::add_instance(const Vec3f& position, const Vec4f& color, float elevation,
                                         float terrain_type) {
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
    if (!is_initialized_) {
        LOG_ERROR("ProceduralHexRenderer::render() - not initialized");
        return RendererError::InvalidState;
    }

    LOG_DEBUG("ProceduralHexRenderer::render() - starting (instances: {})", current_instances_.size());

    // Upload instance data and uniforms
    if (auto result = upload_instances(); !result) {
        return result;
    }

    // Note: No uniform buffers to upload - using push constants

    // Begin frame
    LOG_DEBUG("ProceduralHexRenderer::render() - beginning frame");
    renderer_->begin_frame();

    // Set pipeline
    LOG_DEBUG("ProceduralHexRenderer::render() - binding pipeline");
    renderer_->bind_pipeline(render_pipeline_);

    // Push constants (view-projection matrix)
    auto matrix_data = std::as_bytes(Core::Modern::span<const float>(&global_uniforms_.view_projection_matrix[0][0], 16));
    renderer_->push_constants(matrix_data);
    LOG_DEBUG("ProceduralHexRenderer::render() - pushed constants");

    // Bind instance buffer for position and color data
    renderer_->bind_vertex_buffer(instance_buffer_, 0);

    // Draw full hex geometry - 18 vertices (6 triangles * 3 vertices each)
    DrawCommand draw_cmd{.vertex_count = VERTICES_PER_HEX,  // Full hex per instance
                         .instance_count = static_cast<uint32_t>(current_instances_.size()),
                         .first_vertex = 0,
                         .first_instance = 0};

    LOG_DEBUG("ProceduralHexRenderer::render() - issuing draw command: {} vertices, {} instances", 
              draw_cmd.vertex_count, draw_cmd.instance_count);
    renderer_->draw(draw_cmd);

    // End frame
    LOG_DEBUG("ProceduralHexRenderer::render() - ending frame");
    renderer_->end_frame();

    LOG_DEBUG("ProceduralHexRenderer::render() - completed successfully");
    return {};
}

Result<void> ProceduralHexRenderer::render_frame(const Vec4f& clear_color, const Viewport& viewport) {
    if (!is_initialized_) {
        LOG_ERROR("ProceduralHexRenderer::render_frame() - not initialized");
        return RendererError::InvalidState;
    }

    LOG_DEBUG("ProceduralHexRenderer::render_frame() - starting (instances: {})", current_instances_.size());

    // Upload instance data and uniforms
    if (auto result = upload_instances(); !result) {
        return result;
    }

    // Note: No uniform buffers to upload - using push constants

    // Begin frame
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - beginning frame");
    renderer_->begin_frame();

    // Set up render pass with clear color
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - beginning render pass with clear color ({}, {}, {}, {})", 
              clear_color.x(), clear_color.y(), clear_color.z(), clear_color.w());
    renderer_->begin_render_pass({}, clear_color);  // Default render target (screen)
    
    // Set viewport
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - setting viewport {}x{}", viewport.size.x(), viewport.size.y());
    renderer_->set_viewport(viewport);

    // Set pipeline
    //======================================================================
    // CRITICAL RENDERING SEQUENCE - MUST BE DONE IN THIS EXACT ORDER
    //======================================================================
    // This sequence was discovered during Vulkan→OpenGL migration debugging
    // and represents the ONLY reliable way to render with our OpenGL setup:
    //======================================================================
    
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - binding pipeline");
    // STEP 1: Activate shader program (CRITICAL - without this, rendering fails)
    renderer_->bind_pipeline(render_pipeline_);

    // STEP 2: Upload matrix data (even though shader doesn't use it yet)
    // This maintains compatibility for future camera system
    auto matrix_data = std::as_bytes(Core::Modern::span<const float>(&global_uniforms_.view_projection_matrix[0][0], 16));
    renderer_->push_constants(matrix_data);
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - pushed constants");

    // STEP 3: Bind instance data buffer (position and color data from C++)
    // CRITICAL: This MUST happen after bind_pipeline for vertex attributes to work
    renderer_->bind_vertex_buffer(instance_buffer_, 0);

    // STEP 4: Issue draw command with FULL HEX vertex count
    // Updated to use full hex geometry (18 vertices) while maintaining gl_VertexID approach
    DrawCommand draw_cmd{.vertex_count = VERTICES_PER_HEX,  // Full hex geometry (18 vertices)
                         .instance_count = static_cast<uint32_t>(current_instances_.size()),
                         .first_vertex = 0,
                         .first_instance = 0};

    LOG_DEBUG("ProceduralHexRenderer::render_frame() - issuing draw command: {} vertices, {} instances", 
              draw_cmd.vertex_count, draw_cmd.instance_count);
    renderer_->draw(draw_cmd);

    // End render pass and frame
    LOG_DEBUG("ProceduralHexRenderer::render_frame() - ending render pass and frame");
    renderer_->end_render_pass();
    renderer_->end_frame();

    LOG_DEBUG("ProceduralHexRenderer::render_frame() - completed successfully");
    return {};
}

Result<void> ProceduralHexRenderer::create_shaders() {
    // Load vertex shader - try multiple possible locations
    std::vector<std::filesystem::path> shader_dirs = {
        "assets/shaders",
        "build/assets/shaders",
        "../assets/shaders"
    };
    
    std::ifstream vert_file;
    std::filesystem::path vert_path;
    
    for (const auto& shader_dir : shader_dirs) {
        vert_path = shader_dir / "hex_procedural.vert";  // Back to hex shader
        // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - trying vertex shader path: {}", vert_path.string());
        vert_file.open(vert_path);
        if (vert_file.is_open()) {
            // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - found vertex shader at: {}", vert_path.string());
            break;
        }
    }
    
    if (!vert_file.is_open()) {
        LOG_ERROR("ProceduralHexRenderer::create_shaders() - failed to open vertex shader file");
        return RendererError::CompilationFailed;
    }
    
    LOG_DEBUG("ProceduralHexRenderer::create_shaders() - vertex shader loaded from: {}", vert_path.string());

    std::stringstream vert_buffer;
    vert_buffer << vert_file.rdbuf();
    std::string vert_source = vert_buffer.str();

    // Load fragment shader using the same search paths
    std::ifstream frag_file;
    std::filesystem::path frag_path;
    
    for (const auto& shader_dir : shader_dirs) {
        frag_path = shader_dir / "hex_procedural.frag";  // Back to hex shader
        // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - trying fragment shader path: {}", frag_path.string());
        frag_file.open(frag_path);
        if (frag_file.is_open()) {
            // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - found fragment shader at: {}", frag_path.string());
            break;
        }
    }
    
    if (!frag_file.is_open()) {
        LOG_ERROR("ProceduralHexRenderer::create_shaders() - failed to open fragment shader file");
        return RendererError::CompilationFailed;
    }
    
    LOG_DEBUG("ProceduralHexRenderer::create_shaders() - fragment shader loaded from: {}", frag_path.string());

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
    // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - creating vertex shader with {} bytes", vert_source.size());
    auto vert_result = renderer_->create_shader(vert_desc);
    if (!vert_result) {
        LOG_ERROR("ProceduralHexRenderer::create_shaders() - vertex shader creation failed");
        return vert_result.error();
    }
    vertex_shader_ = *vert_result;
    // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - vertex shader created successfully");

    // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - creating fragment shader with {} bytes", frag_source.size());
    auto frag_result = renderer_->create_shader(frag_desc);
    if (!frag_result) {
        LOG_ERROR("ProceduralHexRenderer::create_shaders() - fragment shader creation failed");
        return frag_result.error();
    }
    fragment_shader_ = *frag_result;
    // LOG_DEBUG("ProceduralHexRenderer::create_shaders() - fragment shader created successfully");

    return {};
}

Result<void> ProceduralHexRenderer::create_buffers() {
    // Create vertex buffer for hex geometry (6 triangles from center)
    constexpr float HEX_RADIUS = 1.0f;
    constexpr float PI = 3.14159265359f;
    std::vector<float> hex_vertices;
    
    // Generate hex as 6 triangles from center point
    for (int i = 0; i < 6; ++i) {
        float angle1 = (i * 2.0f * PI) / 6.0f;
        float angle2 = ((i + 1) * 2.0f * PI) / 6.0f;
        
        // Triangle: center, vertex i, vertex i+1
        // Center point
        hex_vertices.insert(hex_vertices.end(), {0.0f, 0.0f, 0.0f});
        // First edge vertex  
        hex_vertices.insert(hex_vertices.end(), {
            HEX_RADIUS * cos(angle1), 0.0f, HEX_RADIUS * sin(angle1)
        });
        // Second edge vertex
        hex_vertices.insert(hex_vertices.end(), {
            HEX_RADIUS * cos(angle2), 0.0f, HEX_RADIUS * sin(angle2)
        });
    }
    
    BufferDesc vertex_desc{.size = hex_vertices.size() * sizeof(float),
                          .usage = BufferUsage::Vertex,
                          .host_visible = false,
                          .debug_name = "hex_vertex_buffer"};
    
    auto vertex_result = renderer_->create_buffer(vertex_desc, 
        std::as_bytes(Core::Modern::span<const float>(hex_vertices.data(), hex_vertices.size())));
    if (!vertex_result) {
        return vertex_result.error();
    }
    vertex_buffer_ = *vertex_result;

    // Create instance buffer (dynamic, host-visible)
    BufferDesc instance_desc{.size = MAX_INSTANCES * HexInstance::size(),
                             .usage = BufferUsage::Vertex,
                             .host_visible = true,
                             .debug_name = "hex_instance_buffer"};

    auto instance_result = renderer_->create_buffer(instance_desc);
    if (!instance_result) {
        return instance_result.error();
    }
    instance_buffer_ = *instance_result;

    return {};
}

Result<void> ProceduralHexRenderer::create_pipeline() {
    //==========================================================================
    // CRITICAL: MINIMAL VERTEX ATTRIBUTE SETUP - PROVEN TO WORK
    //==========================================================================
    // During Vulkan→OpenGL migration, we discovered that complex vertex setups
    // BREAK OpenGL rendering silently. This minimal approach WORKS:
    // 
    // ✅ SAFE: Only 2 instance attributes (position + color)
    // ✅ SAFE: Simple layout with locations 1,2 (location 0 reserved for gl_VertexID)
    // ✅ SAFE: Single binding point (multiple bindings caused failures)
    // 
    // ⚠️  DANGER: Adding more attributes can break rendering silently!
    // ⚠️  DANGER: Complex binding setups cause OpenGL pipeline failures!
    //==========================================================================
    
    // MINIMAL instance data attributes (ONLY what shader actually uses)
    static const VertexAttribute instance_attributes[] = {
        {.location = 1, .format = AttributeFormat::Float3, .offset = 0, .name = "instance_position"},
        {.location = 2, .format = AttributeFormat::Float4, .offset = sizeof(Vec3f), .name = "instance_color"}
    };

    // SIMPLE binding configuration (complexity breaks OpenGL)
    static const VertexBinding instance_binding{
        .binding = 0,  // Single binding point - MUST match bind_vertex_buffer calls
        .stride = static_cast<std::uint32_t>(HexInstance::size()),
        .attributes = Core::Modern::span<const VertexAttribute>{instance_attributes},
        .input_rate = VertexInputRate::Instance  // Per-instance data advancement
    };
    
    static const ShaderHandle shaders[] = {vertex_shader_, fragment_shader_};
    static const VertexBinding bindings[] = {instance_binding};

    PipelineDesc pipeline_desc{.shaders = Core::Modern::span<const ShaderHandle>{shaders},
                               .vertex_bindings = Core::Modern::span<const VertexBinding>{bindings},
                               .render_state = RenderState{.topology = PrimitiveTopology::Triangles,
                                                           .blend_mode = BlendMode::None,
                                                           .cull_mode = CullMode::Back,  // Enable backface culling for performance
                                                           .depth_test = DepthTest::Less,  // Proper depth testing for 3D hexagons
                                                           .depth_write = true,  // Enable depth writing
                                                           .wireframe = false},  // Solid rendering
                               .render_target = {},  // Use default framebuffer
                               .debug_name = "hex_procedural_pipeline"};

    auto pipeline_result = renderer_->create_pipeline(pipeline_desc);
    if (!pipeline_result) {
        return pipeline_result.error();
    }
    render_pipeline_ = *pipeline_result;

    return {};
}

Result<void> ProceduralHexRenderer::upload_instances() {
    if (current_instances_.empty()) {
        LOG_DEBUG("ProceduralHexRenderer::upload_instances() - no instances to upload");
        return {};
    }

    std::size_t data_size = current_instances_.size() * HexInstance::size();
    std::size_t max_buffer_size = MAX_INSTANCES * HexInstance::size();
    
    LOG_INFO("ProceduralHexRenderer::upload_instances() - uploading {} instances, {} bytes", 
              current_instances_.size(), data_size);
    
    // Critical bounds checking to prevent buffer overflow
    if (data_size > max_buffer_size) {
        LOG_ERROR("Instance data size {} exceeds buffer capacity {}", data_size, max_buffer_size);
        return RendererError::InvalidState;
    }

    // Debug: Check first instance data before upload
    if (!current_instances_.empty()) {
        const auto& first = current_instances_[0];
        LOG_INFO("First instance data: pos=({}, {}, {}), color=({}, {}, {}, {}), size={} bytes", 
                  first.position.x(), first.position.y(), first.position.z(),
                  first.color.x(), first.color.y(), first.color.z(), first.color.w(),
                  HexInstance::size());
    }

    const std::byte* data_ptr = reinterpret_cast<const std::byte*>(current_instances_.data());
    std::span<const std::byte> data{data_ptr, data_size};

    // Remove redundant subspan call that could cause issues
    auto result = renderer_->update_buffer(instance_buffer_, 0, data);
    if (!result) {
        LOG_ERROR("ProceduralHexRenderer::upload_instances() - buffer update failed");
    } else {
        LOG_INFO("ProceduralHexRenderer::upload_instances() - buffer update successful");
    }
    return result;
}

// Note: upload_uniforms() function removed - using push constants instead

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

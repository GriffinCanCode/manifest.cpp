#include "Core.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#include "Converter.hpp"
#include "Debug.hpp"

// Define missing OpenGL constants
#ifndef GL_SHADER
#define GL_SHADER 0x82E1
#endif

#ifndef GL_PROGRAM
#define GL_PROGRAM 0x82E2
#endif

#ifndef GL_COMPUTE_SHADER
#define GL_COMPUTE_SHADER 0x91B9
#endif

// Define compute dispatch function if not available
#ifndef glDispatchCompute
#ifdef GL_VERSION_4_3
// Compute shaders available in OpenGL 4.3+
#else
// Stub for older OpenGL versions
static void glDispatchCompute(GLuint, GLuint, GLuint) {
    // No-op for unsupported versions
}
#endif
#endif

// Use explicit qualification for OpenGL types

namespace Manifest {
namespace Render {
namespace OpenGL {

Result<void> Core::initialize() {
    //==========================================================================
    // CRITICAL OPENGL INITIALIZATION - REQUIRED FOR RENDERING TO WORK
    //==========================================================================
    // These steps are MANDATORY and were discovered during Vulkan→OpenGL migration:
    //==========================================================================
    
    // Enable OpenGL debug output for troubleshooting
    Debug::enable_debug_output();

    // RESTORED: Enable proper 3D rendering state now that basic rendering works
    // These were disabled during debugging but can be safely re-enabled
    glEnable(GL_DEPTH_TEST);   // Enable depth testing for proper 3D hex layering
    glDepthFunc(GL_LESS);      // Standard depth function
    glEnable(GL_CULL_FACE);    // Enable face culling for performance
    glCullFace(GL_BACK);       // Cull back faces
    glFrontFace(GL_CCW);       // Counter-clockwise front faces
    
    // CRITICAL: Create uniform buffer for matrix data (even if not used yet)
    // This simulates Vulkan push constants and must be created before shaders
    glGenBuffers(1, &push_constants_buffer_);
    glBindBuffer(GL_UNIFORM_BUFFER, push_constants_buffer_);
    glBufferData(GL_UNIFORM_BUFFER, 256, nullptr, GL_DYNAMIC_DRAW); // 256 bytes for matrices
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, push_constants_buffer_); // MUST bind to point 0
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // CRITICAL: Create VAO for vertex state management
    // Modern OpenGL requires VAO for any vertex rendering
    glGenVertexArrays(1, &vertex_array_object_);
    glBindVertexArray(vertex_array_object_);

    return {};
}

void Core::shutdown() {
    // Clean up all resources
    for (auto& [handle, resource] : render_targets_) {
        if (resource.framebuffer) glDeleteFramebuffers(1, &resource.framebuffer);
    }

    for (auto& [handle, resource] : pipelines_) {
        if (resource.program) glDeleteProgram(resource.program);
    }

    for (auto& [handle, resource] : shaders_) {
        if (resource.id) glDeleteShader(resource.id);
    }

    for (auto& [handle, resource] : textures_) {
        if (resource.id) glDeleteTextures(1, &resource.id);
    }

    for (auto& [handle, resource] : buffers_) {
        if (resource.id) glDeleteBuffers(1, &resource.id);
    }
    
    // Clean up push constants uniform buffer
    if (push_constants_buffer_) {
        glDeleteBuffers(1, &push_constants_buffer_);
        push_constants_buffer_ = 0;
    }
    
    // Clean up VAO
    if (vertex_array_object_) {
        glDeleteVertexArrays(1, &vertex_array_object_);
        vertex_array_object_ = 0;
    }

    // Clear maps
    render_targets_.clear();
    pipelines_.clear();
    shaders_.clear();
    textures_.clear();
    buffers_.clear();
}

Result<BufferHandle> Core::create_buffer(const BufferDesc& desc, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> initial_data) {
    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);

    if (!buffer_id) {
        return RendererError::InitializationFailed;
    }

    GLenum target = Converter::buffer_usage_to_gl_target(desc.usage);
    GLenum usage = desc.host_visible ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

    glBindBuffer(target, buffer_id);
    glBufferData(target, desc.size, initial_data.empty() ? nullptr : initial_data.data(), usage);

    if (!desc.debug_name.empty()) {
        Debug::set_object_label(GL_BUFFER, buffer_id, desc.debug_name);
    }

    BufferHandle handle = next_buffer_handle_++;
    buffers_[handle] = BufferResource{buffer_id, desc, std::string{desc.debug_name}};

    stats_.gpu_memory_used += desc.size;

    return handle;
}

Result<TextureHandle> Core::create_texture(const TextureDesc& desc, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> initial_data) {
    GLuint texture_id;
    glGenTextures(1, &texture_id);

    if (!texture_id) {
        return RendererError::InitializationFailed;
    }

    GLenum target = desc.array_layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    if (desc.depth > 1) target = GL_TEXTURE_3D;

    glBindTexture(target, texture_id);

    // Convert format
    auto [internal_format, format, type] = Converter::texture_format_to_gl(desc.format);
    if (internal_format == 0) {
        glDeleteTextures(1, &texture_id);
        return RendererError::UnsupportedFormat;
    }

    // Allocate storage (fallback to glTexImage if glTexStorage not available)
    if (target == GL_TEXTURE_2D) {
        #ifdef GL_VERSION_4_2
        if (glTexStorage2D) {
            glTexStorage2D(target, desc.mip_levels, internal_format, desc.width, desc.height);
        } else
        #endif
        {
            for (std::uint32_t level = 0; level < desc.mip_levels; ++level) {
                std::uint32_t mip_width = std::max(1u, desc.width >> level);
                std::uint32_t mip_height = std::max(1u, desc.height >> level);
                glTexImage2D(target, level, internal_format, mip_width, mip_height, 0, format, type, nullptr);
            }
        }
    } else if (target == GL_TEXTURE_2D_ARRAY) {
        #ifdef GL_VERSION_4_2
        if (glTexStorage3D) {
            glTexStorage3D(target, desc.mip_levels, internal_format, desc.width, desc.height, desc.array_layers);
        } else
        #endif
        {
            for (std::uint32_t level = 0; level < desc.mip_levels; ++level) {
                std::uint32_t mip_width = std::max(1u, desc.width >> level);
                std::uint32_t mip_height = std::max(1u, desc.height >> level);
                glTexImage3D(target, level, internal_format, mip_width, mip_height, desc.array_layers, 0, format, type, nullptr);
            }
        }
    } else if (target == GL_TEXTURE_3D) {
        #ifdef GL_VERSION_4_2
        if (glTexStorage3D) {
            glTexStorage3D(target, desc.mip_levels, internal_format, desc.width, desc.height, desc.depth);
        } else
        #endif
        {
            for (std::uint32_t level = 0; level < desc.mip_levels; ++level) {
                std::uint32_t mip_width = std::max(1u, desc.width >> level);
                std::uint32_t mip_height = std::max(1u, desc.height >> level);
                std::uint32_t mip_depth = std::max(1u, desc.depth >> level);
                glTexImage3D(target, level, internal_format, mip_width, mip_height, mip_depth, 0, format, type, nullptr);
            }
        }
    }

    // Upload initial data if provided
    if (!initial_data.empty()) {
        if (target == GL_TEXTURE_2D) {
            glTexSubImage2D(target, 0, 0, 0, desc.width, desc.height, format, type, initial_data.data());
        } else if (target == GL_TEXTURE_2D_ARRAY) {
            glTexSubImage3D(target, 0, 0, 0, 0, desc.width, desc.height, desc.array_layers, format, type, initial_data.data());
        } else if (target == GL_TEXTURE_3D) {
            glTexSubImage3D(target, 0, 0, 0, 0, desc.width, desc.height, desc.depth, format, type, initial_data.data());
        }
    }

    // Generate mipmaps if requested and data was provided
    if (desc.mip_levels > 1 && !initial_data.empty()) {
        glGenerateMipmap(target);
    }

    // Set default sampling parameters
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, desc.mip_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (!desc.debug_name.empty()) {
        Debug::set_object_label(GL_TEXTURE, texture_id, desc.debug_name);
    }

    TextureHandle handle = next_texture_handle_++;
    textures_[handle] = TextureResource{texture_id, desc, std::string{desc.debug_name}};

    // Calculate memory usage (rough estimate)
    std::uint64_t memory_usage = desc.width * desc.height * desc.depth * desc.array_layers;
    memory_usage *= Converter::get_texture_format_size(desc.format);
    memory_usage = (memory_usage * 4) / 3; // Account for mipmaps
    stats_.gpu_memory_used += memory_usage;

    return handle;
}

Result<ShaderHandle> Core::create_shader(const ShaderDesc& desc) {
    GLenum shader_type = Converter::shader_stage_to_gl_type(desc.stage);
    GLuint shader_id = glCreateShader(shader_type);

    if (!shader_id) {
        return RendererError::InitializationFailed;
    }

    // Convert bytecode to source if needed (assuming GLSL source for now)
    std::string source{reinterpret_cast<const char*>(desc.bytecode.data()), desc.bytecode.size()};
    const char* source_ptr = source.c_str();

    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    // Check compilation status
    GLint compiled;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint log_length;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            std::vector<char> log(log_length);
            glGetShaderInfoLog(shader_id, log_length, nullptr, log.data());
            // For now, just use printf since we don't want to include log header here
            printf("OpenGL shader compilation failed: %s\n", log.data());
        }

        glDeleteShader(shader_id);
        return RendererError::CompilationFailed;
    }

    if (!desc.debug_name.empty()) {
        Debug::set_object_label(GL_SHADER, shader_id, desc.debug_name);
    }

    ShaderHandle handle = next_shader_handle_++;
    shaders_[handle] = ShaderResource{shader_id, desc, std::string{desc.debug_name}, std::move(source)};

    return handle;
}

Result<PipelineHandle> Core::create_pipeline(const PipelineDesc& desc) {
    GLuint program = glCreateProgram();
    if (!program) {
        return RendererError::InitializationFailed;
    }

    // Attach shaders
    std::vector<ShaderHandle> shader_handles;
    for (ShaderHandle shader_handle : desc.shaders) {
        auto it = shaders_.find(shader_handle);
        if (it == shaders_.end()) {
            glDeleteProgram(program);
            return RendererError::InvalidHandle;
        }

        glAttachShader(program, it->second.id);
        shader_handles.push_back(shader_handle);
    }

    // Bind vertex attributes
    for (const auto& binding : desc.vertex_bindings) {
        for (const auto& attribute : binding.attributes) {
            glBindAttribLocation(program, attribute.location, attribute.name.data());
        }
    }

    // Link program
    glLinkProgram(program);

    // Check link status
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        if (log_length > 0) {
            std::vector<char> log(log_length);
            glGetProgramInfoLog(program, log_length, nullptr, log.data());
            // Could log the error here
        }

        glDeleteProgram(program);
        return RendererError::CompilationFailed;
    }

    // Bind uniform blocks to binding points (OpenGL 4.1 compatible)
    GLuint uniform_block_index = glGetUniformBlockIndex(program, "GlobalUniforms");
    if (uniform_block_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(program, uniform_block_index, 0); // Bind to binding point 0
        std::cout << "Bound uniform block 'GlobalUniforms' to binding point 0" << std::endl;
    } else {
        std::cout << "WARNING: Could not find uniform block 'GlobalUniforms'" << std::endl;
    }

    // Detach shaders (they're no longer needed)
    for (ShaderHandle shader_handle : shader_handles) {
        auto it = shaders_.find(shader_handle);
        if (it != shaders_.end()) {
            glDetachShader(program, it->second.id);
        }
    }

    if (!desc.debug_name.empty()) {
        Debug::set_object_label(GL_PROGRAM, program, desc.debug_name);
    }

    PipelineHandle handle = next_pipeline_handle_++;
    auto& pipeline_resource = pipelines_[handle] = PipelineResource{program, desc, std::string{desc.debug_name}};
    pipeline_resource.shaders = std::move(shader_handles);

    return handle;
}

Result<RenderTargetHandle> Core::create_render_target(::Manifest::Core::Modern::span<const TextureHandle> color_attachments, TextureHandle depth_attachment) {
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    if (!framebuffer) {
        return RendererError::InitializationFailed;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Attach color textures
    for (std::size_t i = 0; i < color_attachments.size(); ++i) {
        auto it = textures_.find(color_attachments[i]);
        if (it == textures_.end()) {
            glDeleteFramebuffers(1, &framebuffer);
            return RendererError::InvalidHandle;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                              GL_TEXTURE_2D, it->second.id, 0);
    }

    // Attach depth texture if provided
    if (depth_attachment.is_valid()) {
        auto it = textures_.find(depth_attachment);
        if (it == textures_.end()) {
            glDeleteFramebuffers(1, &framebuffer);
            return RendererError::InvalidHandle;
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, it->second.id, 0);
    }

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &framebuffer);
        return RendererError::InitializationFailed;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RenderTargetHandle handle = next_render_target_handle_++;
    auto& rt_resource = render_targets_[handle] = RenderTargetResource{framebuffer};
    rt_resource.color_attachments.assign(color_attachments.begin(), color_attachments.end());
    rt_resource.depth_attachment = depth_attachment;

    return handle;
}

void Core::destroy_buffer(BufferHandle handle) {
    auto it = buffers_.find(handle);
    if (it != buffers_.end()) {
        stats_.gpu_memory_used -= it->second.desc.size;
        if (it->second.id) glDeleteBuffers(1, &it->second.id);
        buffers_.erase(it);
    }
}

void Core::destroy_texture(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        const auto& desc = it->second.desc;
        std::uint64_t memory_usage = desc.width * desc.height * desc.depth * desc.array_layers;
        memory_usage *= Converter::get_texture_format_size(desc.format);
        memory_usage = (memory_usage * 4) / 3;
        stats_.gpu_memory_used -= memory_usage;
        
        if (it->second.id) glDeleteTextures(1, &it->second.id);
        textures_.erase(it);
    }
}

void Core::destroy_shader(ShaderHandle handle) {
    auto it = shaders_.find(handle);
    if (it != shaders_.end()) {
        loader_->unregister_shader(handle);
        if (it->second.id) glDeleteShader(it->second.id);
        shaders_.erase(it);
    }
}

void Core::destroy_pipeline(PipelineHandle handle) {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end()) {
        if (it->second.program) glDeleteProgram(it->second.program);
        pipelines_.erase(it);
    }
}

void Core::destroy_render_target(RenderTargetHandle handle) {
    auto it = render_targets_.find(handle);
    if (it != render_targets_.end()) {
        if (it->second.framebuffer) glDeleteFramebuffers(1, &it->second.framebuffer);
        render_targets_.erase(it);
    }
}

Result<void> Core::update_buffer(BufferHandle handle, std::size_t offset, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> data) {
    auto it = buffers_.find(handle);
    if (it == buffers_.end()) {
        return RendererError::InvalidHandle;
    }

    const auto& resource = it->second;
    if (offset + data.size() > resource.desc.size) {
        return RendererError::InvalidState;
    }

    GLenum target = Converter::buffer_usage_to_gl_target(resource.desc.usage);
    glBindBuffer(target, resource.id);
    glBufferSubData(target, offset, data.size(), data.data());

    return {};
}

Result<void> Core::update_texture(TextureHandle handle, std::uint32_t mip_level, ::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> data) {
    auto it = textures_.find(handle);
    if (it == textures_.end()) {
        return RendererError::InvalidHandle;
    }

    const auto& resource = it->second;
    if (mip_level >= resource.desc.mip_levels) {
        return RendererError::InvalidState;
    }

    GLenum target = resource.desc.array_layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
    if (resource.desc.depth > 1) target = GL_TEXTURE_3D;

    auto [internal_format, format, type] = Converter::texture_format_to_gl(resource.desc.format);

    glBindTexture(target, resource.id);

    std::uint32_t mip_width = std::max(1u, resource.desc.width >> mip_level);
    std::uint32_t mip_height = std::max(1u, resource.desc.height >> mip_level);

    if (target == GL_TEXTURE_2D) {
        glTexSubImage2D(target, mip_level, 0, 0, mip_width, mip_height, format, type, data.data());
    }

    return {};
}

void Core::begin_frame() {
    stats_.reset();
    update_hot_reload();
}

void Core::end_frame() {
    glFlush();
}

void Core::begin_render_pass(RenderTargetHandle target, const Vec4f& clear_color) {
    current_render_target_ = target;

    if (target.is_valid()) {
        auto it = render_targets_.find(target);
        if (it != render_targets_.end()) {
            glBindFramebuffer(GL_FRAMEBUFFER, it->second.framebuffer);
        }
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glClearColor(clear_color.x(), clear_color.y(), clear_color.z(), clear_color.w());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Core::end_render_pass() {
    current_render_target_ = RenderTargetHandle::invalid();
}

void Core::set_viewport(const Viewport& viewport) {
    glViewport(static_cast<GLint>(viewport.position.x()), static_cast<GLint>(viewport.position.y()),
              static_cast<GLsizei>(viewport.size.x()), static_cast<GLsizei>(viewport.size.y()));
    glDepthRange(viewport.depth_range.x(), viewport.depth_range.y());
}

void Core::set_scissor(const Scissor& scissor) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(scissor.offset.x(), scissor.offset.y(), scissor.extent.x(), scissor.extent.y());
}

void Core::bind_pipeline(PipelineHandle pipeline) {
    //==========================================================================
    // CRITICAL: This function was MISSING in original implementation!
    //==========================================================================
    // Without this, shaders never get activated and rendering shows only clear color.
    // This was the key missing piece that caused "dark blue screen" issues.
    //==========================================================================
    
    current_pipeline_ = pipeline;

    auto it = pipelines_.find(pipeline);
    if (it != pipelines_.end()) {
        // CRITICAL: Activate the shader program - without this, default shader is used
        glUseProgram(it->second.program);
        
        // Apply render state (depth testing, culling, etc.)
        apply_render_state(it->second.desc.render_state);
        
        // Note: vertex attributes are set up when buffers are bound
        // This deferred approach prevents VAO state corruption
    }
}

void Core::bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset) {
    auto it = buffers_.find(buffer);
    if (it != buffers_.end()) {
        auto pipeline_it = pipelines_.find(current_pipeline_);
        if (pipeline_it != pipelines_.end()) {
            // Find the vertex binding that matches this binding point
            for (const auto& vertex_binding : pipeline_it->second.desc.vertex_bindings) {
                if (vertex_binding.binding == binding) {
                    glBindBuffer(GL_ARRAY_BUFFER, it->second.id);
                    
                    // Set up vertex attributes for this specific binding
                    for (const auto& attribute : vertex_binding.attributes) {
                        glEnableVertexAttribArray(attribute.location);
                        
                        GLenum type = Converter::attribute_format_to_gl_type(attribute.format);
                        GLint size = Converter::attribute_format_to_gl_size(attribute.format);
                        GLboolean normalized = (type != GL_FLOAT) ? GL_TRUE : GL_FALSE;
                        
                        glVertexAttribPointer(attribute.location, size, type, normalized,
                                            vertex_binding.stride, reinterpret_cast<const void*>(attribute.offset + offset));
                        
                        // Set attribute divisor for instanced rendering
                        if (vertex_binding.input_rate == VertexInputRate::Instance) {
                            glVertexAttribDivisor(attribute.location, 1);  // Advance per instance
                        } else {
                            glVertexAttribDivisor(attribute.location, 0);  // Advance per vertex
                        }
                    }
                    break;
                }
            }
        }
    }
}

void Core::bind_index_buffer(BufferHandle buffer, std::size_t offset) {
    auto it = buffers_.find(buffer);
    if (it != buffers_.end()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, it->second.id);
    }
}

void Core::bind_texture(TextureHandle texture, std::uint32_t binding) {
    auto it = textures_.find(texture);
    if (it != textures_.end()) {
        glActiveTexture(GL_TEXTURE0 + binding);
        
        GLenum target = it->second.desc.array_layers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        if (it->second.desc.depth > 1) target = GL_TEXTURE_3D;
        
        glBindTexture(target, it->second.id);
    }
}

void Core::bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset, std::size_t size) {
    auto it = buffers_.find(buffer);
    if (it != buffers_.end()) {
        if (size == 0) size = it->second.desc.size - offset;
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, it->second.id, offset, size);
    }
}

void Core::push_constants(::Manifest::Core::Modern::span<const ::Manifest::Core::Modern::byte> data,
                         std::uint32_t offset) {
    // OpenGL doesn't have push constants like Vulkan, so we use a uniform buffer
    if (!push_constants_buffer_) return;
    
    glBindBuffer(GL_UNIFORM_BUFFER, push_constants_buffer_);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, data.size(), data.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Core::draw(const DrawCommand& command) {
    auto it = pipelines_.find(current_pipeline_);
    if (it == pipelines_.end()) return;

    GLenum primitive = Converter::primitive_topology_to_gl(it->second.desc.render_state.topology);

    glDrawArraysInstanced(primitive, command.first_vertex, command.vertex_count, command.instance_count);

    stats_.draw_calls++;
    stats_.vertices_rendered += command.vertex_count * command.instance_count;
}

void Core::draw_indexed(const DrawIndexedCommand& command) {
    auto it = pipelines_.find(current_pipeline_);
    if (it == pipelines_.end()) return;

    GLenum primitive = Converter::primitive_topology_to_gl(it->second.desc.render_state.topology);

    glDrawElementsInstancedBaseVertex(
        primitive, command.index_count, GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(command.first_index * sizeof(GLuint)),
        command.instance_count, command.vertex_offset);

    stats_.draw_calls++;
    stats_.vertices_rendered += command.index_count * command.instance_count;
    stats_.triangles_rendered += (command.index_count / 3) * command.instance_count;
}

void Core::dispatch(const ComputeDispatch& dispatch) {
    glDispatchCompute(dispatch.group_count_x, dispatch.group_count_y, dispatch.group_count_z);
}

void Core::set_debug_name(BufferHandle handle, std::string_view name) {
    auto it = buffers_.find(handle);
    if (it != buffers_.end()) {
        it->second.debug_name = name;
        Debug::set_object_label(GL_BUFFER, it->second.id, name);
    }
}

void Core::set_debug_name(TextureHandle handle, std::string_view name) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        it->second.debug_name = name;
        Debug::set_object_label(GL_TEXTURE, it->second.id, name);
    }
}

void Core::set_debug_name(ShaderHandle handle, std::string_view name) {
    auto it = shaders_.find(handle);
    if (it != shaders_.end()) {
        it->second.debug_name = name;
        Debug::set_object_label(GL_SHADER, it->second.id, name);
    }
}

void Core::set_debug_name(PipelineHandle handle, std::string_view name) {
    auto it = pipelines_.find(handle);
    if (it != pipelines_.end()) {
        it->second.debug_name = name;
        Debug::set_object_label(GL_PROGRAM, it->second.program, name);
    }
}

void Core::push_debug_group(std::string_view name) {
    Debug::push_group(name);
}

void Core::pop_debug_group() {
    Debug::pop_group();
}

void Core::insert_debug_marker(std::string_view name) {
    Debug::insert_marker(name);
}

void Core::update_hot_reload() {
    auto changed_shaders = loader_->check_for_changes();

    for (ShaderHandle shader_handle : changed_shaders) {
        reload_shader(shader_handle);
    }
}

void Core::reload_shader(ShaderHandle handle) {
    auto it = shaders_.find(handle);
    if (it == shaders_.end()) return;

    // Recompile shader
    GLuint shader_id = it->second.id;
    const char* source_ptr = it->second.source_code.c_str();

    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    // Check compilation
    GLint compiled;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        // Log compilation error but don't fail completely
        return;
    }

    // Relink all pipelines using this shader
    for (auto& [pipeline_handle, pipeline_resource] : pipelines_) {
        bool uses_shader = std::find(pipeline_resource.shaders.begin(), 
                                   pipeline_resource.shaders.end(), handle) != pipeline_resource.shaders.end();

        if (uses_shader) {
            relink_pipeline(pipeline_handle);
        }
    }
}

void Core::relink_pipeline(PipelineHandle handle) {
    auto it = pipelines_.find(handle);
    if (it == pipelines_.end()) return;

    GLuint program = it->second.program;

    // Detach old shaders
    for (ShaderHandle shader_handle : it->second.shaders) {
        auto shader_it = shaders_.find(shader_handle);
        if (shader_it != shaders_.end()) {
            glDetachShader(program, shader_it->second.id);
        }
    }

    // Attach updated shaders
    for (ShaderHandle shader_handle : it->second.shaders) {
        auto shader_it = shaders_.find(shader_handle);
        if (shader_it != shaders_.end()) {
            glAttachShader(program, shader_it->second.id);
        }
    }

    // Relink
    glLinkProgram(program);

    // Check link status
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        // Log link error
    }

    // Detach shaders again
    for (ShaderHandle shader_handle : it->second.shaders) {
        auto shader_it = shaders_.find(shader_handle);
        if (shader_it != shaders_.end()) {
            glDetachShader(program, shader_it->second.id);
        }
    }
}

void Core::apply_render_state(const RenderState& state) {
    // Blend mode
    switch (state.blend_mode) {
        case BlendMode::None:
            glDisable(GL_BLEND);
            break;
        case BlendMode::Alpha:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case BlendMode::Additive:
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case BlendMode::Multiply:
            glEnable(GL_BLEND);
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
    }

    // Depth test
    if (state.depth_test != DepthTest::Always) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(Converter::depth_test_to_gl(state.depth_test));
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(state.depth_write ? GL_TRUE : GL_FALSE);

    // Cull mode
    if (state.cull_mode != CullMode::None) {
        glEnable(GL_CULL_FACE);
        glCullFace(Converter::cull_mode_to_gl(state.cull_mode));
    } else {
        glDisable(GL_CULL_FACE);
    }

    // Wireframe
    glPolygonMode(GL_FRONT_AND_BACK, state.wireframe ? GL_LINE : GL_FILL);
}

void Core::setup_vertex_attributes(const ::Manifest::Core::Modern::span<const VertexBinding>& bindings) {
    for (const auto& binding : bindings) {
        for (const auto& attribute : binding.attributes) {
            glEnableVertexAttribArray(attribute.location);
            
            GLenum type = Converter::attribute_format_to_gl_type(attribute.format);
            GLint size = Converter::attribute_format_to_gl_size(attribute.format);
            GLboolean normalized = (type != GL_FLOAT) ? GL_TRUE : GL_FALSE;
            
            glVertexAttribPointer(attribute.location, size, type, normalized,
                                binding.stride, reinterpret_cast<const void*>(attribute.offset));
            
            // Set attribute divisor for instanced rendering
            if (binding.input_rate == VertexInputRate::Instance) {
                glVertexAttribDivisor(attribute.location, 1);  // Advance per instance
            } else {
                glVertexAttribDivisor(attribute.location, 0);  // Advance per vertex (default)
            }
        }
    }
}

}  // namespace OpenGL
}  // namespace Render
}  // namespace Manifest

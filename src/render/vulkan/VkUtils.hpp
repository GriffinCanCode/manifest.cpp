#pragma once

#include "../common/Types.hpp"
#include <vulkan/vulkan.hpp>
#include <span>

namespace Manifest::Render::Vulkan {

// Type conversion utilities
class Convert {
public:
    // Format conversions
    static vk::Format to_vk_format(TextureFormat format);
    static TextureFormat from_vk_format(vk::Format format);
    
    // Buffer usage conversions
    static vk::BufferUsageFlags to_vk_buffer_usage(BufferUsage usage);
    static BufferUsage from_vk_buffer_usage(vk::BufferUsageFlags usage);
    
    // Shader stage conversions
    static vk::ShaderStageFlagBits to_vk_shader_stage(ShaderStage stage);
    static ShaderStage from_vk_shader_stage(vk::ShaderStageFlagBits stage);
    
    // Primitive topology conversions
    static vk::PrimitiveTopology to_vk_topology(PrimitiveTopology topology);
    static PrimitiveTopology from_vk_topology(vk::PrimitiveTopology topology);
    
    // Blend operations
    static vk::BlendFactor to_vk_blend_factor(BlendMode mode, bool src);
    static vk::BlendOp to_vk_blend_op(BlendMode mode);
    
    // Cull mode conversions
    static vk::CullModeFlags to_vk_cull_mode(CullMode mode);
    static CullMode from_vk_cull_mode(vk::CullModeFlags mode);
    
    // Depth test conversions
    static vk::CompareOp to_vk_compare_op(DepthTest test);
    static DepthTest from_vk_compare_op(vk::CompareOp op);
    
    // Attribute format conversions
    static vk::Format to_vk_vertex_format(AttributeFormat format);
    static AttributeFormat from_vk_vertex_format(vk::Format format);
    
    // Image layout transitions
    static vk::ImageLayout to_optimal_layout(vk::ImageUsageFlags usage, bool depth_format = false);
    
    // Access mask from layout
    static vk::AccessFlags access_mask_for_layout(vk::ImageLayout layout);
    
    // Pipeline stage from layout
    static vk::PipelineStageFlags pipeline_stage_for_layout(vk::ImageLayout layout);
};

// Helper for image transitions
struct ImageTransition {
    vk::Image image;
    vk::ImageLayout old_layout;
    vk::ImageLayout new_layout;
    vk::ImageSubresourceRange subresource_range;
    
    vk::AccessFlags src_access_mask;
    vk::AccessFlags dst_access_mask;
    vk::PipelineStageFlags src_stage_mask;
    vk::PipelineStageFlags dst_stage_mask;
    
    static ImageTransition create(vk::Image image, 
                                 vk::ImageLayout old_layout, 
                                 vk::ImageLayout new_layout,
                                 vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor,
                                 uint32_t mip_levels = 1,
                                 uint32_t array_layers = 1);
};

// Barrier creation helpers
class Barriers {
public:
    static vk::MemoryBarrier memory(vk::AccessFlags src_access, vk::AccessFlags dst_access);
    
    static vk::BufferMemoryBarrier buffer(vk::Buffer buffer, 
                                         vk::AccessFlags src_access,
                                         vk::AccessFlags dst_access,
                                         vk::DeviceSize offset = 0,
                                         vk::DeviceSize size = VK_WHOLE_SIZE);
    
    static vk::ImageMemoryBarrier image(const ImageTransition& transition);
    
    // Common image transitions
    static vk::ImageMemoryBarrier undefined_to_transfer_dst(vk::Image image, 
                                                           vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor);
    
    static vk::ImageMemoryBarrier transfer_dst_to_shader_read(vk::Image image,
                                                             vk::ImageAspectFlags aspect_mask = vk::ImageAspectFlagBits::eColor);
    
    static vk::ImageMemoryBarrier undefined_to_depth_attachment(vk::Image image);
    
    static vk::ImageMemoryBarrier undefined_to_color_attachment(vk::Image image);
};

// Viewport and scissor helpers
class ViewportHelper {
public:
    static vk::Viewport create(const Viewport& viewport);
    static vk::Rect2D create(const Scissor& scissor);
    
    // Common viewports
    static vk::Viewport fullscreen(uint32_t width, uint32_t height);
    static vk::Rect2D fullscreen_scissor(uint32_t width, uint32_t height);
};

// Command buffer helpers
class Commands {
public:
    // Image layout transition
    static void transition_image_layout(vk::CommandBuffer cmd, const ImageTransition& transition);
    
    // Buffer copy operations
    static void copy_buffer(vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst, 
                           vk::DeviceSize size, vk::DeviceSize src_offset = 0, 
                           vk::DeviceSize dst_offset = 0);
    
    // Buffer to image copy
    static void copy_buffer_to_image(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image image,
                                    uint32_t width, uint32_t height, uint32_t layer = 0);
    
    // Generate mipmaps
    static void generate_mipmaps(vk::CommandBuffer cmd, vk::Image image, vk::Format format,
                                uint32_t width, uint32_t height, uint32_t mip_levels);
    
    // Begin/end render pass helpers
    static void begin_render_pass(vk::CommandBuffer cmd, vk::RenderPass render_pass,
                                 vk::Framebuffer framebuffer, vk::Extent2D extent,
                                 const std::vector<vk::ClearValue>& clear_values = {});
    
    static void end_render_pass(vk::CommandBuffer cmd);
    
    // Pipeline barrier helper
    static void pipeline_barrier(vk::CommandBuffer cmd,
                                vk::PipelineStageFlags src_stage_mask,
                                vk::PipelineStageFlags dst_stage_mask,
                                std::span<const vk::MemoryBarrier> memory_barriers = {},
                                std::span<const vk::BufferMemoryBarrier> buffer_barriers = {},
                                std::span<const vk::ImageMemoryBarrier> image_barriers = {});
};

// Debug naming helpers
class Debug {
public:
    static void set_object_name(vk::Device device, vk::Buffer buffer, std::string_view name);
    static void set_object_name(vk::Device device, vk::Image image, std::string_view name);
    static void set_object_name(vk::Device device, vk::DeviceMemory memory, std::string_view name);
    static void set_object_name(vk::Device device, vk::Pipeline pipeline, std::string_view name);
    static void set_object_name(vk::Device device, vk::RenderPass render_pass, std::string_view name);
    static void set_object_name(vk::Device device, vk::CommandBuffer cmd_buffer, std::string_view name);
    
    // Debug markers
    static void begin_debug_label(vk::CommandBuffer cmd, std::string_view name, 
                                 std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f});
    static void end_debug_label(vk::CommandBuffer cmd);
    static void insert_debug_label(vk::CommandBuffer cmd, std::string_view name,
                                  std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f});

private:
    template<typename T>
    static void set_object_name_impl(vk::Device device, T object, 
                                    vk::ObjectType object_type, std::string_view name);
};

// Format properties queries
class FormatUtils {
public:
    static bool is_depth_format(vk::Format format);
    static bool is_stencil_format(vk::Format format);
    static bool is_depth_stencil_format(vk::Format format);
    
    static uint32_t format_size_bytes(vk::Format format);
    static uint32_t format_component_count(vk::Format format);
    
    static vk::ImageAspectFlags get_aspect_mask(vk::Format format);
};

// Validation and error checking
class Validation {
public:
    static bool check_result(vk::Result result, std::string_view operation);
    static void check_result_throw(vk::Result result, std::string_view operation);
    
    static bool is_format_supported(vk::PhysicalDevice physical_device, vk::Format format,
                                   vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    
    static bool is_extension_available(vk::PhysicalDevice physical_device, std::string_view extension);
    static bool is_layer_available(std::string_view layer);
};

} // namespace Manifest::Render::Vulkan

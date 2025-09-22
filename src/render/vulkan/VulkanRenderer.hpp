#pragma once

#include "../../core/types/Modern.hpp"
#include "../common/Renderer.hpp"
#include "VkCompat.hpp"
#include "VkCore.hpp"
#include "VkDevice.hpp"
#include "VkMemory.hpp"
#include "VkResources.hpp"
#include "VkCommands.hpp"
#include "VkUtils.hpp"
#include "vk_mem_alloc.h"
#include <memory>

namespace Manifest {
namespace Render {
namespace Vulkan {

// Import specific modern C++ compatibility types (avoiding Result conflict)
using Core::Modern::span;
using Core::Modern::byte;

/**
 * Modern Vulkan renderer implementation with modular architecture
 * Features: RAII resource management, thread-safe operations, extensible design
 */
class VulkanRenderer : public Renderer {
    // Core Vulkan objects
    std::unique_ptr<Instance> instance_;
    std::unique_ptr<PhysicalDevice> physical_device_;
    std::unique_ptr<Device> device_;
    std::unique_ptr<CommandManager> command_manager_;
    
    // Memory management with VMA
    VmaAllocator vma_allocator_{VK_NULL_HANDLE};
    std::unique_ptr<MemoryAllocator> memory_allocator_;
    
    // Resource managers
    std::unique_ptr<ShaderManager> shader_manager_;
    std::unique_ptr<PipelineManager> pipeline_manager_;
    std::unique_ptr<TextureManager> texture_manager_;
    std::unique_ptr<BufferManager> buffer_manager_;
    std::unique_ptr<RenderTargetManager> render_target_manager_;
    std::unique_ptr<DescriptorManager> descriptor_manager_;
    
    // Swapchain and presentation (to be added later)
    vk::UniqueSurfaceKHR surface_;
    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchain_images_;
    std::vector<vk::UniqueImageView> swapchain_image_views_;
    uint32_t current_frame_{0};
    
    // Current state tracking
    RenderTargetHandle current_render_target_;
    PipelineHandle current_pipeline_;
    Viewport current_viewport_;
    Scissor current_scissor_;
    
    // Debug and validation
    bool validation_enabled_{false};
    bool initialized_{false};
    
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override { shutdown(); }

    // Renderer interface implementation
    Result<void> initialize() override;
    void shutdown() override;

    Result<BufferHandle> create_buffer(const BufferDesc& desc,
                                     Core::Modern::span<const Core::Modern::byte> initial_data = {}) override;

    Result<TextureHandle> create_texture(const TextureDesc& desc,
                                       Core::Modern::span<const Core::Modern::byte> initial_data = {}) override;

    Result<ShaderHandle> create_shader(const ShaderDesc& desc) override;

    Result<PipelineHandle> create_pipeline(const PipelineDesc& desc) override;

    Result<RenderTargetHandle> create_render_target(
        Core::Modern::span<const TextureHandle> color_attachments,
        TextureHandle depth_attachment = {}) override;

    void destroy_buffer(BufferHandle handle) override;
    void destroy_texture(TextureHandle handle) override;
    void destroy_shader(ShaderHandle handle) override;
    void destroy_pipeline(PipelineHandle handle) override;
    void destroy_render_target(RenderTargetHandle handle) override;

    Result<void> update_buffer(BufferHandle handle, std::size_t offset,
                             Core::Modern::span<const Core::Modern::byte> data) override;

    Result<void> update_texture(TextureHandle handle, std::uint32_t mip_level,
                              Core::Modern::span<const Core::Modern::byte> data) override;

    void begin_frame() override;
    void end_frame() override;

    void begin_render_pass(RenderTargetHandle target,
                         const Vec4f& clear_color = {0.0f, 0.0f, 0.0f, 1.0f}) override;
    void end_render_pass() override;

    void set_viewport(const Viewport& viewport) override;
    void set_scissor(const Scissor& scissor) override;
    void bind_pipeline(PipelineHandle pipeline) override;
    void bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding = 0,
                          std::size_t offset = 0) override;
    void bind_index_buffer(BufferHandle buffer, std::size_t offset = 0) override;
    void bind_texture(TextureHandle texture, std::uint32_t binding) override;
    void bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding,
                           std::size_t offset = 0, std::size_t size = 0) override;

    void draw(const DrawCommand& command) override;
    void draw_indexed(const DrawIndexedCommand& command) override;
    void dispatch(const ComputeDispatch& dispatch) override;

    API get_api() const noexcept override { return API::Vulkan; }
    const RenderStats& get_stats() const noexcept override { return stats_; }
    void reset_stats() noexcept override { stats_.reset(); }

    std::uint64_t get_used_memory() const noexcept override;
    std::uint64_t get_available_memory() const noexcept override;

    void set_debug_name(BufferHandle handle, std::string_view name) override;
    void set_debug_name(TextureHandle handle, std::string_view name) override;
    void set_debug_name(ShaderHandle handle, std::string_view name) override;
    void set_debug_name(PipelineHandle handle, std::string_view name) override;

    void push_debug_group(std::string_view name) override;
    void pop_debug_group() override;
    void insert_debug_marker(std::string_view name) override;
    
private:
    bool create_instance();
    bool select_physical_device();
    bool create_logical_device();
    bool create_allocators();
    bool create_resource_managers();
    bool create_swapchain();
    void cleanup_swapchain();
    bool is_initialized() const { return initialized_; }
};

}  // namespace Vulkan
}  // namespace Render 
}  // namespace Manifest
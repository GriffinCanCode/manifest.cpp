#pragma once

#include "VkMemory.hpp"
#include "VkUtils.hpp"
#include "../common/Types.hpp"
#include <unordered_map>
#include <vector>

namespace Manifest::Render::Vulkan {

// Resource managers for Vulkan objects
class ResourceManager {
protected:
    vk::Device device_;
    
public:
    explicit ResourceManager(vk::Device device) : device_{device} {}
    virtual ~ResourceManager() = default;
};

// Shader module management
class ShaderManager : public ResourceManager {
    struct ShaderResource {
        vk::UniqueShaderModule module;
        ShaderDesc description;
        std::string debug_name;
    };
    
    std::unordered_map<ShaderHandle, ShaderResource> shaders_;
    ShaderHandle next_handle_{ShaderHandle{1}};

public:
    explicit ShaderManager(vk::Device device) : ResourceManager{device} {}
    
    ShaderHandle create(const ShaderDesc& desc);
    void destroy(ShaderHandle handle);
    
    vk::ShaderModule get(ShaderHandle handle) const;
    const ShaderDesc* get_desc(ShaderHandle handle) const;
    
    void set_debug_name(ShaderHandle handle, std::string_view name);
    
    size_t size() const { return shaders_.size(); }
    bool is_valid(ShaderHandle handle) const { return shaders_.find(handle) != shaders_.end(); }
};

// Pipeline management
class PipelineManager : public ResourceManager {
    struct PipelineResource {
        vk::UniquePipeline pipeline;
        vk::UniquePipelineLayout layout;
        PipelineDesc description;
        std::string debug_name;
        std::vector<ShaderHandle> shader_handles;
        vk::UniqueRenderPass render_pass;
    };
    
    std::unordered_map<PipelineHandle, PipelineResource> pipelines_;
    PipelineHandle next_handle_{PipelineHandle{1}};
    
    const ShaderManager* shader_manager_;

public:
    explicit PipelineManager(vk::Device device, const ShaderManager* shader_manager) 
        : ResourceManager{device}, shader_manager_{shader_manager} {}
    
    PipelineHandle create(const PipelineDesc& desc);
    void destroy(PipelineHandle handle);
    
    vk::Pipeline get(PipelineHandle handle) const;
    vk::PipelineLayout get_layout(PipelineHandle handle) const;
    vk::RenderPass get_render_pass(PipelineHandle handle) const;
    const PipelineDesc* get_desc(PipelineHandle handle) const;
    
    void set_debug_name(PipelineHandle handle, std::string_view name);
    
    size_t size() const { return pipelines_.size(); }
    bool is_valid(PipelineHandle handle) const { return pipelines_.find(handle) != pipelines_.end(); }

private:
    vk::UniqueRenderPass create_render_pass(const PipelineDesc& desc);
    vk::UniquePipelineLayout create_pipeline_layout();
};

// Texture and image management
class TextureManager : public ResourceManager {
    struct TextureResource {
        Image image;
        vk::UniqueImageView view;
        vk::UniqueSampler sampler;
        TextureDesc description;
        std::string debug_name;
    };
    
    std::unordered_map<TextureHandle, TextureResource> textures_;
    TextureHandle next_handle_{TextureHandle{1}};
    
    MemoryAllocator* allocator_;
    ImageBuilder image_builder_;

public:
    explicit TextureManager(vk::Device device, MemoryAllocator* allocator)
        : ResourceManager{device}, allocator_{allocator}, image_builder_{device, *allocator} {}
    
    TextureHandle create(const TextureDesc& desc, std::span<const std::byte> initial_data = {});
    void destroy(TextureHandle handle);
    
    vk::Image get_image(TextureHandle handle) const;
    vk::ImageView get_view(TextureHandle handle) const;
    vk::Sampler get_sampler(TextureHandle handle) const;
    const TextureDesc* get_desc(TextureHandle handle) const;
    
    void update(TextureHandle handle, uint32_t mip_level, std::span<const std::byte> data);
    void set_debug_name(TextureHandle handle, std::string_view name);
    
    size_t size() const { return textures_.size(); }
    bool is_valid(TextureHandle handle) const { return textures_.find(handle) != textures_.end(); }

private:
    vk::UniqueImageView create_image_view(vk::Image image, vk::Format format, 
                                         vk::ImageAspectFlags aspect_mask, uint32_t mip_levels);
    vk::UniqueSampler create_sampler();
};

// Buffer management
class BufferManager : public ResourceManager {
    struct BufferResource {
        Buffer buffer;
        BufferDesc description;
        std::string debug_name;
    };
    
    std::unordered_map<BufferHandle, BufferResource> buffers_;
    BufferHandle next_handle_{BufferHandle{1}};
    
    MemoryAllocator* allocator_;
    BufferBuilder buffer_builder_;

public:
    explicit BufferManager(vk::Device device, MemoryAllocator* allocator)
        : ResourceManager{device}, allocator_{allocator}, buffer_builder_{device, *allocator} {}
    
    BufferHandle create(const BufferDesc& desc, std::span<const std::byte> initial_data = {});
    void destroy(BufferHandle handle);
    
    vk::Buffer get(BufferHandle handle) const;
    const BufferDesc* get_desc(BufferHandle handle) const;
    
    void update(BufferHandle handle, size_t offset, std::span<const std::byte> data);
    void set_debug_name(BufferHandle handle, std::string_view name);
    
    size_t size() const { return buffers_.size(); }
    bool is_valid(BufferHandle handle) const { return buffers_.find(handle) != buffers_.end(); }
};

// Render target management
class RenderTargetManager : public ResourceManager {
    struct RenderTargetResource {
        vk::UniqueFramebuffer framebuffer;
        vk::UniqueRenderPass render_pass;
        std::vector<TextureHandle> color_attachments;
        TextureHandle depth_attachment;
        std::string debug_name;
        uint32_t width, height;
    };
    
    std::unordered_map<RenderTargetHandle, RenderTargetResource> render_targets_;
    RenderTargetHandle next_handle_{RenderTargetHandle{1}};
    
    const TextureManager* texture_manager_;

public:
    explicit RenderTargetManager(vk::Device device, const TextureManager* texture_manager)
        : ResourceManager{device}, texture_manager_{texture_manager} {}
    
    RenderTargetHandle create(std::span<const TextureHandle> color_attachments,
                             TextureHandle depth_attachment = {});
    void destroy(RenderTargetHandle handle);
    
    vk::Framebuffer get_framebuffer(RenderTargetHandle handle) const;
    vk::RenderPass get_render_pass(RenderTargetHandle handle) const;
    vk::Extent2D get_extent(RenderTargetHandle handle) const;
    
    void set_debug_name(RenderTargetHandle handle, std::string_view name);
    
    size_t size() const { return render_targets_.size(); }
    bool is_valid(RenderTargetHandle handle) const { return render_targets_.find(handle) != render_targets_.end(); }

private:
    vk::UniqueRenderPass create_render_pass(const std::vector<vk::Format>& color_formats,
                                           std::optional<vk::Format> depth_format);
};

// Descriptor set management for uniforms and textures
class DescriptorManager : public ResourceManager {
    struct DescriptorSetLayout {
        vk::UniqueDescriptorSetLayout layout;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
    };
    
    struct DescriptorSet {
        vk::DescriptorSet set;
        vk::DescriptorPool pool;
    };
    
    std::vector<DescriptorSetLayout> layouts_;
    std::vector<vk::UniqueDescriptorPool> pools_;
    std::vector<DescriptorSet> allocated_sets_;

public:
    explicit DescriptorManager(vk::Device device) : ResourceManager{device} {}
    
    vk::DescriptorSetLayout create_layout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings);
    
    vk::DescriptorSet allocate_set(vk::DescriptorSetLayout layout);
    
    void update_buffer_binding(vk::DescriptorSet set, uint32_t binding, 
                              vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize range);
    
    void update_image_binding(vk::DescriptorSet set, uint32_t binding,
                             vk::ImageView view, vk::Sampler sampler, vk::ImageLayout layout);
    
    void cleanup();

private:
    vk::UniqueDescriptorPool create_pool(uint32_t max_sets);
};

} // namespace Manifest::Render::Vulkan

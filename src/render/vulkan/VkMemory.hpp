#pragma once

#include "VkDevice.hpp"
#include <memory>
#include <unordered_map>

// Custom hash function for vk::DeviceMemory
namespace std {
    template<>
    struct hash<vk::DeviceMemory> {
        std::size_t operator()(const vk::DeviceMemory& memory) const noexcept {
            return std::hash<VkDeviceMemory>{}(static_cast<VkDeviceMemory>(memory));
        }
    };
}

namespace Manifest::Render::Vulkan {

// Memory allocation info
struct Allocation {
    vk::DeviceMemory memory;
    vk::DeviceSize offset{0};
    vk::DeviceSize size{0};
    void* mapped_ptr{nullptr};
    uint32_t memory_type_index{0};
    
    bool is_mapped() const { return mapped_ptr != nullptr; }
};

// Simple memory allocator for Vulkan
class MemoryAllocator {
    struct MemoryBlock {
        vk::UniqueDeviceMemory memory;
        vk::DeviceSize size;
        vk::DeviceSize offset{0};
        uint32_t memory_type_index;
        void* mapped_ptr{nullptr};
        bool is_coherent;
        
        vk::DeviceSize remaining() const { return size - offset; }
    };

    vk::Device device_;
    const Device* device_wrapper_;
    std::vector<std::unique_ptr<MemoryBlock>> blocks_;
    std::unordered_map<vk::DeviceMemory, MemoryBlock*> memory_to_block_;
    
    static constexpr vk::DeviceSize BLOCK_SIZE = 256 * 1024 * 1024; // 256 MB
    static constexpr vk::DeviceSize MIN_ALIGNMENT = 256;

public:
    explicit MemoryAllocator(const Device& device);
    ~MemoryAllocator() = default;

    // Buffer allocation
    Allocation allocate_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, 
                              vk::MemoryPropertyFlags properties);
    
    // Image allocation  
    Allocation allocate_image(vk::DeviceSize size, vk::MemoryRequirements requirements,
                             vk::MemoryPropertyFlags properties);
    
    // Direct allocation for specific requirements
    Allocation allocate(vk::MemoryRequirements requirements, 
                       vk::MemoryPropertyFlags properties);
    
    void deallocate(const Allocation& allocation);
    
    // Memory mapping
    void* map(const Allocation& allocation);
    void unmap(const Allocation& allocation);
    void flush(const Allocation& allocation, vk::DeviceSize offset = 0, 
              vk::DeviceSize size = VK_WHOLE_SIZE);
    void invalidate(const Allocation& allocation, vk::DeviceSize offset = 0,
                   vk::DeviceSize size = VK_WHOLE_SIZE);

    // Statistics
    vk::DeviceSize total_allocated() const;
    vk::DeviceSize total_used() const;

private:
    MemoryBlock* find_suitable_block(vk::DeviceSize size, vk::DeviceSize alignment,
                                    uint32_t memory_type_index);
    
    std::unique_ptr<MemoryBlock> create_block(vk::DeviceSize size, uint32_t memory_type_index);
    
    vk::DeviceSize align_size(vk::DeviceSize size, vk::DeviceSize alignment) const;
};

// RAII wrapper for buffer + allocation
class Buffer {
    vk::UniqueBuffer buffer_;
    Allocation allocation_;
    MemoryAllocator* allocator_;
    vk::DeviceSize size_;

public:
    Buffer() = default;
    Buffer(vk::UniqueBuffer buffer, Allocation allocation, 
           MemoryAllocator* allocator, vk::DeviceSize size)
        : buffer_{std::move(buffer)}, allocation_{allocation}, 
          allocator_{allocator}, size_{size} {}

    ~Buffer() {
        if (allocator_ && allocation_.memory) {
            allocator_->deallocate(allocation_);
        }
    }

    // Move only
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    
    Buffer(Buffer&& other) noexcept
        : buffer_{std::move(other.buffer_)}, allocation_{other.allocation_},
          allocator_{other.allocator_}, size_{other.size_} {
        other.allocator_ = nullptr;
        other.allocation_ = {};
    }
    
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            if (allocator_ && allocation_.memory) {
                allocator_->deallocate(allocation_);
            }
            
            buffer_ = std::move(other.buffer_);
            allocation_ = other.allocation_;
            allocator_ = other.allocator_;
            size_ = other.size_;
            
            other.allocator_ = nullptr;
            other.allocation_ = {};
        }
        return *this;
    }

    vk::Buffer get() const { return buffer_.get(); }
    const Allocation& allocation() const { return allocation_; }
    vk::DeviceSize size() const { return size_; }
    
    void* map() { return allocator_ ? allocator_->map(allocation_) : nullptr; }
    void unmap() { if (allocator_) allocator_->unmap(allocation_); }
    void flush(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) {
        if (allocator_) allocator_->flush(allocation_, offset, size);
    }
    void invalidate(vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE) {
        if (allocator_) allocator_->invalidate(allocation_, offset, size);
    }
    
    operator bool() const { return static_cast<bool>(buffer_); }
};

// RAII wrapper for image + allocation
class Image {
    vk::UniqueImage image_;
    Allocation allocation_;
    MemoryAllocator* allocator_;
    vk::Format format_;
    vk::Extent3D extent_;

public:
    Image() = default;
    Image(vk::UniqueImage image, Allocation allocation, 
          MemoryAllocator* allocator, vk::Format format, vk::Extent3D extent)
        : image_{std::move(image)}, allocation_{allocation}, 
          allocator_{allocator}, format_{format}, extent_{extent} {}

    ~Image() {
        if (allocator_ && allocation_.memory) {
            allocator_->deallocate(allocation_);
        }
    }

    // Move only
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    
    Image(Image&& other) noexcept
        : image_{std::move(other.image_)}, allocation_{other.allocation_},
          allocator_{other.allocator_}, format_{other.format_}, extent_{other.extent_} {
        other.allocator_ = nullptr;
        other.allocation_ = {};
    }
    
    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            if (allocator_ && allocation_.memory) {
                allocator_->deallocate(allocation_);
            }
            
            image_ = std::move(other.image_);
            allocation_ = other.allocation_;
            allocator_ = other.allocator_;
            format_ = other.format_;
            extent_ = other.extent_;
            
            other.allocator_ = nullptr;
            other.allocation_ = {};
        }
        return *this;
    }

    vk::Image get() const { return image_.get(); }
    const Allocation& allocation() const { return allocation_; }
    vk::Format format() const { return format_; }
    const vk::Extent3D& extent() const { return extent_; }
    
    operator bool() const { return static_cast<bool>(image_); }
};

// Buffer creation helper
class BufferBuilder {
    vk::Device device_;
    MemoryAllocator& allocator_;

public:
    BufferBuilder(vk::Device device, MemoryAllocator& allocator)
        : device_{device}, allocator_{allocator} {}

    Buffer create(vk::DeviceSize size, vk::BufferUsageFlags usage, 
                  vk::MemoryPropertyFlags properties) const;
    
    Buffer create_staging(vk::DeviceSize size) const {
        return create(size, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | 
                     vk::MemoryPropertyFlagBits::eHostCoherent);
    }
    
    Buffer create_vertex(vk::DeviceSize size, bool device_local = true) const {
        auto usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        auto properties = device_local ? vk::MemoryPropertyFlagBits::eDeviceLocal :
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        return create(size, usage, properties);
    }
    
    Buffer create_index(vk::DeviceSize size, bool device_local = true) const {
        auto usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        auto properties = device_local ? vk::MemoryPropertyFlagBits::eDeviceLocal :
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        return create(size, usage, properties);
    }
    
    Buffer create_uniform(vk::DeviceSize size) const {
        return create(size, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    }
};

// Image creation helper
class ImageBuilder {
    vk::Device device_;
    MemoryAllocator& allocator_;

public:
    ImageBuilder(vk::Device device, MemoryAllocator& allocator)
        : device_{device}, allocator_{allocator} {}

    Image create(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage,
                 vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
                 vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
                 uint32_t mip_levels = 1, uint32_t array_layers = 1) const;
    
    Image create_2d_texture(uint32_t width, uint32_t height, vk::Format format,
                           uint32_t mip_levels = 1) const {
        return create({width, height, 1}, format,
                     vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                     vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal,
                     vk::ImageTiling::eOptimal, mip_levels);
    }
    
    Image create_render_target(uint32_t width, uint32_t height, vk::Format format) const {
        return create({width, height, 1}, format,
                     vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
    }
    
    Image create_depth_buffer(uint32_t width, uint32_t height, vk::Format format) const {
        return create({width, height, 1}, format,
                     vk::ImageUsageFlagBits::eDepthStencilAttachment);
    }
};

} // namespace Manifest::Render::Vulkan

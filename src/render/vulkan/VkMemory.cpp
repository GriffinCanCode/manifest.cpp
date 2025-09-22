#include "VkMemory.hpp"
#include <iostream>
#include <algorithm>
#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

namespace Manifest::Render::Vulkan {

MemoryAllocator::MemoryAllocator(const Device& device)
    : device_{device.get()}, device_wrapper_{&device} {}

Allocation MemoryAllocator::allocate_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                           vk::MemoryPropertyFlags properties) {
    vk::BufferCreateInfo buffer_info{};
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = vk::SharingMode::eExclusive;

    auto buffer = device_.createBufferUnique(buffer_info).value;
    auto requirements = device_.getBufferMemoryRequirements(buffer.get());
    
    return allocate(requirements, properties);
}

Allocation MemoryAllocator::allocate_image(vk::DeviceSize size, vk::MemoryRequirements requirements,
                                          vk::MemoryPropertyFlags properties) {
    return allocate(requirements, properties);
}

Allocation MemoryAllocator::allocate(vk::MemoryRequirements requirements,
                                    vk::MemoryPropertyFlags properties) {
    uint32_t memory_type = device_wrapper_->find_memory_type(requirements.memoryTypeBits, properties);
    vk::DeviceSize aligned_size = align_size(requirements.size, requirements.alignment);
    
    auto block = find_suitable_block(aligned_size, requirements.alignment, memory_type);
    if (!block) {
        // Create new block
        vk::DeviceSize block_size = std::max(BLOCK_SIZE, aligned_size);
        auto new_block = create_block(block_size, memory_type);
        block = new_block.get();
        blocks_.push_back(std::move(new_block));
    }

    Allocation allocation{};
    allocation.memory = block->memory.get();
    allocation.offset = block->offset;
    allocation.size = aligned_size;
    allocation.memory_type_index = memory_type;
    
    // Update block offset
    block->offset += aligned_size;
    
    return allocation;
}

void MemoryAllocator::deallocate(const Allocation& allocation) {
    // Simple implementation - in a real allocator you'd track free regions
    // For now we just leak the memory until the block is destroyed
    // This is acceptable for a basic implementation
}

void* MemoryAllocator::map(const Allocation& allocation) {
    auto it = memory_to_block_.find(allocation.memory);
    if (it == memory_to_block_.end()) {
        return nullptr;
    }
    
    auto* block = it->second;
    if (!block->mapped_ptr) {
        auto result = device_.mapMemory(allocation.memory, 0, VK_WHOLE_SIZE).value;
        block->mapped_ptr = result;
    }
    
    return static_cast<char*>(block->mapped_ptr) + allocation.offset;
}

void MemoryAllocator::unmap(const Allocation& allocation) {
    auto it = memory_to_block_.find(allocation.memory);
    if (it != memory_to_block_.end() && it->second->mapped_ptr) {
        device_.unmapMemory(allocation.memory);
        it->second->mapped_ptr = nullptr;
    }
}

void MemoryAllocator::flush(const Allocation& allocation, vk::DeviceSize offset, vk::DeviceSize size) {
    auto it = memory_to_block_.find(allocation.memory);
    if (it != memory_to_block_.end() && !it->second->is_coherent) {
        vk::MappedMemoryRange range{};
        range.memory = allocation.memory;
        range.offset = allocation.offset + offset;
        range.size = (size == VK_WHOLE_SIZE) ? allocation.size - offset : size;
        device_.flushMappedMemoryRanges(range);
    }
}

void MemoryAllocator::invalidate(const Allocation& allocation, vk::DeviceSize offset, vk::DeviceSize size) {
    auto it = memory_to_block_.find(allocation.memory);
    if (it != memory_to_block_.end() && !it->second->is_coherent) {
        vk::MappedMemoryRange range{};
        range.memory = allocation.memory;
        range.offset = allocation.offset + offset;
        range.size = (size == VK_WHOLE_SIZE) ? allocation.size - offset : size;
        device_.invalidateMappedMemoryRanges(range);
    }
}

vk::DeviceSize MemoryAllocator::total_allocated() const {
    vk::DeviceSize total = 0;
    for (const auto& block : blocks_) {
        total += block->size;
    }
    return total;
}

vk::DeviceSize MemoryAllocator::total_used() const {
    vk::DeviceSize used = 0;
    for (const auto& block : blocks_) {
        used += block->offset;
    }
    return used;
}

MemoryAllocator::MemoryBlock* MemoryAllocator::find_suitable_block(vk::DeviceSize size, 
                                                                   vk::DeviceSize alignment,
                                                                   uint32_t memory_type_index) {
    for (auto& block : blocks_) {
        if (block->memory_type_index != memory_type_index) continue;
        
        vk::DeviceSize aligned_offset = align_size(block->offset, alignment);
        vk::DeviceSize required_size = aligned_offset - block->offset + size;
        
        if (block->remaining() >= required_size) {
            block->offset = aligned_offset;
            return block.get();
        }
    }
    return nullptr;
}

std::unique_ptr<MemoryAllocator::MemoryBlock> MemoryAllocator::create_block(vk::DeviceSize size,
                                                                            uint32_t memory_type_index) {
    vk::MemoryAllocateInfo alloc_info{};
    alloc_info.allocationSize = size;
    alloc_info.memoryTypeIndex = memory_type_index;

    auto block = std::make_unique<MemoryBlock>();
    block->memory = device_.allocateMemoryUnique(alloc_info).value;
    block->size = size;
    block->memory_type_index = memory_type_index;
    
    // Check if memory type is coherent
    const auto& mem_properties = device_wrapper_->physical_info().memory_properties;
    block->is_coherent = (mem_properties.memoryTypes[memory_type_index].propertyFlags & 
                         vk::MemoryPropertyFlagBits::eHostCoherent) != vk::MemoryPropertyFlags{};

    memory_to_block_[block->memory.get()] = block.get();
    return block;
}

vk::DeviceSize MemoryAllocator::align_size(vk::DeviceSize size, vk::DeviceSize alignment) const {
    return (size + alignment - 1) & ~(alignment - 1);
}

Buffer BufferBuilder::create(vk::DeviceSize size, vk::BufferUsageFlags usage,
                            vk::MemoryPropertyFlags properties) const {
    try {
        vk::BufferCreateInfo buffer_info{};
        buffer_info.size = size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = vk::SharingMode::eExclusive;

        auto buffer = device_.createBufferUnique(buffer_info).value;
        auto requirements = device_.getBufferMemoryRequirements(buffer.get());
        
        auto allocation = allocator_.allocate(requirements, properties);
        device_.bindBufferMemory(buffer.get(), allocation.memory, allocation.offset);
        
        return Buffer{std::move(buffer), allocation, &allocator_, size};
    } catch (const std::exception& e) {
        std::cerr << "Failed to create buffer: " << e.what() << std::endl;
        return {};
    }
}

Image ImageBuilder::create(vk::Extent3D extent, vk::Format format, vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties, vk::ImageTiling tiling,
                          uint32_t mip_levels, uint32_t array_layers) const {
    try {
        vk::ImageCreateInfo image_info{};
        image_info.imageType = vk::ImageType::e2D;
        image_info.extent = extent;
        image_info.mipLevels = mip_levels;
        image_info.arrayLayers = array_layers;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = vk::ImageLayout::eUndefined;
        image_info.usage = usage;
        image_info.samples = vk::SampleCountFlagBits::e1;
        image_info.sharingMode = vk::SharingMode::eExclusive;

        auto image = device_.createImageUnique(image_info).value;
        auto requirements = device_.getImageMemoryRequirements(image.get());
        
        auto allocation = allocator_.allocate(requirements, properties);
        device_.bindImageMemory(image.get(), allocation.memory, allocation.offset);
        
        return Image{std::move(image), allocation, &allocator_, format, extent};
    } catch (const std::exception& e) {
        std::cerr << "Failed to create image: " << e.what() << std::endl;
        return {};
    }
}

} // namespace Manifest::Render::Vulkan

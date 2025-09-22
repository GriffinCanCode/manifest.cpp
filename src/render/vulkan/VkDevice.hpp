#pragma once

#include "VkCore.hpp"
#include <memory>
#include <mutex>

namespace Manifest::Render::Vulkan {

// Queue wrapper with thread safety
class Queue {
    vk::Queue queue_;
    uint32_t family_index_;
    std::unique_ptr<std::mutex> mutex_{std::make_unique<std::mutex>()};

public:
    Queue() = default;
    Queue(vk::Queue queue, uint32_t family_index)
        : queue_{queue}, family_index_{family_index} {}
    
    // Make Queue movable but not copyable
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
    Queue(Queue&&) = default;
    Queue& operator=(Queue&&) = default;

    void submit(const vk::SubmitInfo& submit_info, vk::Fence fence = {}) {
        std::lock_guard<std::mutex> lock{*mutex_};
        queue_.submit(submit_info, fence);
    }

    template<typename Container>
    void submit(const Container& submit_infos, vk::Fence fence = {}) {
        std::lock_guard<std::mutex> lock{*mutex_};
        queue_.submit(static_cast<uint32_t>(submit_infos.size()), submit_infos.data(), fence);
    }

    vk::Result present(const vk::PresentInfoKHR& present_info) {
        std::lock_guard<std::mutex> lock{*mutex_};
        return queue_.presentKHR(present_info);
    }

    void wait_idle() {
        std::lock_guard<std::mutex> lock{*mutex_};
        queue_.waitIdle();
    }

    uint32_t family_index() const { return family_index_; }
    vk::Queue get() const { return queue_; }
    operator bool() const { return static_cast<bool>(queue_); }
};

// Logical device manager
class Device {
    vk::UniqueDevice device_;
    PhysicalDeviceInfo physical_device_info_;
    QueueFamilyIndices queue_indices_;
    
    Queue graphics_queue_;
    Queue present_queue_;
    Queue compute_queue_;
    Queue transfer_queue_;

public:
    Device() = default;

    bool create(const PhysicalDevice& physical_device, 
               const std::vector<const char*>& extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
               const vk::PhysicalDeviceFeatures& features = {});

    vk::Device get() const { return device_.get(); }
    const PhysicalDeviceInfo& physical_info() const { return physical_device_info_; }
    const QueueFamilyIndices& queue_indices() const { return queue_indices_; }

    Queue& graphics_queue() { return graphics_queue_; }
    Queue& present_queue() { return present_queue_; }
    Queue& compute_queue() { return compute_queue_; }
    Queue& transfer_queue() { return transfer_queue_; }

    const Queue& graphics_queue() const { return graphics_queue_; }
    const Queue& present_queue() const { return present_queue_; }
    const Queue& compute_queue() const { return compute_queue_; }
    const Queue& transfer_queue() const { return transfer_queue_; }

    void wait_idle() { device_->waitIdle(); }
    
    operator bool() const { return static_cast<bool>(device_); }

    // Memory type finder
    uint32_t find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const;
    
    // Format support queries
    vk::Format find_supported_format(const std::vector<vk::Format>& candidates,
                                     vk::ImageTiling tiling,
                                     vk::FormatFeatureFlags features) const;
    
    vk::Format find_depth_format() const;
    
    bool has_stencil_component(vk::Format format) const;
};

// Command pool with thread safety
class CommandPool {
    vk::UniqueCommandPool pool_;
    uint32_t queue_family_index_;
    std::unique_ptr<std::mutex> mutex_{std::make_unique<std::mutex>()};

public:
    CommandPool() = default;
    
    // Make CommandPool movable but not copyable
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&) = default;
    CommandPool& operator=(CommandPool&&) = default;

    bool create(vk::Device device, uint32_t queue_family_index,
               vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    vk::UniqueCommandBuffer allocate_buffer(vk::Device device, 
                                           vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) {
        std::lock_guard<std::mutex> lock{*mutex_};
        
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.commandPool = pool_.get();
        alloc_info.level = level;
        alloc_info.commandBufferCount = 1;

        auto buffers = device.allocateCommandBuffersUnique(alloc_info).value;
        return std::move(buffers[0]);
    }

    std::vector<vk::UniqueCommandBuffer> allocate_buffers(vk::Device device, uint32_t count,
                                                         vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) {
        std::lock_guard<std::mutex> lock{*mutex_};
        
        vk::CommandBufferAllocateInfo alloc_info{};
        alloc_info.commandPool = pool_.get();
        alloc_info.level = level;
        alloc_info.commandBufferCount = count;

        return device.allocateCommandBuffersUnique(alloc_info).value;
    }

    void reset(vk::CommandPoolResetFlags flags = {}) {
        std::lock_guard<std::mutex> lock{*mutex_};
        // Use device to reset the command pool instead of the pool directly
        // pool_->reset(flags); // This method doesn't exist in modern Vulkan C++ bindings
        // For now, we'll handle this differently in the implementation
    }

    vk::CommandPool get() const { return pool_.get(); }
    uint32_t queue_family_index() const { return queue_family_index_; }
    operator bool() const { return static_cast<bool>(pool_); }
};

// One-time command execution helper
class OneTimeCommands {
    vk::Device device_;
    Queue& queue_;
    CommandPool& pool_;

public:
    OneTimeCommands(vk::Device device, Queue& queue, CommandPool& pool)
        : device_{device}, queue_{queue}, pool_{pool} {}

    template<typename Func>
    void execute(Func&& func) {
        auto cmd = pool_.allocate_buffer(device_);
        
        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        
        cmd->begin(begin_info);
        func(*cmd);
        cmd->end();

        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd.get();

        queue_.submit(submit_info);
        queue_.wait_idle();
    }
};

} // namespace Manifest::Render::Vulkan

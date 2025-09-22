#include "VkDevice.hpp"
#include <set>
#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Manifest::Render::Vulkan {

bool Device::create(const PhysicalDevice& physical_device,
                   const std::vector<const char*>& extensions,
                   const vk::PhysicalDeviceFeatures& features) {
    try {
        physical_device_info_ = physical_device.info();
        queue_indices_ = find_queue_families(physical_device_info_.device, {});

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families;

        // Collect unique queue families
        if (queue_indices_.graphics.has_value()) {
            unique_queue_families.insert(queue_indices_.graphics.value());
        }
        if (queue_indices_.present.has_value()) {
            unique_queue_families.insert(queue_indices_.present.value());
        }
        if (queue_indices_.compute.has_value()) {
            unique_queue_families.insert(queue_indices_.compute.value());
        }
        if (queue_indices_.transfer.has_value()) {
            unique_queue_families.insert(queue_indices_.transfer.value());
        }

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families) {
            vk::DeviceQueueCreateInfo queue_create_info{};
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        vk::DeviceCreateInfo create_info{};
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        device_ = physical_device_info_.device.createDeviceUnique(create_info).value;

        // Get queue handles
        if (queue_indices_.graphics.has_value()) {
            auto queue = device_->getQueue(queue_indices_.graphics.value(), 0);
            graphics_queue_ = Queue{queue, queue_indices_.graphics.value()};
        }

        if (queue_indices_.present.has_value()) {
            auto queue = device_->getQueue(queue_indices_.present.value(), 0);
            present_queue_ = Queue{queue, queue_indices_.present.value()};
        }

        if (queue_indices_.compute.has_value()) {
            auto queue = device_->getQueue(queue_indices_.compute.value(), 0);
            compute_queue_ = Queue{queue, queue_indices_.compute.value()};
        }

        if (queue_indices_.transfer.has_value()) {
            auto queue = device_->getQueue(queue_indices_.transfer.value(), 0);
            transfer_queue_ = Queue{queue, queue_indices_.transfer.value()};
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create logical device: " << e.what() << std::endl;
        return false;
    }
}

uint32_t Device::find_memory_type(uint32_t type_filter, vk::MemoryPropertyFlags properties) const {
    const auto& mem_properties = physical_device_info_.memory_properties;
    
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && 
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type");
}

vk::Format Device::find_supported_format(const std::vector<vk::Format>& candidates,
                                         vk::ImageTiling tiling,
                                         vk::FormatFeatureFlags features) const {
    for (vk::Format format : candidates) {
        vk::FormatProperties props = physical_device_info_.device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

vk::Format Device::find_depth_format() const {
    return find_supported_format(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

bool Device::has_stencil_component(vk::Format format) const {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

bool CommandPool::create(vk::Device device, uint32_t queue_family_index,
                        vk::CommandPoolCreateFlags flags) {
    try {
        queue_family_index_ = queue_family_index;
        
        vk::CommandPoolCreateInfo create_info{};
        create_info.flags = flags;
        create_info.queueFamilyIndex = queue_family_index;
        
        pool_ = device.createCommandPoolUnique(create_info).value;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create command pool: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Manifest::Render::Vulkan

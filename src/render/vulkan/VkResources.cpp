#include "VkResources.hpp"
#include "VkDevice.hpp"
#include <iostream>
#include <vulkan/vulkan.hpp>

namespace Manifest::Render::Vulkan {

ShaderHandle ShaderManager::create(const ShaderDesc& desc) {
    try {
        vk::ShaderModuleCreateInfo create_info{};
        create_info.codeSize = desc.bytecode.size();
        create_info.pCode = reinterpret_cast<const uint32_t*>(desc.bytecode.data());
        
        auto module = device_.createShaderModuleUnique(create_info).value;
        
        // Debug naming will be implemented later
        // if (!desc.debug_name.empty()) {
        //     Debug::set_object_name(device_, module.get(), desc.debug_name);
        // }
        
        ShaderHandle handle = next_handle_++;
        shaders_[handle] = ShaderResource{
            .module = std::move(module),
            .description = desc,
            .debug_name = std::string{desc.debug_name}
        };
        
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create shader: " << e.what() << std::endl;
        return {};
    }
}

void ShaderManager::destroy(ShaderHandle handle) {
    shaders_.erase(handle);
}

vk::ShaderModule ShaderManager::get(ShaderHandle handle) const {
    auto it = shaders_.find(handle);
    return it != shaders_.end() ? it->second.module.get() : vk::ShaderModule{};
}

const ShaderDesc* ShaderManager::get_desc(ShaderHandle handle) const {
    auto it = shaders_.find(handle);
    return it != shaders_.end() ? &it->second.description : nullptr;
}

void ShaderManager::set_debug_name(ShaderHandle handle, std::string_view name) {
    auto it = shaders_.find(handle);
    if (it != shaders_.end()) {
        it->second.debug_name = name;
        // Debug naming will be implemented later
        // Debug::set_object_name(device_, it->second.module.get(), name);
    }
}

PipelineHandle PipelineManager::create(const PipelineDesc& desc) {
    try {
        // Create render pass
        auto render_pass = create_render_pass(desc);
        
        // Create pipeline layout
        auto layout = create_pipeline_layout();
        
        // Collect shader stages
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        std::vector<ShaderHandle> shader_handles{desc.shaders.begin(), desc.shaders.end()};
        
        for (auto shader_handle : desc.shaders) {
            if (!shader_manager_->is_valid(shader_handle)) {
                throw std::runtime_error("Invalid shader handle");
            }
            
            auto shader_desc = shader_manager_->get_desc(shader_handle);
            vk::PipelineShaderStageCreateInfo stage_info{};
            stage_info.stage = Convert::to_vk_shader_stage(shader_desc->stage);
            stage_info.module = shader_manager_->get(shader_handle);
            stage_info.pName = shader_desc->entry_point.data();
            
            shader_stages.push_back(stage_info);
        }
        
        // Vertex input
        std::vector<vk::VertexInputBindingDescription> binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> attribute_descriptions;
        
        for (const auto& binding : desc.vertex_bindings) {
            vk::VertexInputBindingDescription bind_desc{};
            bind_desc.binding = binding.binding;
            bind_desc.stride = binding.stride;
            bind_desc.inputRate = vk::VertexInputRate::eVertex;
            binding_descriptions.push_back(bind_desc);
            
            for (const auto& attribute : binding.attributes) {
                vk::VertexInputAttributeDescription attr_desc{};
                attr_desc.binding = binding.binding;
                attr_desc.location = attribute.location;
                attr_desc.format = Convert::to_vk_vertex_format(attribute.format);
                attr_desc.offset = attribute.offset;
                attribute_descriptions.push_back(attr_desc);
            }
        }
        
        vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = binding_descriptions.data();
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
        
        // Input assembly
        vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.topology = Convert::to_vk_topology(desc.render_state.topology);
        input_assembly.primitiveRestartEnable = VK_FALSE;
        
        // Viewport state (dynamic)
        vk::PipelineViewportStateCreateInfo viewport_state{};
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;
        
        // Rasterization
        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = desc.render_state.wireframe ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = Convert::to_vk_cull_mode(desc.render_state.cull_mode);
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        // Multisampling
        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        
        // Depth/stencil testing
        vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.depthTestEnable = desc.render_state.depth_test != DepthTest::Always;
        depth_stencil.depthWriteEnable = desc.render_state.depth_write;
        depth_stencil.depthCompareOp = Convert::to_vk_compare_op(desc.render_state.depth_test);
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.stencilTestEnable = VK_FALSE;
        
        // Color blending
        vk::PipelineColorBlendAttachmentState color_blend_attachment{};
        if (desc.render_state.blend_mode != BlendMode::None) {
            color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | 
                                                   vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | 
                                                   vk::ColorComponentFlagBits::eA;
            color_blend_attachment.blendEnable = VK_TRUE;
            color_blend_attachment.srcColorBlendFactor = Convert::to_vk_blend_factor(desc.render_state.blend_mode, true);
            color_blend_attachment.dstColorBlendFactor = Convert::to_vk_blend_factor(desc.render_state.blend_mode, false);
            color_blend_attachment.colorBlendOp = Convert::to_vk_blend_op(desc.render_state.blend_mode);
            color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
        } else {
            color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | 
                                                   vk::ColorComponentFlagBits::eG |
                                                   vk::ColorComponentFlagBits::eB | 
                                                   vk::ColorComponentFlagBits::eA;
            color_blend_attachment.blendEnable = VK_FALSE;
        }
        
        vk::PipelineColorBlendStateCreateInfo color_blending{};
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        
        // Dynamic state
        std::vector<vk::DynamicState> dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };
        
        vk::PipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();
        
        // Create pipeline
        vk::GraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = layout.get();
        pipeline_info.renderPass = render_pass.get();
        pipeline_info.subpass = 0;
        
        auto pipeline = device_.createGraphicsPipelineUnique({}, pipeline_info).value;
        
        // Debug naming will be implemented later
        // if (!desc.debug_name.empty()) {
        //     Debug::set_object_name(device_, pipeline.get(), desc.debug_name);
        // }
        
        PipelineHandle handle = next_handle_++;
        pipelines_[handle] = PipelineResource{
            .pipeline = std::move(pipeline),
            .layout = std::move(layout),
            .description = desc,
            .debug_name = std::string{desc.debug_name},
            .shader_handles = std::move(shader_handles),
            .render_pass = std::move(render_pass)
        };
        
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create pipeline: " << e.what() << std::endl;
        return {};
    }
}

void PipelineManager::destroy(PipelineHandle handle) {
    pipelines_.erase(handle);
}

vk::Pipeline PipelineManager::get(PipelineHandle handle) const {
    auto it = pipelines_.find(handle);
    return it != pipelines_.end() ? it->second.pipeline.get() : vk::Pipeline{};
}

vk::PipelineLayout PipelineManager::get_layout(PipelineHandle handle) const {
    auto it = pipelines_.find(handle);
    return it != pipelines_.end() ? it->second.layout.get() : vk::PipelineLayout{};
}

vk::RenderPass PipelineManager::get_render_pass(PipelineHandle handle) const {
    auto it = pipelines_.find(handle);
    return it != pipelines_.end() ? it->second.render_pass.get() : vk::RenderPass{};
}

vk::UniqueRenderPass PipelineManager::create_render_pass(const PipelineDesc& desc) {
    // Simple render pass with one color attachment
    vk::AttachmentDescription color_attachment{};
    color_attachment.format = vk::Format::eR8G8B8A8Unorm; // Default format
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    
    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
    
    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    
    vk::SubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    
    vk::RenderPassCreateInfo render_pass_info{};
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;
    
    return device_.createRenderPassUnique(render_pass_info).value;
}

vk::UniquePipelineLayout PipelineManager::create_pipeline_layout() {
    // Simple pipeline layout with no descriptors
    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    return device_.createPipelineLayoutUnique(pipeline_layout_info).value;
}

TextureHandle TextureManager::create(const TextureDesc& desc, std::span<const std::byte> initial_data) {
    try {
        auto image = image_builder_.create(
            {desc.width, desc.height, desc.depth},
            Convert::to_vk_format(desc.format),
            desc.render_target ? vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled :
                                vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::ImageTiling::eOptimal,
            desc.mip_levels,
            desc.array_layers
        );
        
        auto view = create_image_view(image.get(), Convert::to_vk_format(desc.format), 
                                     FormatUtils::get_aspect_mask(Convert::to_vk_format(desc.format)), 
                                     desc.mip_levels);
        
        auto sampler = create_sampler();
        
        // Debug naming will be implemented later
        // if (!desc.debug_name.empty()) {
        //     Debug::set_object_name(device_, image.get(), desc.debug_name);
        // }
        
        TextureHandle handle = next_handle_++;
        textures_[handle] = TextureResource{
            .image = std::move(image),
            .view = std::move(view),
            .sampler = std::move(sampler),
            .description = desc,
            .debug_name = std::string{desc.debug_name}
        };
        
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create texture: " << e.what() << std::endl;
        return {};
    }
}

void TextureManager::destroy(TextureHandle handle) {
    textures_.erase(handle);
}

vk::Image TextureManager::get_image(TextureHandle handle) const {
    auto it = textures_.find(handle);
    return it != textures_.end() ? it->second.image.get() : vk::Image{};
}

vk::ImageView TextureManager::get_view(TextureHandle handle) const {
    auto it = textures_.find(handle);
    return it != textures_.end() ? it->second.view.get() : vk::ImageView{};
}

vk::Sampler TextureManager::get_sampler(TextureHandle handle) const {
    auto it = textures_.find(handle);
    return it != textures_.end() ? it->second.sampler.get() : vk::Sampler{};
}

const TextureDesc* TextureManager::get_desc(TextureHandle handle) const {
    auto it = textures_.find(handle);
    return it != textures_.end() ? &it->second.description : nullptr;
}

vk::UniqueImageView TextureManager::create_image_view(vk::Image image, vk::Format format,
                                                     vk::ImageAspectFlags aspect_mask, uint32_t mip_levels) {
    vk::ImageViewCreateInfo view_info{};
    view_info.image = image;
    view_info.viewType = vk::ImageViewType::e2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    
    return device_.createImageViewUnique(view_info).value;
}

vk::UniqueSampler TextureManager::create_sampler() {
    vk::SamplerCreateInfo sampler_info{};
    sampler_info.magFilter = vk::Filter::eLinear;
    sampler_info.minFilter = vk::Filter::eLinear;
    sampler_info.addressModeU = vk::SamplerAddressMode::eRepeat;
    sampler_info.addressModeV = vk::SamplerAddressMode::eRepeat;
    sampler_info.addressModeW = vk::SamplerAddressMode::eRepeat;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16.0f;
    sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = vk::CompareOp::eAlways;
    sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    
    return device_.createSamplerUnique(sampler_info).value;
}

BufferHandle BufferManager::create(const BufferDesc& desc, std::span<const std::byte> initial_data) {
    try {
        auto buffer = buffer_builder_.create(
            desc.size,
            Convert::to_vk_buffer_usage(desc.usage) | vk::BufferUsageFlagBits::eTransferDst,
            desc.host_visible ? 
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent :
                vk::MemoryPropertyFlagBits::eDeviceLocal
        );
        
        // Upload initial data if provided
        if (!initial_data.empty() && initial_data.size() <= desc.size) {
            if (desc.host_visible) {
                // Direct copy to host-visible buffer
                if (auto* mapped = buffer.map()) {
                    std::memcpy(mapped, initial_data.data(), initial_data.size());
                    buffer.flush();
                    buffer.unmap();
                }
            } else {
                // TODO: Use staging buffer for device-local buffer
                std::cerr << "Warning: Initial data upload to device-local buffer not implemented" << std::endl;
            }
        }
        
        // Debug naming will be implemented later
        // if (!desc.debug_name.empty()) {
        //     Debug::set_object_name(device_, buffer.get(), desc.debug_name);
        // }
        
        BufferHandle handle = next_handle_++;
        buffers_[handle] = BufferResource{
            .buffer = std::move(buffer),
            .description = desc,
            .debug_name = std::string{desc.debug_name}
        };
        
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create buffer: " << e.what() << std::endl;
        return {};
    }
}

void BufferManager::destroy(BufferHandle handle) {
    buffers_.erase(handle);
}

vk::Buffer BufferManager::get(BufferHandle handle) const {
    auto it = buffers_.find(handle);
    return it != buffers_.end() ? it->second.buffer.get() : vk::Buffer{};
}

const BufferDesc* BufferManager::get_desc(BufferHandle handle) const {
    auto it = buffers_.find(handle);
    return it != buffers_.end() ? &it->second.description : nullptr;
}

void BufferManager::update(BufferHandle handle, size_t offset, std::span<const std::byte> data) {
    auto it = buffers_.find(handle);
    if (it != buffers_.end() && it->second.description.host_visible) {
        if (auto* mapped = it->second.buffer.map()) {
            std::memcpy(static_cast<char*>(mapped) + offset, data.data(), data.size());
            it->second.buffer.flush(offset, data.size());
            it->second.buffer.unmap();
        }
    }
}

} // namespace Manifest::Render::Vulkan

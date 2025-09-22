#include "VkUtils.hpp"
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

namespace Manifest::Render::Vulkan {

vk::Format Convert::to_vk_format(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8_UNORM: return vk::Format::eR8Unorm;
        case TextureFormat::RG8_UNORM: return vk::Format::eR8G8Unorm;
        case TextureFormat::RGB8_UNORM: return vk::Format::eR8G8B8Unorm;
        case TextureFormat::RGBA8_UNORM: return vk::Format::eR8G8B8A8Unorm;
        case TextureFormat::R16_FLOAT: return vk::Format::eR16Sfloat;
        case TextureFormat::RG16_FLOAT: return vk::Format::eR16G16Sfloat;
        case TextureFormat::RGB16_FLOAT: return vk::Format::eR16G16B16Sfloat;
        case TextureFormat::RGBA16_FLOAT: return vk::Format::eR16G16B16A16Sfloat;
        case TextureFormat::R32_FLOAT: return vk::Format::eR32Sfloat;
        case TextureFormat::RG32_FLOAT: return vk::Format::eR32G32Sfloat;
        case TextureFormat::RGB32_FLOAT: return vk::Format::eR32G32B32Sfloat;
        case TextureFormat::RGBA32_FLOAT: return vk::Format::eR32G32B32A32Sfloat;
        case TextureFormat::D16_UNORM: return vk::Format::eD16Unorm;
        case TextureFormat::D24_UNORM_S8_UINT: return vk::Format::eD24UnormS8Uint;
        case TextureFormat::D32_FLOAT: return vk::Format::eD32Sfloat;
        default: return vk::Format::eUndefined;
    }
}

TextureFormat Convert::from_vk_format(vk::Format format) {
    switch (format) {
        case vk::Format::eR8Unorm: return TextureFormat::R8_UNORM;
        case vk::Format::eR8G8Unorm: return TextureFormat::RG8_UNORM;
        case vk::Format::eR8G8B8Unorm: return TextureFormat::RGB8_UNORM;
        case vk::Format::eR8G8B8A8Unorm: return TextureFormat::RGBA8_UNORM;
        case vk::Format::eR16Sfloat: return TextureFormat::R16_FLOAT;
        case vk::Format::eR16G16Sfloat: return TextureFormat::RG16_FLOAT;
        case vk::Format::eR16G16B16Sfloat: return TextureFormat::RGB16_FLOAT;
        case vk::Format::eR16G16B16A16Sfloat: return TextureFormat::RGBA16_FLOAT;
        case vk::Format::eR32Sfloat: return TextureFormat::R32_FLOAT;
        case vk::Format::eR32G32Sfloat: return TextureFormat::RG32_FLOAT;
        case vk::Format::eR32G32B32Sfloat: return TextureFormat::RGB32_FLOAT;
        case vk::Format::eR32G32B32A32Sfloat: return TextureFormat::RGBA32_FLOAT;
        case vk::Format::eD16Unorm: return TextureFormat::D16_UNORM;
        case vk::Format::eD24UnormS8Uint: return TextureFormat::D24_UNORM_S8_UINT;
        case vk::Format::eD32Sfloat: return TextureFormat::D32_FLOAT;
        default: return TextureFormat::RGBA8_UNORM; // fallback
    }
}

vk::BufferUsageFlags Convert::to_vk_buffer_usage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::Vertex: return vk::BufferUsageFlagBits::eVertexBuffer;
        case BufferUsage::Index: return vk::BufferUsageFlagBits::eIndexBuffer;
        case BufferUsage::Uniform: return vk::BufferUsageFlagBits::eUniformBuffer;
        case BufferUsage::Storage: return vk::BufferUsageFlagBits::eStorageBuffer;
        case BufferUsage::Staging: return vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        default: return vk::BufferUsageFlagBits::eVertexBuffer;
    }
}

vk::ShaderStageFlagBits Convert::to_vk_shader_stage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Geometry: return vk::ShaderStageFlagBits::eGeometry;
        case ShaderStage::TessellationControl: return vk::ShaderStageFlagBits::eTessellationControl;
        case ShaderStage::TessellationEvaluation: return vk::ShaderStageFlagBits::eTessellationEvaluation;
        case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
        default: return vk::ShaderStageFlagBits::eVertex;
    }
}

vk::PrimitiveTopology Convert::to_vk_topology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::Points: return vk::PrimitiveTopology::ePointList;
        case PrimitiveTopology::Lines: return vk::PrimitiveTopology::eLineList;
        case PrimitiveTopology::LineStrip: return vk::PrimitiveTopology::eLineStrip;
        case PrimitiveTopology::Triangles: return vk::PrimitiveTopology::eTriangleList;
        case PrimitiveTopology::TriangleStrip: return vk::PrimitiveTopology::eTriangleStrip;
        case PrimitiveTopology::TriangleFan: return vk::PrimitiveTopology::eTriangleFan;
        default: return vk::PrimitiveTopology::eTriangleList;
    }
}

vk::BlendFactor Convert::to_vk_blend_factor(BlendMode mode, bool src) {
    switch (mode) {
        case BlendMode::None: return vk::BlendFactor::eOne;
        case BlendMode::Alpha:
            return src ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOneMinusSrcAlpha;
        case BlendMode::Additive:
            return src ? vk::BlendFactor::eSrcAlpha : vk::BlendFactor::eOne;
        case BlendMode::Multiply:
            return src ? vk::BlendFactor::eDstColor : vk::BlendFactor::eZero;
        default: return vk::BlendFactor::eOne;
    }
}

vk::BlendOp Convert::to_vk_blend_op([[maybe_unused]] BlendMode mode) {
    // All our blend modes use standard add operation
    return vk::BlendOp::eAdd;
}

vk::CullModeFlags Convert::to_vk_cull_mode(CullMode mode) {
    switch (mode) {
        case CullMode::None: return vk::CullModeFlagBits::eNone;
        case CullMode::Front: return vk::CullModeFlagBits::eFront;
        case CullMode::Back: return vk::CullModeFlagBits::eBack;
        case CullMode::FrontAndBack: return vk::CullModeFlagBits::eFrontAndBack;
        default: return vk::CullModeFlagBits::eBack;
    }
}

vk::CompareOp Convert::to_vk_compare_op(DepthTest test) {
    switch (test) {
        case DepthTest::Never: return vk::CompareOp::eNever;
        case DepthTest::Less: return vk::CompareOp::eLess;
        case DepthTest::Equal: return vk::CompareOp::eEqual;
        case DepthTest::LessEqual: return vk::CompareOp::eLessOrEqual;
        case DepthTest::Greater: return vk::CompareOp::eGreater;
        case DepthTest::NotEqual: return vk::CompareOp::eNotEqual;
        case DepthTest::GreaterEqual: return vk::CompareOp::eGreaterOrEqual;
        case DepthTest::Always: return vk::CompareOp::eAlways;
        default: return vk::CompareOp::eLess;
    }
}

vk::Format Convert::to_vk_vertex_format(AttributeFormat format) {
    switch (format) {
        case AttributeFormat::Float: return vk::Format::eR32Sfloat;
        case AttributeFormat::Float2: return vk::Format::eR32G32Sfloat;
        case AttributeFormat::Float3: return vk::Format::eR32G32B32Sfloat;
        case AttributeFormat::Float4: return vk::Format::eR32G32B32A32Sfloat;
        case AttributeFormat::Int: return vk::Format::eR32Sint;
        case AttributeFormat::Int2: return vk::Format::eR32G32Sint;
        case AttributeFormat::Int3: return vk::Format::eR32G32B32Sint;
        case AttributeFormat::Int4: return vk::Format::eR32G32B32A32Sint;
        case AttributeFormat::UInt: return vk::Format::eR32Uint;
        case AttributeFormat::UInt2: return vk::Format::eR32G32Uint;
        case AttributeFormat::UInt3: return vk::Format::eR32G32B32Uint;
        case AttributeFormat::UInt4: return vk::Format::eR32G32B32A32Uint;
        default: return vk::Format::eR32G32B32Sfloat;
    }
}

vk::ImageLayout Convert::to_optimal_layout(vk::ImageUsageFlags usage, [[maybe_unused]] bool depth_format) {
    if (usage & vk::ImageUsageFlagBits::eColorAttachment) {
        return vk::ImageLayout::eColorAttachmentOptimal;
    }
    if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        return vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    if (usage & vk::ImageUsageFlagBits::eSampled) {
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    }
    if (usage & vk::ImageUsageFlagBits::eStorage) {
        return vk::ImageLayout::eGeneral;
    }
    if (usage & vk::ImageUsageFlagBits::eTransferSrc) {
        return vk::ImageLayout::eTransferSrcOptimal;
    }
    if (usage & vk::ImageUsageFlagBits::eTransferDst) {
        return vk::ImageLayout::eTransferDstOptimal;
    }
    return vk::ImageLayout::eGeneral;
}

vk::AccessFlags Convert::access_mask_for_layout(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eUndefined:
            return {};
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::AccessFlagBits::eTransferWrite;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::AccessFlagBits::eShaderRead;
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::ePresentSrcKHR:
            return {};
        default:
            return {};
    }
}

vk::PipelineStageFlags Convert::pipeline_stage_for_layout(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eUndefined:
            return vk::PipelineStageFlagBits::eTopOfPipe;
        case vk::ImageLayout::eTransferSrcOptimal:
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::PipelineStageFlagBits::eTransfer;
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::PipelineStageFlagBits::eFragmentShader;
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
        case vk::ImageLayout::ePresentSrcKHR:
            return vk::PipelineStageFlagBits::eBottomOfPipe;
        default:
            return vk::PipelineStageFlagBits::eAllCommands;
    }
}

ImageTransition ImageTransition::create(vk::Image image, vk::ImageLayout old_layout, 
                                        vk::ImageLayout new_layout, vk::ImageAspectFlags aspect_mask,
                                        uint32_t mip_levels, uint32_t array_layers) {
    ImageTransition transition{};
    transition.image = image;
    transition.old_layout = old_layout;
    transition.new_layout = new_layout;
    
    transition.subresource_range.aspectMask = aspect_mask;
    transition.subresource_range.baseMipLevel = 0;
    transition.subresource_range.levelCount = mip_levels;
    transition.subresource_range.baseArrayLayer = 0;
    transition.subresource_range.layerCount = array_layers;
    
    transition.src_access_mask = Convert::access_mask_for_layout(old_layout);
    transition.dst_access_mask = Convert::access_mask_for_layout(new_layout);
    transition.src_stage_mask = Convert::pipeline_stage_for_layout(old_layout);
    transition.dst_stage_mask = Convert::pipeline_stage_for_layout(new_layout);
    
    return transition;
}

vk::MemoryBarrier Barriers::memory(vk::AccessFlags src_access, vk::AccessFlags dst_access) {
    vk::MemoryBarrier barrier{};
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    return barrier;
}

vk::BufferMemoryBarrier Barriers::buffer(vk::Buffer buffer, vk::AccessFlags src_access,
                                         vk::AccessFlags dst_access, vk::DeviceSize offset,
                                         vk::DeviceSize size) {
    vk::BufferMemoryBarrier barrier{};
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer;
    barrier.offset = offset;
    barrier.size = size;
    return barrier;
}

vk::ImageMemoryBarrier Barriers::image(const ImageTransition& transition) {
    vk::ImageMemoryBarrier barrier{};
    barrier.srcAccessMask = transition.src_access_mask;
    barrier.dstAccessMask = transition.dst_access_mask;
    barrier.oldLayout = transition.old_layout;
    barrier.newLayout = transition.new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = transition.image;
    barrier.subresourceRange = transition.subresource_range;
    return barrier;
}

vk::Viewport ViewportHelper::create(const Viewport& viewport) {
    vk::Viewport vp{};
    vp.x = viewport.position.x();
    vp.y = viewport.position.y();
    vp.width = viewport.size.x();
    vp.height = viewport.size.y();
    vp.minDepth = viewport.depth_range.x();
    vp.maxDepth = viewport.depth_range.y();
    return vp;
}

vk::Rect2D ViewportHelper::create(const Scissor& scissor) {
    vk::Rect2D rect{};
    rect.offset.x = scissor.offset.x();
    rect.offset.y = scissor.offset.y();
    rect.extent.width = static_cast<uint32_t>(scissor.extent.x());
    rect.extent.height = static_cast<uint32_t>(scissor.extent.y());
    return rect;
}

void Commands::transition_image_layout(vk::CommandBuffer cmd, const ImageTransition& transition) {
    auto barrier = Barriers::image(transition);
    cmd.pipelineBarrier(transition.src_stage_mask, transition.dst_stage_mask, {}, {}, {}, barrier);
}

void Commands::copy_buffer(vk::CommandBuffer cmd, vk::Buffer src, vk::Buffer dst,
                          vk::DeviceSize size, vk::DeviceSize src_offset, vk::DeviceSize dst_offset) {
    vk::BufferCopy copy_region{};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    cmd.copyBuffer(src, dst, copy_region);
}

void Commands::copy_buffer_to_image(vk::CommandBuffer cmd, vk::Buffer buffer, vk::Image image,
                                   uint32_t width, uint32_t height, uint32_t layer) {
    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};
    
    cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void Commands::pipeline_barrier(vk::CommandBuffer cmd, vk::PipelineStageFlags src_stage_mask,
                               vk::PipelineStageFlags dst_stage_mask,
                               std::span<const vk::MemoryBarrier> memory_barriers,
                               std::span<const vk::BufferMemoryBarrier> buffer_barriers,
                               std::span<const vk::ImageMemoryBarrier> image_barriers) {
    cmd.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, memory_barriers, buffer_barriers, image_barriers);
}

bool FormatUtils::is_depth_format(vk::Format format) {
    return format == vk::Format::eD16Unorm || format == vk::Format::eD32Sfloat || 
           format == vk::Format::eD24UnormS8Uint || format == vk::Format::eD32SfloatS8Uint;
}

bool FormatUtils::is_stencil_format(vk::Format format) {
    return format == vk::Format::eS8Uint || format == vk::Format::eD24UnormS8Uint || 
           format == vk::Format::eD32SfloatS8Uint;
}

bool FormatUtils::is_depth_stencil_format(vk::Format format) {
    return is_depth_format(format) || is_stencil_format(format);
}

vk::ImageAspectFlags FormatUtils::get_aspect_mask(vk::Format format) {
    if (is_depth_stencil_format(format)) {
        vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
        if (is_stencil_format(format)) {
            aspect |= vk::ImageAspectFlagBits::eStencil;
        }
        return aspect;
    }
    return vk::ImageAspectFlagBits::eColor;
}

bool Validation::check_result(vk::Result result, std::string_view operation) {
    if (result != vk::Result::eSuccess) {
        std::cerr << "Vulkan operation '" << operation << "' failed with result: " 
                  << vk::to_string(result) << std::endl;
        return false;
    }
    return true;
}

void Validation::check_result_throw(vk::Result result, std::string_view operation) {
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error(std::string("Vulkan operation '") + 
                               std::string(operation) + "' failed with result: " + 
                               vk::to_string(result));
    }
}

} // namespace Manifest::Render::Vulkan

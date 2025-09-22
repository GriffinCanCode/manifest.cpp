#include "VulkanRenderer.hpp"
#include "../../core/log/Log.hpp"
#include <iostream>
#include <cstdlib>

// Include proper Vulkan headers
#include <vulkan/vulkan.hpp>

// Include VMA for memory allocation  
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// Include GLFW for window/surface management
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Manifest {
namespace Render {
namespace Vulkan {

// Module-specific logger for Vulkan renderer
MODULE_LOGGER("VulkanRenderer");

// Import specific modern C++ compatibility types (avoiding Result conflict) 
using Core::Modern::span;
using Core::Modern::byte;

Result<void> VulkanRenderer::initialize() {
    try {
        logger_->info("Initializing Vulkan renderer...");
        
        // Create Vulkan instance
        logger_->debug("Creating Vulkan instance");
        if (!create_instance()) {
            logger_->error("Failed to create Vulkan instance");
            return RendererError::InitializationFailed;
        }
        
        // Select physical device
        logger_->debug("Selecting physical device");
        if (!select_physical_device()) {
            logger_->error("Failed to select suitable physical device");
            return RendererError::InitializationFailed;
        }
        
        // Create logical device
        logger_->debug("Creating logical device");
        if (!create_logical_device()) {
            logger_->error("Failed to create logical device");
            return RendererError::InitializationFailed;
        }
        
        // Create memory allocators
        logger_->debug("Creating memory allocators");
        if (!create_allocators()) {
            logger_->error("Failed to create memory allocators");
            return RendererError::InitializationFailed;
        }
        
        // Create resource managers
        logger_->debug("Creating resource managers");
        if (!create_resource_managers()) {
            logger_->error("Failed to create resource managers");
            return RendererError::InitializationFailed;
        }
        
        // TODO: Create swapchain (requires window surface)
        // Window integration available via UI::Window::Integration namespace
        
        initialized_ = true;
        logger_->info("Vulkan renderer initialized successfully");
        return {};
    } catch (const std::exception& e) {
        logger_->fatal("Failed to initialize Vulkan renderer: {}", e.what());
        return RendererError::InitializationFailed;
    }
}

void VulkanRenderer::shutdown() {
    if (!initialized_) return;
    
    logger_->info("Shutting down Vulkan renderer");
    
    std::cout << "Shutting down Vulkan renderer..." << std::endl;
    
    // Wait for device to be idle before cleanup
    if (device_) {
        device_->wait_idle();
    }
    
    // Cleanup in reverse order
    cleanup_swapchain();
    
    // Resource managers will cleanup automatically via RAII
    descriptor_manager_.reset();
    render_target_manager_.reset();
    buffer_manager_.reset();
    texture_manager_.reset();
    pipeline_manager_.reset();
    shader_manager_.reset();
    
    command_manager_.reset();
    memory_allocator_.reset();
    
    // Cleanup VMA
    if (vma_allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(vma_allocator_);
        vma_allocator_ = VK_NULL_HANDLE;
    }
    
    device_.reset();
    physical_device_.reset();
    surface_.reset();
    instance_.reset();
    
    initialized_ = false;
    std::cout << "Vulkan renderer shutdown complete" << std::endl;
}

Result<BufferHandle> VulkanRenderer::create_buffer(const BufferDesc& desc,
                                                  Core::Modern::span<const Core::Modern::byte> initial_data) {
    if (!initialized_ || !buffer_manager_) return RendererError::InvalidState;
    
    try {
        auto handle = buffer_manager_->create(desc, initial_data);
        if (!handle.is_valid()) return RendererError::OutOfMemory;
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create buffer: " << e.what() << std::endl;
        return RendererError::OutOfMemory;
    }
}

Result<TextureHandle> VulkanRenderer::create_texture(const TextureDesc& desc,
                                                     Core::Modern::span<const Core::Modern::byte> initial_data) {
    if (!initialized_ || !texture_manager_) return RendererError::InvalidState;
    
    try {
        auto handle = texture_manager_->create(desc, initial_data);
        if (!handle.is_valid()) return RendererError::UnsupportedFormat;
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create texture: " << e.what() << std::endl;
        return RendererError::UnsupportedFormat;
    }
}

Result<ShaderHandle> VulkanRenderer::create_shader(const ShaderDesc& desc) {
    if (!initialized_ || !shader_manager_) return RendererError::InvalidState;
    
    try {
        auto handle = shader_manager_->create(desc);
        if (!handle.is_valid()) return RendererError::CompilationFailed;
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create shader: " << e.what() << std::endl;
        return RendererError::CompilationFailed;
    }
}

Result<PipelineHandle> VulkanRenderer::create_pipeline(const PipelineDesc& desc) {
    if (!initialized_ || !pipeline_manager_) return RendererError::InvalidState;
    
    try {
        auto handle = pipeline_manager_->create(desc);
        if (!handle.is_valid()) return RendererError::CompilationFailed;
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create pipeline: " << e.what() << std::endl;
        return RendererError::CompilationFailed;
    }
}

Result<RenderTargetHandle> VulkanRenderer::create_render_target(
    Core::Modern::span<const TextureHandle> color_attachments,
    TextureHandle depth_attachment) {
    if (!initialized_ || !render_target_manager_) return RendererError::InvalidState;
    
    try {
        auto handle = render_target_manager_->create(color_attachments, depth_attachment);
        if (!handle.is_valid()) return RendererError::InvalidState;
        return handle;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create render target: " << e.what() << std::endl;
        return RendererError::InvalidState;
    }
}

void VulkanRenderer::destroy_buffer(BufferHandle handle) {
    if (buffer_manager_) buffer_manager_->destroy(handle);
}

void VulkanRenderer::destroy_texture(TextureHandle handle) {
    if (texture_manager_) texture_manager_->destroy(handle);
}

void VulkanRenderer::destroy_shader(ShaderHandle handle) {
    if (shader_manager_) shader_manager_->destroy(handle);
}

void VulkanRenderer::destroy_pipeline(PipelineHandle handle) {
    if (pipeline_manager_) pipeline_manager_->destroy(handle);
}

void VulkanRenderer::destroy_render_target(RenderTargetHandle handle) {
    if (render_target_manager_) render_target_manager_->destroy(handle);
}

Result<void> VulkanRenderer::update_buffer(BufferHandle handle, std::size_t offset,
                                           Core::Modern::span<const Core::Modern::byte> data) {
    if (!initialized_ || !buffer_manager_) return RendererError::InvalidState;
    
    try {
        buffer_manager_->update(handle, offset, data);
        return {};
    } catch (const std::exception& e) {
        std::cerr << "Failed to update buffer: " << e.what() << std::endl;
        return RendererError::InvalidState;
    }
}

Result<void> VulkanRenderer::update_texture(TextureHandle handle, std::uint32_t mip_level,
                                            Core::Modern::span<const Core::Modern::byte> data) {
    if (!initialized_ || !texture_manager_) return RendererError::InvalidState;
    
    try {
        texture_manager_->update(handle, mip_level, data);
        return {};
    } catch (const std::exception& e) {
        std::cerr << "Failed to update texture: " << e.what() << std::endl;
        return RendererError::InvalidState;
    }
}

void VulkanRenderer::begin_frame() {
    if (!initialized_) return;
    stats_.reset();
}

void VulkanRenderer::end_frame() {
    if (!initialized_) return;
}

void VulkanRenderer::begin_render_pass(RenderTargetHandle target, const Vec4f& clear_color) {
    if (!initialized_) return;
    current_render_target_ = target;
}

void VulkanRenderer::end_render_pass() {
    if (!initialized_) return;
    current_render_target_ = {};
}

void VulkanRenderer::set_viewport(const Viewport& viewport) {
    if (!initialized_) return;
    current_viewport_ = viewport;
}

void VulkanRenderer::set_scissor(const Scissor& scissor) {
    if (!initialized_) return;
    current_scissor_ = scissor;
}

void VulkanRenderer::bind_pipeline(PipelineHandle pipeline) {
    if (!initialized_) return;
    current_pipeline_ = pipeline;
}

void VulkanRenderer::bind_vertex_buffer(BufferHandle buffer, std::uint32_t binding, std::size_t offset) {
    if (!initialized_) return;
    // Stub implementation
}

void VulkanRenderer::bind_index_buffer(BufferHandle buffer, std::size_t offset) {
    if (!initialized_) return;
    // Stub implementation
}

void VulkanRenderer::bind_texture(TextureHandle texture, std::uint32_t binding) {
    if (!initialized_) return;
    // Stub implementation
}

void VulkanRenderer::bind_uniform_buffer(BufferHandle buffer, std::uint32_t binding,
                                        std::size_t offset, std::size_t size) {
    if (!initialized_) return;
    // Stub implementation
}

void VulkanRenderer::draw(const DrawCommand& command) {
    if (!initialized_) return;
    
    stats_.draw_calls++;
    stats_.vertices_rendered += command.vertex_count * command.instance_count;
}

void VulkanRenderer::draw_indexed(const DrawIndexedCommand& command) {
    if (!initialized_) return;
    
    stats_.draw_calls++;
    stats_.vertices_rendered += command.index_count * command.instance_count;
    stats_.triangles_rendered += (command.index_count / 3) * command.instance_count;
}

void VulkanRenderer::dispatch(const ComputeDispatch& dispatch) {
    if (!initialized_) return;
    // Stub implementation
}

std::uint64_t VulkanRenderer::get_used_memory() const noexcept {
    return stats_.gpu_memory_used;
}

std::uint64_t VulkanRenderer::get_available_memory() const noexcept {
    // Stub implementation - return 0 for now
    return 0;
}

void VulkanRenderer::set_debug_name(BufferHandle handle, std::string_view name) {
    // Stub implementation
}

void VulkanRenderer::set_debug_name(TextureHandle handle, std::string_view name) {
    // Stub implementation
}

void VulkanRenderer::set_debug_name(ShaderHandle handle, std::string_view name) {
    // Stub implementation
}

void VulkanRenderer::set_debug_name(PipelineHandle handle, std::string_view name) {
    // Stub implementation
}

void VulkanRenderer::push_debug_group(std::string_view name) {
    // Stub implementation
}

void VulkanRenderer::pop_debug_group() {
    // Stub implementation
}

void VulkanRenderer::insert_debug_marker(std::string_view name) {
    // Stub implementation
}

bool VulkanRenderer::create_instance() {
    // Setup MoltenVK environment with proper implementation
    #ifdef __APPLE__
        setenv("MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", "1", 1);
        setenv("MVK_CONFIG_SHADER_CONVERSION_FLIP_VERTEX_Y", "1", 1);
        setenv("MVK_CONFIG_TRACE_VULKAN_CALLS", "0", 1);
        setenv("MVK_CONFIG_FORCE_LOW_POWER_GPU", "0", 1);
    #endif
    
    instance_ = std::make_unique<Instance>();
    
    std::vector<const char*> extensions{REQUIRED_INSTANCE_EXTENSIONS.begin(), 
                                       REQUIRED_INSTANCE_EXTENSIONS.end()};
    
    std::vector<const char*> layers;
    #ifdef VK_ENABLE_VALIDATION
    layers.push_back(VALIDATION_LAYER_NAME);
    validation_enabled_ = true;
    #endif
    
    return instance_->create(extensions, layers);
}

bool VulkanRenderer::select_physical_device() {
    physical_device_ = std::make_unique<PhysicalDevice>();
    
    DeviceRequirements requirements{};
    requirements.discrete_gpu_preferred = true;
    requirements.graphics_queue_required = true;
    requirements.present_queue_required = true;
    requirements.required_extensions = {REQUIRED_DEVICE_EXTENSIONS.begin(), 
                                       REQUIRED_DEVICE_EXTENSIONS.end()};
    
    return physical_device_->select(instance_->get(), {}, requirements);
}

bool VulkanRenderer::create_logical_device() {
    device_ = std::make_unique<Device>();
    
    // Enable useful features
    vk::PhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;
    features.sampleRateShading = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;
    
    std::vector<const char*> extensions{REQUIRED_DEVICE_EXTENSIONS.begin(), 
                                       REQUIRED_DEVICE_EXTENSIONS.end()};
    
    if (!device_->create(*physical_device_, extensions, features)) {
        return false;
    }
    
    // Create command manager
    command_manager_ = std::make_unique<CommandManager>(device_.get(), 2);
    return command_manager_->initialize();
}

bool VulkanRenderer::create_allocators() {
    // Create VMA allocator
    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.physicalDevice = physical_device_->get();
    allocator_info.device = device_->get();
    allocator_info.instance = instance_->get();
    
    if (vmaCreateAllocator(&allocator_info, &vma_allocator_) != VK_SUCCESS) {
        logger_->error("Failed to create VMA allocator");
        return false;
    }
    
    // Create our custom allocator wrapper
    memory_allocator_ = std::make_unique<MemoryAllocator>(*device_);
    return true;
}

bool VulkanRenderer::create_resource_managers() {
    shader_manager_ = std::make_unique<ShaderManager>(device_->get());
    pipeline_manager_ = std::make_unique<PipelineManager>(device_->get(), shader_manager_.get());
    texture_manager_ = std::make_unique<TextureManager>(device_->get(), memory_allocator_.get());
    buffer_manager_ = std::make_unique<BufferManager>(device_->get(), memory_allocator_.get());
    render_target_manager_ = std::make_unique<RenderTargetManager>(device_->get(), texture_manager_.get());
    descriptor_manager_ = std::make_unique<DescriptorManager>(device_->get());
    
    return true;
}

bool VulkanRenderer::create_swapchain() {
    // TODO: Implement swapchain creation when surface is available
    return true;
}

void VulkanRenderer::cleanup_swapchain() {
    swapchain_image_views_.clear();
    swapchain_images_.clear();
    swapchain_.reset();
}

}  // namespace Vulkan
}  // namespace Render
}  // namespace Manifest
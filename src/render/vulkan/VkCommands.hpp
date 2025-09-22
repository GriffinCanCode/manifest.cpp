#pragma once

#include "VkDevice.hpp"
#include <vector>
#include <functional>

namespace Manifest::Render::Vulkan {

// Frame synchronization objects
struct FrameSync {
    vk::UniqueSemaphore image_available;
    vk::UniqueSemaphore render_finished;
    vk::UniqueFence in_flight;
    
    // Make FrameSync movable but not copyable
    FrameSync() = default;
    FrameSync(const FrameSync&) = delete;
    FrameSync& operator=(const FrameSync&) = delete;
    FrameSync(FrameSync&&) = default;
    FrameSync& operator=(FrameSync&&) = default;
    
    bool create(vk::Device device) {
        try {
            vk::SemaphoreCreateInfo semaphore_info{};
            image_available = device.createSemaphoreUnique(semaphore_info).value;
            render_finished = device.createSemaphoreUnique(semaphore_info).value;
            
            vk::FenceCreateInfo fence_info{};
            fence_info.flags = vk::FenceCreateFlagBits::eSignaled;
            in_flight = device.createFenceUnique(fence_info).value;
            
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
};

// Command buffer recording helper
class CommandRecorder {
    vk::CommandBuffer cmd_;
    bool recording_{false};

public:
    explicit CommandRecorder(vk::CommandBuffer cmd) : cmd_{cmd} {}
    
    void begin(vk::CommandBufferUsageFlags usage = {}) {
        if (recording_) return;
        
        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = usage;
        cmd_.begin(begin_info);
        recording_ = true;
    }
    
    void end() {
        if (!recording_) return;
        cmd_.end();
        recording_ = false;
    }
    
    // RAII wrapper for automatic begin/end
    class ScopedRecord {
        CommandRecorder& recorder_;
    public:
        explicit ScopedRecord(CommandRecorder& recorder) : recorder_{recorder} {
            recorder_.begin();
        }
        ~ScopedRecord() { recorder_.end(); }
    };
    
    ScopedRecord scoped_record() { return ScopedRecord{*this}; }
    
    vk::CommandBuffer get() const { return cmd_; }
    bool is_recording() const { return recording_; }
};

// Command buffer manager for frame-based rendering
class CommandManager {
    struct FrameData {
        CommandPool graphics_pool;
        CommandPool transfer_pool;
        vk::UniqueCommandBuffer primary_cmd;
        std::vector<vk::UniqueCommandBuffer> secondary_cmds;
        FrameSync sync;
        
        // Make FrameData movable but not copyable
        FrameData() = default;
        FrameData(const FrameData&) = delete;
        FrameData& operator=(const FrameData&) = delete;
        FrameData(FrameData&&) = default;
        FrameData& operator=(FrameData&&) = default;
    };
    
    Device* device_;
    std::vector<FrameData> frames_;
    uint32_t current_frame_{0};
    uint32_t max_frames_in_flight_;

public:
    explicit CommandManager(Device* device, uint32_t max_frames = 2)
        : device_{device}, max_frames_in_flight_{max_frames} {
        frames_.reserve(max_frames);
        for (uint32_t i = 0; i < max_frames; ++i) {
            frames_.emplace_back();
        }
    }
    
    bool initialize() {
        for (auto& frame : frames_) {
            // Create command pools
            if (!frame.graphics_pool.create(device_->get(), device_->graphics_queue().family_index())) {
                return false;
            }
            if (!frame.transfer_pool.create(device_->get(), device_->graphics_queue().family_index())) {
                return false;
            }
            
            // Allocate primary command buffer
            frame.primary_cmd = frame.graphics_pool.allocate_buffer(device_->get());
            
            // Create synchronization objects
            if (!frame.sync.create(device_->get())) {
                return false;
            }
        }
        return true;
    }
    
    void wait_for_frame() {
        auto& frame = frames_[current_frame_];
        auto result = device_->get().waitForFences(frame.sync.in_flight.get(), VK_TRUE, UINT64_MAX);
        if (result == vk::Result::eSuccess) {
            device_->get().resetFences(frame.sync.in_flight.get());
        }
    }
    
    void begin_frame() {
        auto& frame = frames_[current_frame_];
        frame.graphics_pool.reset();
        
        CommandRecorder recorder{frame.primary_cmd.get()};
        recorder.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    }
    
    void end_frame() {
        auto& frame = frames_[current_frame_];
        CommandRecorder recorder{frame.primary_cmd.get()};
        recorder.end();
    }
    
    void submit_frame() {
        auto& frame = frames_[current_frame_];
        
        vk::SubmitInfo submit_info{};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &frame.primary_cmd.get();
        
        device_->graphics_queue().submit(submit_info, frame.sync.in_flight.get());
        
        current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
    }
    
    vk::CommandBuffer current_command_buffer() {
        return frames_[current_frame_].primary_cmd.get();
    }
    
    CommandRecorder get_recorder() {
        return CommandRecorder{current_command_buffer()};
    }
    
    // One-time command execution
    template<typename Func>
    void execute_immediate(Func&& func) {
        OneTimeCommands one_time{device_->get(), device_->graphics_queue(), 
                                frames_[current_frame_].graphics_pool};
        one_time.execute(std::forward<Func>(func));
    }
    
    uint32_t current_frame_index() const { return current_frame_; }
    uint32_t max_frames() const { return max_frames_in_flight_; }
};

// Render pass management
class RenderPassRecorder {
    vk::CommandBuffer cmd_;
    vk::RenderPass render_pass_;
    bool active_{false};

public:
    RenderPassRecorder(vk::CommandBuffer cmd, vk::RenderPass render_pass)
        : cmd_{cmd}, render_pass_{render_pass} {}
    
    void begin(vk::Framebuffer framebuffer, vk::Extent2D extent,
              const std::vector<vk::ClearValue>& clear_values = {}) {
        if (active_) return;
        
        vk::RenderPassBeginInfo begin_info{};
        begin_info.renderPass = render_pass_;
        begin_info.framebuffer = framebuffer;
        begin_info.renderArea.offset = vk::Offset2D{0, 0};
        begin_info.renderArea.extent = extent;
        begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        begin_info.pClearValues = clear_values.data();
        
        cmd_.beginRenderPass(begin_info, vk::SubpassContents::eInline);
        active_ = true;
    }
    
    void end() {
        if (!active_) return;
        cmd_.endRenderPass();
        active_ = false;
    }
    
    void next_subpass() {
        if (active_) {
            cmd_.nextSubpass(vk::SubpassContents::eInline);
        }
    }
    
    // RAII wrapper
    class ScopedRenderPass {
        RenderPassRecorder& recorder_;
    public:
        ScopedRenderPass(RenderPassRecorder& recorder, vk::Framebuffer framebuffer, 
                        vk::Extent2D extent, const std::vector<vk::ClearValue>& clear_values = {})
            : recorder_{recorder} {
            recorder_.begin(framebuffer, extent, clear_values);
        }
        ~ScopedRenderPass() { recorder_.end(); }
    };
    
    ScopedRenderPass scoped(vk::Framebuffer framebuffer, vk::Extent2D extent,
                           const std::vector<vk::ClearValue>& clear_values = {}) {
        return ScopedRenderPass{*this, framebuffer, extent, clear_values};
    }
    
    bool is_active() const { return active_; }
    vk::CommandBuffer command_buffer() const { return cmd_; }
};

// Drawing state management
struct DrawState {
    vk::Pipeline current_pipeline{};
    vk::PipelineLayout current_layout{};
    std::vector<vk::Buffer> vertex_buffers;
    vk::Buffer index_buffer{};
    vk::IndexType index_type{vk::IndexType::eUint32};
    
    void reset() {
        current_pipeline = nullptr;
        current_layout = nullptr;
        vertex_buffers.clear();
        index_buffer = nullptr;
    }
};

// High-level drawing interface
class DrawingContext {
    vk::CommandBuffer cmd_;
    DrawState state_;

public:
    explicit DrawingContext(vk::CommandBuffer cmd) : cmd_{cmd} {}
    
    void bind_pipeline(vk::Pipeline pipeline, vk::PipelineLayout layout) {
        if (state_.current_pipeline != pipeline) {
            cmd_.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            state_.current_pipeline = pipeline;
            state_.current_layout = layout;
        }
    }
    
    void bind_vertex_buffer(uint32_t binding, vk::Buffer buffer, vk::DeviceSize offset = 0) {
        if (state_.vertex_buffers.size() <= binding) {
            state_.vertex_buffers.resize(binding + 1);
        }
        
        if (state_.vertex_buffers[binding] != buffer) {
            cmd_.bindVertexBuffers(binding, buffer, offset);
            state_.vertex_buffers[binding] = buffer;
        }
    }
    
    void bind_index_buffer(vk::Buffer buffer, vk::DeviceSize offset = 0, 
                          vk::IndexType index_type = vk::IndexType::eUint32) {
        if (state_.index_buffer != buffer || state_.index_type != index_type) {
            cmd_.bindIndexBuffer(buffer, offset, index_type);
            state_.index_buffer = buffer;
            state_.index_type = index_type;
        }
    }
    
    void set_viewport(const vk::Viewport& viewport) {
        cmd_.setViewport(0, viewport);
    }
    
    void set_scissor(const vk::Rect2D& scissor) {
        cmd_.setScissor(0, scissor);
    }
    
    void draw(uint32_t vertex_count, uint32_t instance_count = 1, 
              uint32_t first_vertex = 0, uint32_t first_instance = 0) {
        cmd_.draw(vertex_count, instance_count, first_vertex, first_instance);
    }
    
    void draw_indexed(uint32_t index_count, uint32_t instance_count = 1,
                     uint32_t first_index = 0, int32_t vertex_offset = 0, 
                     uint32_t first_instance = 0) {
        cmd_.drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
    }
    
    void reset_state() { state_.reset(); }
    vk::CommandBuffer command_buffer() const { return cmd_; }
};

} // namespace Manifest::Render::Vulkan

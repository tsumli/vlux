#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#include "pch.h"
//
#include "common/buffer.h"

namespace vlux {

template <class UniformBufferObject>
class UniformBuffer {
   public:
    UniformBuffer(const VkDevice device, const VkPhysicalDevice physical_device) : device_(device) {
        const auto buffer_size = static_cast<VkDeviceSize>(sizeof(UniformBufferObject));

        uniform_buffers_.resize(kMaxFramesInFlight);
        uniform_buffers_memory_.resize(kMaxFramesInFlight);
        uniform_buffers_mapped_.resize(kMaxFramesInFlight);

        for (size_t i = 0; i < kMaxFramesInFlight; i++) {
            CreateBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniform_buffers_[i], uniform_buffers_memory_[i], device_, physical_device);

            if (vkMapMemory(device_, uniform_buffers_memory_[i], 0, buffer_size, 0,
                            &uniform_buffers_mapped_[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to map uniform buffer memory!");
            }
        }
    }

    ~UniformBuffer() {
        for (size_t i = 0; i < kMaxFramesInFlight; i++) {
            vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
            vkFreeMemory(device_, uniform_buffers_memory_[i], nullptr);
        }
    }

    const std::vector<VkBuffer>& GetVkUniformBuffers() const { return uniform_buffers_; }
    VkBuffer GetVkUniformBuffer(const size_t idx) const { return uniform_buffers_[idx]; }

    void UpdateUniformBuffer(const UniformBufferObject& params, const uint32_t cur_frame) {
        memcpy(uniform_buffers_mapped_[cur_frame], &params, sizeof(UniformBufferObject));
    }

    size_t GetUniformBufferObjectSize() const { return sizeof(UniformBufferObject); }

   private:
    const VkDevice device_;
    std::vector<VkBuffer> uniform_buffers_;
    std::vector<VkDeviceMemory> uniform_buffers_memory_;
    std::vector<void*> uniform_buffers_mapped_;
};
}  // namespace vlux

#endif
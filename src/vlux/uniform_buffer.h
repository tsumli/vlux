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
        buffers_.reserve(kMaxFramesInFlight);

        for (size_t i = 0; i < kMaxFramesInFlight; i++) {
            auto default_params = UniformBufferObject{};
            buffers_.emplace_back(
                device_, physical_device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                buffer_size, static_cast<void*>(&default_params));
        }
    }

    ~UniformBuffer() = default;

    VkBuffer GetVkBufferUniform(const size_t idx) const { return buffers_.at(idx).GetVkBuffer(); }

    void UpdateUniformBuffer(const UniformBufferObject& params, const uint32_t cur_frame) {
        buffers_.at(cur_frame).UpdateBuffer(&params, sizeof(UniformBufferObject));
    }

    size_t GetUniformBufferObjectSize() const { return sizeof(UniformBufferObject); }

   private:
    const VkDevice device_;
    std::vector<Buffer> buffers_;
};
}  // namespace vlux

#endif
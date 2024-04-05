#ifndef VERTEX_H
#define VERTEX_H
#include "pch.h"

namespace vlux {
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};

constexpr VkVertexInputBindingDescription GetBindingDescription() {
    constexpr auto kBindingDesc = VkVertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    return kBindingDesc;
}

constexpr std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions() {
    constexpr auto kAttrDescs = std::to_array<VkVertexInputAttributeDescription>({
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, pos),
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, normal),
        },
        {
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(Vertex, uv),
        },
        {
            .location = 3,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(Vertex, tangent),
        },
    });
    return kAttrDescs;
}

class VertexBuffer {
   public:
    VertexBuffer(const VkDevice device, const VkPhysicalDevice physical_device,
                 const VkQueue graphics_queue, const VkCommandPool command_pool,
                 std::vector<Vertex>&& vertices);
    ~VertexBuffer();
    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = default;
    VertexBuffer& operator=(VertexBuffer&&) = default;
    VkBuffer GetVertexBuffer() const {
        if (vertex_buffer_ == VK_NULL_HANDLE) {
            throw std::runtime_error("`VertexBuffer::vertex_buffer` is `VK_NULL_HANDLE`");
        }
        return vertex_buffer_;
    }

   private:
    VkDevice device_;
    std::vector<Vertex> vertices_;
    VkBuffer vertex_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertex_buffer_memory_ = VK_NULL_HANDLE;
};

}  // namespace vlux

#endif
#ifndef INDEX_H
#define INDEX_H

#include "pch.h"
//
#include "common/buffer.h"

namespace vlux {
using Index = uint16_t;

class IndexBuffer {
   public:
    IndexBuffer(const VkDevice device, const VkPhysicalDevice physical_device, const VkQueue queue,
                const VkCommandPool command_pool, std::vector<Index>&& indices);
    ~IndexBuffer() = default;

    VkBuffer GetVkBuffer() const { return buffer_->GetVkBuffer(); }
    size_t GetSize() const { return index_count_; }
    const std::vector<Index>& GetIndices() const { return indices_; }

   private:
    const VkDevice device_;
    const size_t index_count_;

    std::optional<Buffer> buffer_;
    std::vector<Index> indices_;
};
}  // namespace vlux

#endif
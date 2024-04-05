#ifndef SHADER_H
#define SHADER_H
#include "pch.h"
//
#include "utils/io.h"

namespace vlux {
VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& code);

class Shader {
   public:
    Shader() = default;
    Shader(const std::filesystem::path& filename, const VkShaderStageFlagBits flagbits,
           const VkDevice device);
    ~Shader() { vkDestroyShaderModule(device_, shader_module_, nullptr); }

    // accessor
    const VkPipelineShaderStageCreateInfo& GetStageInfo() const { return stage_info_; }

   private:
    VkDevice device_;
    VkShaderModule shader_module_;
    VkPipelineShaderStageCreateInfo stage_info_;
    std::vector<char> code_;
};

}  // namespace vlux

#endif
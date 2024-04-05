#include "shader.h"

namespace vlux {

VkShaderModule CreateShaderModule(const VkDevice device, const std::vector<char>& code) {
    const auto create_info = VkShaderModuleCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    assert(shader_module != VK_NULL_HANDLE);
    return shader_module;
}

/**
 * @brief Construct a new Shader:: Shader object
 *
 * @param filename Path to SPIR-V file
 * @param flagbits
 * @param device
 */
Shader::Shader(const std::filesystem::path& filename, const VkShaderStageFlagBits flagbits,
               const VkDevice device)
    : device_(device) {
    constexpr auto kCurLocation = std::source_location::current();
    const auto shader_root = std::filesystem::path(kCurLocation.file_name())
                                 .parent_path()
                                 .parent_path()
                                 .parent_path()
                                 .parent_path() /
                             "processed" / "spv";  // TODO: shader root

    code_ = read_file(shader_root / filename);
    shader_module_ = CreateShaderModule(device, code_);
    stage_info_ = VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = flagbits,
        .module = shader_module_,
        .pName = "main",
    };
}

}  // namespace vlux
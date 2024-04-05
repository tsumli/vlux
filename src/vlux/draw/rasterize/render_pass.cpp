#include "render_pass.h"

namespace vlux::draw::rasterize {

RenderPass::RenderPass(const VkDevice device, const VkFormat format,
                       const VkAttachmentDescription depth_stencil_attachment,
                       const VkAttachmentReference depth_stencil_attachment_ref)
    : device_(device) {
    const auto color_attachment = VkAttachmentDescription{
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    constexpr auto kColorAttachmentRef = std::to_array<VkAttachmentReference>({
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    });

    const auto subpass = VkSubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = kColorAttachmentRef.data(),
        .pDepthStencilAttachment = &depth_stencil_attachment_ref,
    };

    const auto dependencies = std::to_array({
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        },
    });

    const auto attachments = std::to_array({color_attachment, depth_stencil_attachment});
    const auto render_pass_info = VkRenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    render_pass_ = std::make_optional<VkRenderPass>();
    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass_.value()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}
RenderPass::~RenderPass() { vkDestroyRenderPass(device_, render_pass_.value(), nullptr); }
}  // namespace vlux::draw::rasterize
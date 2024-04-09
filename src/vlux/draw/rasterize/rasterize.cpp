#include "rasterize.h"

#include "common/texture_view.h"
#include "device_resource/device_resource.h"
#include "scene/scene.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {
DrawRasterize::DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo, Scene& scene,
                             const DeviceResource& device_resource)
    : scene_(scene),
      transform_ubo_(transform_ubo),
      render_pass_(device_resource.GetDevice().GetVkDevice(),
                   device_resource.GetSwapchain().GetFormat(),
                   device_resource.GetDepthStencil().CreateAttachmentDesc(),
                   device_resource.GetDepthStencil().CreateAttachmentRef()),
      descriptor_set_layout_(device_resource.GetDevice().GetVkDevice()),
      descriptor_pool_(device_resource.GetDevice().GetVkDevice(), scene.GetModels().size()),
      graphics_pipeline_(device_resource.GetDevice().GetVkDevice(), render_pass_.GetVkRenderPass(),
                         descriptor_set_layout_.GetVkDescriptorSetLayout()),
      framebuffer_(render_pass_.GetVkRenderPass(), device_resource.GetDevice().GetVkDevice(),
                   device_resource.GetSwapchain().GetVkExtent().width,
                   device_resource.GetSwapchain().GetVkExtent().height,
                   device_resource.GetSwapchain().GetVkImageViews(),
                   device_resource.GetDepthStencil().GetVkImageView()) {
    for (const auto& model : scene.GetModels()) {
        const auto texture_view = [&]() -> std::optional<TextureView> {
            if (model.GetBaseColorTexture() == nullptr || model.GetNormalTexture() == nullptr) {
                return std::nullopt;
            } else {
                return std::make_optional(TextureView{
                    .color = model.GetBaseColorTexture()->GetImageView(),
                    .normal = model.GetNormalTexture()->GetImageView(),
                });
            }
        }();
        descriptor_sets_.emplace_back(
            device_resource.GetVkPhysicalDevice(), device_resource.GetDevice().GetVkDevice(),
            descriptor_pool_.GetVkDescriptorPool(),
            descriptor_set_layout_.GetVkDescriptorSetLayout(), transform_ubo, texture_view);
    }
}

void DrawRasterize::RecordCommandBuffer(const Scene& scene, const uint32_t image_idx,
                                        const uint32_t cur_frame,
                                        const std::vector<VkFramebuffer>& swapchain_framebuffers,
                                        const VkExtent2D& swapchain_extent,
                                        const VkPipeline graphics_pipeline,
                                        const VkCommandBuffer command_buffer) {
    const auto begin_info = VkCommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    constexpr auto kClearValues = std::to_array<VkClearValue>(
        {{.color = {{0.0f, 0.0f, 0.0f, 1.0f}}}, {.depthStencil = {1.0f, 0}}});

    const auto render_pass_info = VkRenderPassBeginInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass_.GetVkRenderPass(),
        .framebuffer = swapchain_framebuffers[image_idx],
        .renderArea =
            {
                .offset = {0, 0},
                .extent = swapchain_extent,
            },
        .clearValueCount = static_cast<uint32_t>(kClearValues.size()),
        .pClearValues = kClearValues.data(),
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    const auto viewport = VkViewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapchain_extent.width),
        .height = static_cast<float>(swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    const auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = swapchain_extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    for (std::size_t i = 0; const auto& model : scene.GetModels()) {
        const auto vertex_buffers =
            std::vector<VkBuffer>{model.GetVertexBuffers()[0].GetVertexBuffer()};
        const auto offsets = std::vector<VkDeviceSize>{0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers.data(), offsets.data());
        vkCmdBindIndexBuffer(command_buffer, model.GetIndexBuffers()[0].GetIndexBuffer(), 0,
                             VK_INDEX_TYPE_UINT16);
        const auto descriptor_set =
            std::to_array({descriptor_sets_[i].GetVkDescriptorSet(cur_frame)});
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                graphics_pipeline_.GetVkPipelineLayout(), 0, descriptor_set.size(),
                                descriptor_set.data(), 0, nullptr);
        vkCmdDrawIndexed(command_buffer,
                         static_cast<uint32_t>(model.GetIndexBuffers()[0].GetSize()), 1, 0, 0, 0);
        i++;
    }
}

void DrawRasterize::OnRecreateSwapChain(const DeviceResource& device_resource) {
    framebuffer_ =
        FrameBuffer(render_pass_.GetVkRenderPass(), device_resource.GetDevice().GetVkDevice(),
                    device_resource.GetSwapchain().GetVkExtent().width,
                    device_resource.GetSwapchain().GetVkExtent().height,
                    device_resource.GetSwapchain().GetVkImageViews(),
                    device_resource.GetDepthStencil().GetVkImageView());
}

}  // namespace vlux::draw::rasterize
#include "rasterize.h"

#include "common/texture_view.h"
#include "device_resource/device_resource.h"
#include "scene/scene.h"
#include "shader/shader.h"
#include "texture/texture_sampler.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {
DrawRasterize::DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo, Scene& scene,
                             const DeviceResource& device_resource)
    : scene_(scene), transform_ubo_(transform_ubo) {
    const auto device = device_resource.GetDevice().GetVkDevice();
    const auto physical_device = device_resource.GetVkPhysicalDevice();
    // DescriptorPool
    [&]() {
        const auto num_model = static_cast<uint32_t>(scene.GetModels().size());
        const auto pool_sizes = std::to_array({
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model,
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model * 2,
            },
        });
        const auto pool_size_count = static_cast<uint32_t>(pool_sizes.size());
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = num_model * kMaxFramesInFlight,
            .poolSizeCount = pool_size_count,
            .pPoolSizes = pool_sizes.data(),
        };
        descriptor_pool_.emplace(device, pool_info);
    }();

    // RenderPass
    [&]() {
        const auto color_attachment = VkAttachmentDescription{
            .format = device_resource.GetSwapchain().GetFormat(),
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

        const auto depth_stencil_attachment_ref =
            device_resource.GetDepthStencil().CreateAttachmentRef();
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
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            },
        });

        const auto depth_stencil_attachment =
            device_resource.GetDepthStencil().CreateAttachmentDesc();
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
        render_pass_.emplace(device, render_pass_info);
    }();

    // DescriptorSetLayout
    [&]() {
        constexpr auto kLayoutBindings = std::to_array({
            // transform
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = nullptr,
            },
            // color
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
            // normal
            VkDescriptorSetLayoutBinding{
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pImmutableSamplers = nullptr,
            },
        });

        const auto layout_info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
            .pBindings = kLayoutBindings.data(),
        };
        descriptor_set_layout_.emplace(device, layout_info);
    }();

    // PipelineLayout
    [&]() {
        const auto set_layout = descriptor_set_layout_->GetVkDescriptorSetLayout();
        const auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &set_layout,
        };

        pipeline_layout_.emplace(device, pipeline_layout_info);
    }();

    // GraphicsPipeline
    [&]() {
        const auto vert_path = std::filesystem::path("rasterize/shader.vert.spv");
        const auto frag_path = std::filesystem::path("rasterize/shader.frag.spv");
        const auto vert_shader = Shader(vert_path, VK_SHADER_STAGE_VERTEX_BIT, device);
        const auto frag_shader = Shader(frag_path, VK_SHADER_STAGE_FRAGMENT_BIT, device);

        const auto shader_stages = std::vector<VkPipelineShaderStageCreateInfo>{
            vert_shader.GetStageInfo(), frag_shader.GetStageInfo()};

        constexpr auto kBindingDescription = GetBindingDescription();
        constexpr auto kAttributeDescriptions = GetAttributeDescriptions();

        const auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &kBindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(kAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = kAttributeDescriptions.data(),
        };

        constexpr auto kInputAssembly = VkPipelineInputAssemblyStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        constexpr auto kViewportState = VkPipelineViewportStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
        };

        constexpr auto kRasterizer = VkPipelineRasterizationStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };

        constexpr auto kMultisampling = VkPipelineMultisampleStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
        };

        constexpr auto kDepthStencil = VkPipelineDepthStencilStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
        };

        constexpr auto kColorBlendAttachment = VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        const auto color_blending = [&]() {
            auto color_blending = VkPipelineColorBlendStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1,
                .pAttachments = &kColorBlendAttachment,
            };
            color_blending.blendConstants[0] = 0.0f;
            color_blending.blendConstants[1] = 0.0f;
            color_blending.blendConstants[2] = 0.0f;
            color_blending.blendConstants[3] = 0.0f;

            return color_blending;
        }();

        constexpr auto kDynamicStates =
            std::to_array<VkDynamicState>({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

        const auto kDynamicState = VkPipelineDynamicStateCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(kDynamicStates.size()),
            .pDynamicStates = kDynamicStates.data(),
        };

        const auto pipeline_info = VkGraphicsPipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &kInputAssembly,
            .pViewportState = &kViewportState,
            .pRasterizationState = &kRasterizer,
            .pMultisampleState = &kMultisampling,
            .pDepthStencilState = &kDepthStencil,
            .pColorBlendState = &color_blending,
            .pDynamicState = &kDynamicState,
            .layout = pipeline_layout_->GetVkPipelineLayout(),
            .renderPass = render_pass_->GetVkRenderPass(),
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
        };

        graphics_pipeline_.emplace(device, pipeline_info);
    }();

    // FrameBuffer
    [&]() {
        const auto width = device_resource.GetSwapchain().GetVkExtent().width;
        const auto height = device_resource.GetSwapchain().GetVkExtent().height;
        const auto image_views = device_resource.GetSwapchain().GetVkImageViews();
        const auto depth_image_view = device_resource.GetDepthStencil().GetVkImageView();

        framebuffer_.reserve(image_views.size());
        for (size_t i = 0; i < image_views.size(); i++) {
            const auto attachments = std::to_array({image_views[i], depth_image_view});
            const auto framebuffer_info = VkFramebufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass_->GetVkRenderPass(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = width,
                .height = height,
                .layers = 1,
            };
            framebuffer_.emplace_back(device, framebuffer_info);
        }
    }();

    // DescriptorSets
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
        [&]() {
            const auto layouts = std::vector<VkDescriptorSetLayout>(
                kMaxFramesInFlight, descriptor_set_layout_->GetVkDescriptorSetLayout());

            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                .pSetLayouts = layouts.data(),
            };

            for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
                descriptor_sets_.emplace_back(device, alloc_info, 3);
            }

            for (size_t frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
                const auto transform_ubo_buffer_info = VkDescriptorBufferInfo{
                    .buffer = transform_ubo.GetVkUniformBuffer(frame_i),
                    .offset = 0,
                    .range = transform_ubo.GetUniformBufferObjectSize(),
                };

                auto descriptor_write = std::vector<VkWriteDescriptorSet>();
                // transform
                descriptor_write.emplace_back(VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &transform_ubo_buffer_info,
                });
                if (texture_view.has_value()) {
                    texture_sampler_ = std::make_shared<TextureSampler>(physical_device, device);
                    const auto create_image_info = [&](const VkImageView image_view) {
                        return VkDescriptorImageInfo{
                            .sampler = texture_sampler_->GetSampler(),
                            .imageView = image_view,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        };
                    };

                    // color
                    const auto color_image_info = create_image_info(texture_view->color);
                    descriptor_write.emplace_back(VkWriteDescriptorSet{
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptor_sets_[frame_i].GetVkDescriptorSet(1),
                        .dstBinding = 1,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &color_image_info,
                    });

                    // normal
                    const auto normal_image_info = create_image_info(texture_view->normal);
                    descriptor_write.emplace_back(VkWriteDescriptorSet{
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = descriptor_sets_[frame_i].GetVkDescriptorSet(2),
                        .dstBinding = 2,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &normal_image_info,
                    });
                }
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                       descriptor_write.data(), 0, nullptr);
            }
        }();
    }
}

void DrawRasterize::RecordCommandBuffer(const Scene& scene, const uint32_t image_idx,
                                        const uint32_t cur_frame,
                                        const VkExtent2D& swapchain_extent,
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
        .renderPass = render_pass_->GetVkRenderPass(),
        .framebuffer = framebuffer_.at(image_idx).GetVkFrameBuffer(),
        .renderArea =
            {
                .offset = {0, 0},
                .extent = swapchain_extent,
            },
        .clearValueCount = static_cast<uint32_t>(kClearValues.size()),
        .pClearValues = kClearValues.data(),
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline_->GetVkGraphicsPipeline());

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
                                pipeline_layout_->GetVkPipelineLayout(), 0, descriptor_set.size(),
                                descriptor_set.data(), 0, nullptr);
        vkCmdDrawIndexed(command_buffer,
                         static_cast<uint32_t>(model.GetIndexBuffers()[0].GetSize()), 1, 0, 0, 0);
        i++;
    }
}

void DrawRasterize::OnRecreateSwapChain([[maybe_unused]] const DeviceResource& device_resource) {}

}  // namespace vlux::draw::rasterize
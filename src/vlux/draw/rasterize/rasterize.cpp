#include "rasterize.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <exception>
#include <memory>

#include "common/descriptor_set_layout.h"
#include "common/texture_view.h"
#include "device_resource/device_resource.h"
#include "pch.h"
#include "scene/scene.h"
#include "shader/shader.h"
#include "texture/texture_sampler.h"
#include "transform.h"
#include "uniform_buffer.h"

namespace vlux::draw::rasterize {
namespace {
constexpr auto kNumDescriptorSetCompute = 3;
}

DrawRasterize::DrawRasterize(const UniformBuffer<TransformParams>& transform_ubo,
                             const UniformBuffer<CameraParams>& camera_ubo,
                             const UniformBuffer<LightParams>& light_ubo, Scene& scene,
                             const DeviceResource& device_resource)
    : scene_(scene) {
    const auto device = device_resource.GetDevice().GetVkDevice();
    const auto physical_device = device_resource.GetVkPhysicalDevice();
    const auto [width, height] = device_resource.GetWindow().GetWindowSize();

    // Color
    [&]() {
        render_targets_[RenderTargetType::kColor].emplace(device);
        auto& color = render_targets_.at(RenderTargetType::kColor).value();
        color.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
        CreateImage(width, height, color.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color.GetVkImageRef(),
                    color.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view = CreateImageView(color.GetVkImageRef(), color.GetVkFormat(), 0,
                                                VK_IMAGE_ASPECT_COLOR_BIT, device);
        color.SetImageView(image_view);
    }();

    // Normal
    [&]() {
        render_targets_[RenderTargetType::kNormal].emplace(device);
        auto& normal = render_targets_.at(RenderTargetType::kNormal).value();
        normal.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
        CreateImage(width, height, normal.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normal.GetVkImageRef(),
                    normal.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view = CreateImageView(normal.GetVkImageRef(), normal.GetVkFormat(), 0,
                                                VK_IMAGE_ASPECT_COLOR_BIT, device);
        normal.SetImageView(image_view);
    }();

    // Depth Stencil
    [&]() {
        render_targets_[RenderTargetType::kDepthStencil].emplace(device);
        auto& depth_stencil = render_targets_.at(RenderTargetType::kDepthStencil).value();
        depth_stencil.SetFormat(VK_FORMAT_D32_SFLOAT);
        CreateImage(width, height, depth_stencil.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_stencil.GetVkImageRef(),
                    depth_stencil.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view =
            CreateImageView(depth_stencil.GetVkImageRef(), depth_stencil.GetVkFormat(), 0,
                            VK_IMAGE_ASPECT_DEPTH_BIT, device);
        depth_stencil.SetImageView(image_view);
    }();

    // Position
    [&]() {
        render_targets_[RenderTargetType::kPosition].emplace(device);
        auto& position = render_targets_.at(RenderTargetType::kPosition).value();
        position.SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT);
        CreateImage(width, height, position.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, position.GetVkImageRef(),
                    position.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view = CreateImageView(position.GetVkImageRef(), position.GetVkFormat(), 0,
                                                VK_IMAGE_ASPECT_COLOR_BIT, device);
        position.SetImageView(image_view);
    }();

    // Finalized
    [&]() {
        render_targets_[RenderTargetType::kFinalized].emplace(device);
        auto& finalized = render_targets_.at(RenderTargetType::kFinalized).value();
        finalized.SetFormat(VK_FORMAT_B8G8R8A8_UNORM);
        CreateImage(width, height, finalized.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, finalized.GetVkImageRef(),
                    finalized.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view = CreateImageView(finalized.GetVkImageRef(), finalized.GetVkFormat(),
                                                0, VK_IMAGE_ASPECT_COLOR_BIT, device);
        finalized.SetImageView(image_view);
    }();

    // texture sampler
    [&]() {
        texture_samplers_[TextureSamplerType::kColor].emplace(physical_device, device);
        texture_samplers_[TextureSamplerType::kNormal].emplace(physical_device, device);
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

        graphics_descriptor_set_layout_.emplace(device, layout_info);
    }();

    // DescriptorPool
    [&]() {
        const auto num_model = static_cast<uint32_t>(scene.GetModels().size());
        const auto pool_sizes = std::to_array({
            // transform
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model,
            },
            // color + normal
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model * 2,
            },
        });
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = kMaxFramesInFlight * num_model,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        graphics_descriptor_pool_.emplace(device, pool_info);
    }();

    // RenderPass
    [&]() {
        const auto attachment_descs = std::to_array({
            // Color
            VkAttachmentDescription{
                .format = render_targets_.at(RenderTargetType::kColor)->GetVkFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
            // Normal
            VkAttachmentDescription{
                .format = render_targets_.at(RenderTargetType::kNormal)->GetVkFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
            // Position
            VkAttachmentDescription{
                .format = render_targets_.at(RenderTargetType::kPosition)->GetVkFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            },
            // Depth
            VkAttachmentDescription{
                .format = render_targets_.at(RenderTargetType::kDepthStencil)->GetVkFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
        });

        auto subpass = std::vector<VkSubpassDescription>();
        constexpr auto kColorAttachmentRef = std::to_array<VkAttachmentReference>({
            // color
            {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            // normal
            {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            // position
            {
                .attachment = 2,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        });
        constexpr auto kDepthStencilAttachmentRef = VkAttachmentReference{
            .attachment = 3,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        subpass.emplace_back(VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(kColorAttachmentRef.size()),
            .pColorAttachments = kColorAttachmentRef.data(),
            .pDepthStencilAttachment = &kDepthStencilAttachmentRef,
        });

        constexpr auto kDependencies = std::to_array({
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

        const auto render_pass_info = VkRenderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachment_descs.size()),
            .pAttachments = attachment_descs.data(),
            .subpassCount = static_cast<uint32_t>(subpass.size()),
            .pSubpasses = subpass.data(),
            .dependencyCount = static_cast<uint32_t>(kDependencies.size()),
            .pDependencies = kDependencies.data(),
        };
        render_pass_.emplace(device, render_pass_info);
    }();

    // PipelineLayout (Graphics)
    [&]() {
        const auto set_layout = graphics_descriptor_set_layout_->GetVkDescriptorSetLayout();
        const auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &set_layout,
        };
        graphics_pipeline_layout_.emplace(device, pipeline_layout_info);
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

        constexpr auto kColorBlendAttachment = std::to_array({
            // Color
            VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            },
            // Normal
            VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            },
            // Position
            VkPipelineColorBlendAttachmentState{
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            },
        });

        const auto color_blending = [&]() {
            auto color_blending = VkPipelineColorBlendStateCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = static_cast<uint32_t>(kColorBlendAttachment.size()),
                .pAttachments = kColorBlendAttachment.data(),
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
            .layout = graphics_pipeline_layout_->GetVkPipelineLayout(),
            .renderPass = render_pass_->GetVkRenderPass(),
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
        };
        graphics_pipeline_.emplace(device, pipeline_info);
    }();

    // DescriptorSets
    [&]() {
        // allocate descriptor sets
        graphics_descriptor_sets_.reserve(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto descriptor_set_layout = std::vector<VkDescriptorSetLayout>(
                scene.GetModels().size(),
                graphics_descriptor_set_layout_->GetVkDescriptorSetLayout());
            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = graphics_descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = static_cast<uint32_t>(descriptor_set_layout.size()),
                .pSetLayouts = descriptor_set_layout.data(),
            };
            graphics_descriptor_sets_.emplace_back(device, alloc_info);
        }

        // update descriptor sets
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            for (auto model_i = 0; const auto& model : scene_.GetModels()) {
                const auto texture_view = [&]() -> std::optional<TextureView> {
                    if (model.GetBaseColorTexture() == nullptr ||
                        model.GetNormalTexture() == nullptr) {
                        return std::nullopt;
                    } else {
                        return std::make_optional(TextureView{
                            .color = model.GetBaseColorTexture()->GetImageView(),
                            .normal = model.GetNormalTexture()->GetImageView(),
                        });
                    }
                }();

                auto descriptor_write = std::vector<VkWriteDescriptorSet>();
                // transform
                const auto transform_ubo_buffer_info = VkDescriptorBufferInfo{
                    .buffer = transform_ubo.GetVkUniformBuffer(frame_i),
                    .offset = 0,
                    .range = transform_ubo.GetUniformBufferObjectSize(),
                };
                descriptor_write.emplace_back(VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = graphics_descriptor_sets_.at(frame_i).GetVkDescriptorSet(model_i),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &transform_ubo_buffer_info,
                });
                if (texture_view.has_value()) {
                    // color
                    const auto color_image_info = VkDescriptorImageInfo{
                        .sampler = texture_samplers_.at(TextureSamplerType::kColor)->GetSampler(),
                        .imageView = texture_view->color,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                    descriptor_write.emplace_back(VkWriteDescriptorSet{
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = graphics_descriptor_sets_.at(frame_i).GetVkDescriptorSet(model_i),
                        .dstBinding = 1,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &color_image_info,
                    });

                    // normal
                    const auto normal_image_info = VkDescriptorImageInfo{
                        .sampler = texture_samplers_.at(TextureSamplerType::kNormal)->GetSampler(),
                        .imageView = texture_view->normal,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                    descriptor_write.emplace_back(VkWriteDescriptorSet{
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = graphics_descriptor_sets_.at(frame_i).GetVkDescriptorSet(model_i),
                        .dstBinding = 2,
                        .dstArrayElement = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &normal_image_info,
                    });
                }
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                       descriptor_write.data(), 0, nullptr);
                model_i++;
            }
        }
    }();

    // FrameBuffer
    [&]() {
        framebuffer_.reserve(kMaxFramesInFlight);
        for (size_t frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto attachments = std::to_array(
                {render_targets_.at(RenderTargetType::kColor)->GetVkImageView(),
                 render_targets_.at(RenderTargetType::kNormal)->GetVkImageView(),
                 render_targets_.at(RenderTargetType::kPosition)->GetVkImageView(),
                 render_targets_.at(RenderTargetType::kDepthStencil)->GetVkImageView()});
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

    // DescriptorPool (Compute)
    [&]() {
        const auto pool_sizes = std::to_array({
            // transform + camera
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * 2,
            },
            // depth
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
            // color + normal + position + finalized
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * 4,
            },
        });

        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = kMaxFramesInFlight * kNumDescriptorSetCompute,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        compute_descriptor_pool_.emplace(device, pool_info);
    }();

    // DescriptorSetLayout (Compute)
    [&]() {
        compute_descriptor_set_layout_.reserve(kNumDescriptorSetCompute);

        // set = 0
        {
            constexpr auto kLayoutBindings = std::to_array({
                // transform
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
                .pBindings = kLayoutBindings.data(),
            };

            compute_descriptor_set_layout_.emplace_back(device, layout_info);
        }
        // set = 1
        {
            constexpr auto kLayoutBindings = std::to_array({
                // color
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
                // normal
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
                // position
                VkDescriptorSetLayoutBinding{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
                // depth
                VkDescriptorSetLayoutBinding{
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
                // output
                VkDescriptorSetLayoutBinding{
                    .binding = 4,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
                .pBindings = kLayoutBindings.data(),
            };

            compute_descriptor_set_layout_.emplace_back(device, layout_info);
        }

        // set = 2
        {
            constexpr auto kLayoutBindings = std::to_array({
                // light
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .pImmutableSamplers = nullptr,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
                .pBindings = kLayoutBindings.data(),
            };

            compute_descriptor_set_layout_.emplace_back(device, layout_info);
        }
    }();

    // PipelineLayout (Compute)
    [&]() {
        constexpr auto kPushConstantRanges = std::to_array({VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(ModePushConstants),
        }});
        compute_pipeline_layout_.reserve(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            auto set_layout = std::vector<VkDescriptorSetLayout>();
            set_layout.reserve(compute_descriptor_set_layout_.size());
            for (const auto& layout : compute_descriptor_set_layout_) {
                set_layout.emplace_back(layout.GetVkDescriptorSetLayout());
            }
            const auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = static_cast<uint32_t>(set_layout.size()),
                .pSetLayouts = set_layout.data(),
                .pushConstantRangeCount = static_cast<uint32_t>(kPushConstantRanges.size()),
                .pPushConstantRanges = kPushConstantRanges.data(),
            };
            compute_pipeline_layout_.emplace_back(device, pipeline_layout_info);
        }
    }();

    // ComputePipeline
    [&]() {
        compute_pipeline_.reserve(kMaxFramesInFlight);
        const auto comp_path = std::filesystem::path("rasterize/deferred.comp.spv");
        const auto comp_shader = Shader(comp_path, VK_SHADER_STAGE_COMPUTE_BIT, device);

        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto pipeline_info = VkComputePipelineCreateInfo{
                .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                .stage = comp_shader.GetStageInfo(),
                .layout = compute_pipeline_layout_.at(frame_i).GetVkPipelineLayout(),
            };
            compute_pipeline_.emplace_back(device, pipeline_info);
        }
    }();

    // DescriptorSets (Compute)
    [&]() {
        // allocate descriptor sets
        compute_descriptor_sets_.reserve(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            auto set_layout = std::vector<VkDescriptorSetLayout>();
            set_layout.reserve(compute_descriptor_set_layout_.size());
            for (const auto& layout : compute_descriptor_set_layout_) {
                set_layout.emplace_back(layout.GetVkDescriptorSetLayout());
            }
            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = compute_descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = static_cast<uint32_t>(set_layout.size()),
                .pSetLayouts = set_layout.data(),
            };
            compute_descriptor_sets_.emplace_back(device, alloc_info);
        }

        // update descriptor sets
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto transform_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = transform_ubo.GetVkUniformBuffer(frame_i),
                .offset = 0,
                .range = transform_ubo.GetUniformBufferObjectSize(),
            };
            const auto camera_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = camera_ubo.GetVkUniformBuffer(frame_i),
                .offset = 0,
                .range = camera_ubo.GetUniformBufferObjectSize(),
            };
            const auto color_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kColor)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            const auto normal_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kNormal)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            const auto position_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kPosition)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            const auto depth_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kDepthStencil)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            };
            const auto finalized_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kFinalized)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            const auto light_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = light_ubo.GetVkUniformBuffer(frame_i),
                .offset = 0,
                .range = light_ubo.GetUniformBufferObjectSize(),
            };
            const auto descriptor_write = std::to_array({
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &transform_ubo_buffer_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &camera_ubo_buffer_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &color_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &normal_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &position_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                    .dstBinding = 3,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .pImageInfo = &depth_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                    .dstBinding = 4,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &finalized_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(2),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &light_ubo_buffer_info,
                },
            });
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                   descriptor_write.data(), 0, nullptr);
        }
    }();
}

void DrawRasterize::RecordCommandBuffer(const uint32_t image_idx,
                                        const VkExtent2D& swapchain_extent,
                                        const VkCommandBuffer command_buffer) {
    constexpr auto kClearValues = std::to_array<VkClearValue>({
        // Color
        {
            .color =
                {
                    {0.0f, 0.0f, 0.0f, 1.0f},
                },
        },
        // Normal
        {
            .color =
                {
                    {0.0f, 0.0f, 0.0f, 1.0f},
                },
        },
        // Position
        {
            .color =
                {
                    {0.0f, 0.0f, 0.0f, 0.0f},
                },
        },
        // Depth Stencil
        {
            .depthStencil = {1.0f, 0},
        },
    });

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

    for (const auto&& [model_i, model] : std::ranges::views::enumerate(scene_.GetModels())) {
        const auto vertex_buffers =
            std::vector<VkBuffer>{model.GetVertexBuffers()[0].GetVertexBuffer()};
        const auto offsets = std::vector<VkDeviceSize>{0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers.data(), offsets.data());
        vkCmdBindIndexBuffer(command_buffer, model.GetIndexBuffers()[0].GetIndexBuffer(), 0,
                             VK_INDEX_TYPE_UINT16);
        const auto descriptor_set =
            std::to_array({graphics_descriptor_sets_.at(image_idx).GetVkDescriptorSet(model_i)});
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                graphics_pipeline_layout_->GetVkPipelineLayout(), 0,
                                static_cast<uint32_t>(descriptor_set.size()), descriptor_set.data(),
                                0, nullptr);
        vkCmdDrawIndexed(command_buffer,
                         static_cast<uint32_t>(model.GetIndexBuffers()[0].GetSize()), 1, 0, 0, 0);
    }

    spdlog::debug("End render pass");
    vkCmdEndRenderPass(command_buffer);

    // compute
    spdlog::debug("Compute");
    // Transition
    [&]() {
        // depth
        const auto barrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = render_targets_.at(RenderTargetType::kDepthStencil)->GetVkImage(),
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                             VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);
    }();

    [&]() {
        const auto div_up = [](const uint32_t x, const uint32_t y) -> uint32_t {
            return (x + y - 1) / y;
        };
        // thread size is 16x16 in the shader
        const auto group_count_x = div_up(swapchain_extent.width, 16);
        const auto group_count_y = div_up(swapchain_extent.height, 16);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          compute_pipeline_.at(image_idx).GetVkComputePipeline());

        const auto mode = ModePushConstants{.mode = mode_};
        vkCmdPushConstants(command_buffer,
                           compute_pipeline_layout_.at(image_idx).GetVkPipelineLayout(),
                           VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ModePushConstants), &mode);
        vkCmdBindDescriptorSets(
            command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            compute_pipeline_layout_.at(image_idx).GetVkPipelineLayout(), 0,
            static_cast<uint32_t>(compute_descriptor_sets_.at(image_idx).GetSize()),
            compute_descriptor_sets_.at(image_idx).GetVkDescriptorSetPtr(), 0, nullptr);
        vkCmdDispatch(command_buffer, group_count_x, group_count_y, 1);
    }();
}

void DrawRasterize::OnRecreateSwapChain([[maybe_unused]] const DeviceResource& device_resource) {}

}  // namespace vlux::draw::rasterize
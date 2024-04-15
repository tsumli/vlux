#include "rasterize.h"

#include <vulkan/vulkan_core.h>

#include "common/texture_view.h"
#include "common/utils.h"
#include "device_resource/device_resource.h"
#include "pch.h"
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
        const auto image_view = CreateImageView(color.GetVkImageRef(), color.GetVkFormat(),
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
        const auto image_view = CreateImageView(normal.GetVkImageRef(), normal.GetVkFormat(),
                                                VK_IMAGE_ASPECT_COLOR_BIT, device);
        normal.SetImageView(image_view);
    }();

    // Finalized
    [&]() {
        render_targets_[RenderTargetType::kFinalized].emplace(device);
        auto& finalized = render_targets_.at(RenderTargetType::kFinalized).value();
        finalized.SetFormat(VK_FORMAT_R8G8B8A8_UNORM);
        CreateImage(width, height, finalized.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, finalized.GetVkImageRef(),
                    finalized.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view = CreateImageView(finalized.GetVkImageRef(), finalized.GetVkFormat(),
                                                VK_IMAGE_ASPECT_COLOR_BIT, device);
        finalized.SetImageView(image_view);
    }();

    // Depth Stencil
    [&]() {
        render_targets_[RenderTargetType::kDepthStencil].emplace(device);
        auto& depth_stencil = render_targets_.at(RenderTargetType::kDepthStencil).value();
        const auto candidates = std::vector<VkFormat>({
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        });
        const auto format =
            FindSupportedFormat(candidates, VK_IMAGE_TILING_OPTIMAL,
                                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physical_device);
        depth_stencil.SetFormat(format);
        CreateImage(width, height, depth_stencil.GetVkFormat(), VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_stencil.GetVkImageRef(),
                    depth_stencil.GetVkDeviceMemoryRef(), device, physical_device);
        const auto image_view =
            CreateImageView(depth_stencil.GetVkImageRef(), depth_stencil.GetVkFormat(),
                            VK_IMAGE_ASPECT_DEPTH_BIT, device);
        depth_stencil.SetImageView(image_view);
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
            // Depth
            VkAttachmentDescription{
                .format = render_targets_.at(RenderTargetType::kDepthStencil)->GetVkFormat(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            },
        });

        auto subpass = std::vector<VkSubpassDescription>();
        constexpr auto kColorAttachmentRef = std::to_array<VkAttachmentReference>({
            {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
            {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        });
        constexpr auto kDepthStencilAttachmentRef = VkAttachmentReference{
            .attachment = 2,
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
        graphics_descriptor_sets_.resize(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto descriptor_set_layout =
                graphics_descriptor_set_layout_->GetVkDescriptorSetLayout();
            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = graphics_descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = 1,  // only have one descriptor set for each layout
                .pSetLayouts = &descriptor_set_layout,
            };
            graphics_descriptor_sets_.at(frame_i).reserve(scene.GetModels().size());
            for (size_t model_i = 0; model_i < scene.GetModels().size(); model_i++) {
                graphics_descriptor_sets_.at(frame_i).emplace_back(device, alloc_info);
            }
        }

        // texture sampler
        texture_sampler_ = std::make_shared<TextureSampler>(physical_device, device);

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
                    .dstSet =
                        graphics_descriptor_sets_.at(frame_i).at(model_i).GetVkDescriptorSet(),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &transform_ubo_buffer_info,
                });
                if (texture_view.has_value()) {
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
                        .dstSet =
                            graphics_descriptor_sets_.at(frame_i).at(model_i).GetVkDescriptorSet(),
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
                        .dstSet =
                            graphics_descriptor_sets_.at(frame_i).at(model_i).GetVkDescriptorSet(),
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

    // DescriptorPool
    [&]() {
        const auto pool_sizes = std::to_array({
            // color + normal + finalized
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * 3,
            },
        });
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = kMaxFramesInFlight,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        compute_descriptor_pool_.emplace(device, pool_info);
    }();

    // DescriptorSetLayout (Compute)
    [&]() {
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
            // output
            VkDescriptorSetLayoutBinding{
                .binding = 2,
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
        compute_descriptor_set_layout_.emplace(device, layout_info);
    }();

    // PipelineLayout (Compute)
    [&]() {
        const auto set_layout = compute_descriptor_set_layout_->GetVkDescriptorSetLayout();
        const auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &set_layout,
        };
        compute_pipeline_layout_.emplace(device, pipeline_layout_info);
    }();

    // ComputePipeline
    [&]() {
        const auto comp_path = std::filesystem::path("rasterize/deferred.comp.spv");
        const auto comp_shader = Shader(comp_path, VK_SHADER_STAGE_COMPUTE_BIT, device);
        const auto pipeline_info = VkComputePipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = comp_shader.GetStageInfo(),
            .layout = compute_pipeline_layout_->GetVkPipelineLayout(),
        };
        compute_pipeline_.emplace(device, pipeline_info);
    }();

    // DescriptorSets (Compute)
    [&]() {
        // allocate descriptor sets
        compute_descriptor_sets_.reserve(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto descriptor_set_layout =
                compute_descriptor_set_layout_->GetVkDescriptorSetLayout();
            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = compute_descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = 1,
                .pSetLayouts = &descriptor_set_layout,
            };
            compute_descriptor_sets_.emplace_back(device, alloc_info);
        }

        // update descriptor sets
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto color_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kColor)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            const auto normal_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kNormal)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            const auto finalized_image_info = VkDescriptorImageInfo{
                .imageView = render_targets_.at(RenderTargetType::kFinalized)->GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            const auto descriptor_write = std::to_array({
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(),
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &color_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(),
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &normal_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = compute_descriptor_sets_.at(frame_i).GetVkDescriptorSet(),
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &finalized_image_info,
                },
            });
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                   descriptor_write.data(), 0, nullptr);
        }
    }();

    // FrameBuffer
    [&]() {
        framebuffer_.reserve(kMaxFramesInFlight);
        for (size_t frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto attachments = std::to_array(
                {render_targets_.at(RenderTargetType::kColor)->GetVkImageView(),
                 render_targets_.at(RenderTargetType::kNormal)->GetVkImageView(),
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
        const auto descriptor_set = std::to_array(
            {graphics_descriptor_sets_.at(image_idx).at(model_i).GetVkDescriptorSet()});
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
    [&]() {
        const auto div_up = [](const uint32_t x, const uint32_t y) -> uint32_t {
            return (x + y - 1) / y;
        };
        // thread size is 16x16 in the shader
        const auto group_count_x = div_up(swapchain_extent.width, 16);
        const auto group_count_y = div_up(swapchain_extent.height, 16);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          compute_pipeline_->GetVkComputePipeline());
        const auto descriptor_set =
            std::to_array({compute_descriptor_sets_.at(image_idx).GetVkDescriptorSet()});
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                compute_pipeline_layout_->GetVkPipelineLayout(), 0,
                                static_cast<uint32_t>(descriptor_set.size()), descriptor_set.data(),
                                0, nullptr);
        vkCmdDispatch(command_buffer, group_count_x, group_count_y, 1);
    }();
}

void DrawRasterize::OnRecreateSwapChain([[maybe_unused]] const DeviceResource& device_resource) {}

}  // namespace vlux::draw::rasterize
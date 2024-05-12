#include "raytracing.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "common/buffer.h"
#include "common/descriptor_set_layout.h"
#include "model/index.h"
#include "model/vertex.h"
#include "scratch_buffer.h"
#include "shader/shader.h"
#include "utils/math.h"
namespace vlux::draw::raytracing {
namespace {
constexpr auto kNumDescriptorSetRaytracing = 3;
}

DrawRaytracing::DrawRaytracing(const UniformBuffer<TransformParams>& transform_ubo,
                               const UniformBuffer<CameraParams>& camera_ubo,
                               const UniformBuffer<CameraMatrixParams>& camera_matrix_ubo,
                               const UniformBuffer<LightParams>& light_ubo, Scene& scene,
                               const VkQueue queue, const VkCommandPool command_pool,
                               const DeviceResource& device_resource)
    : scene_(scene), device_(device_resource.GetDevice().GetVkDevice()) {
    const auto device = device_resource.GetDevice().GetVkDevice();
    const auto physical_device = device_resource.GetVkPhysicalDevice();
    const auto [width, height] = device_resource.GetWindow().GetWindowSize();

    // Get the ray tracing and accelertion structure related function pointers required by this
    // sample
    spdlog::debug("get ray tracing function pointers");
    vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
        vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR =
        reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR =
        reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR =
        reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR =
        reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
            vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
        vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

    // texture sampler
    spdlog::debug("setup texture samplers");
    [&]() {
        for (auto type_i = 0; type_i < std::to_underlying(TextureSamplerType::kCount); type_i++) {
            texture_samplers_[static_cast<TextureSamplerType>(type_i)].emplace(physical_device,
                                                                               device);
        }
    }();

    spdlog::debug("get ray tracing pipeline properties");
    raytracing_pipeline_properties_ = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };
    auto properties = VkPhysicalDeviceProperties2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &raytracing_pipeline_properties_,
    };
    vkGetPhysicalDeviceProperties2(physical_device, &properties);

    spdlog::debug("get acceleration structure properties");
    acceleration_structure_features_ = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
    };
    auto features = VkPhysicalDeviceFeatures2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &acceleration_structure_features_,
    };
    vkGetPhysicalDeviceFeatures2(physical_device, &features);

    spdlog::debug("setup render targets");
    // Finalized
    render_targets_[RenderTargetType::kFinalized].emplace(
        device, physical_device, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, VK_IMAGE_ASPECT_COLOR_BIT);

    spdlog::debug("create bottom level acceleration structure");
    CreateBottomLevelAS(device, physical_device, queue, command_pool);
    spdlog::debug("create top level acceleration structure");
    CreateTopLevelAS(device, physical_device, queue, command_pool);

    spdlog::debug("Setup rendering objects");

    spdlog::debug("create DescriptorSetLayout");
    [&]() {
        raytracing_descriptor_set_layout_.reserve(kNumDescriptorSetRaytracing);
        {
            // set = 0
            constexpr auto kLayoutBindings = std::to_array({
                // TLAS
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                    .descriptorCount = 1,
                    .stageFlags =
                        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                },
                // Camera
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                  VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                  VK_SHADER_STAGE_MISS_BIT_KHR,
                },
                // Light
                VkDescriptorSetLayoutBinding{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                },
                // Transform
                VkDescriptorSetLayoutBinding{
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                  VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
                .pBindings = kLayoutBindings.data(),
            };
            raytracing_descriptor_set_layout_.emplace_back(device, layout_info);
        }
        {
            // set = 1
            const auto layout_bindings = std::to_array({
                // Finalized
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
                },
                // color
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = static_cast<uint32_t>(scene_.GetModels().size()),
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                    .pImmutableSamplers = nullptr,
                },
                // normal
                VkDescriptorSetLayoutBinding{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = static_cast<uint32_t>(scene_.GetModels().size()),
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                    .pImmutableSamplers = nullptr,
                },
                // emissive
                VkDescriptorSetLayoutBinding{
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = static_cast<uint32_t>(scene_.GetModels().size()),
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                    .pImmutableSamplers = nullptr,
                },
                // occlusion roughness metallic
                VkDescriptorSetLayoutBinding{
                    .binding = 4,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = static_cast<uint32_t>(scene_.GetModels().size()),
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                    .pImmutableSamplers = nullptr,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
                .pBindings = layout_bindings.data(),
            };
            raytracing_descriptor_set_layout_.emplace_back(device, layout_info);
        }
        {
            // set = 2
            constexpr auto kLayoutBindings = std::to_array({
                // geometry
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags =
                        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                },
            });
            const auto layout_info = VkDescriptorSetLayoutCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
                .pBindings = kLayoutBindings.data(),
            };
            raytracing_descriptor_set_layout_.emplace_back(device, layout_info);
        }
    }();

    spdlog::debug("create descriptor pool");
    [&]() {
        const auto num_model = static_cast<uint32_t>(scene.GetModels().size());
        const auto pool_sizes = std::to_array({
            // top level acceleration structure
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
            // finalized
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
            // camera + light + transform
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * 3,
            },
            // color + normal + emissive + occlusion roughness metallic
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight) * num_model * 4,
            },
            // geometry
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
        });
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = kMaxFramesInFlight * kNumDescriptorSetRaytracing * num_model,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };
        raytracing_descriptor_pool_.emplace(device, pool_info);
    }();

    spdlog::debug("create descriptor set");
    [&]() {
        // allocate
        spdlog::debug("allocate descriptor sets");
        raytracing_descriptor_sets_.reserve(kMaxFramesInFlight);
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            auto set_layout = std::vector<VkDescriptorSetLayout>();
            set_layout.reserve(raytracing_descriptor_sets_.size());
            for (const auto& layout : raytracing_descriptor_set_layout_) {
                set_layout.emplace_back(layout.GetVkDescriptorSetLayout());
            }

            const auto alloc_info = VkDescriptorSetAllocateInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = raytracing_descriptor_pool_->GetVkDescriptorPool(),
                .descriptorSetCount = static_cast<uint32_t>(set_layout.size()),
                .pSetLayouts = set_layout.data(),
            };
            raytracing_descriptor_sets_.emplace_back(device, alloc_info);
        }

        // update descriptor sets
        spdlog::debug("update descriptor sets");
        // aggregate image view
        const auto get_image_view = [&](const auto texture) -> VkImageView {
            if (texture == nullptr) {
                return VK_NULL_HANDLE;
            }
            return texture->GetImageView();
        };

        // base color
        auto base_color_image_infos = std::vector<VkDescriptorImageInfo>();
        base_color_image_infos.reserve(scene.GetModels().size());

        // normal
        auto normal_image_infos = std::vector<VkDescriptorImageInfo>();
        normal_image_infos.reserve(scene.GetModels().size());

        // emissive
        auto emissive_image_infos = std::vector<VkDescriptorImageInfo>();
        emissive_image_infos.reserve(scene.GetModels().size());

        // occlusion roughness metallic
        auto occlusion_roughness_metallic_image_infos = std::vector<VkDescriptorImageInfo>();
        occlusion_roughness_metallic_image_infos.reserve(scene.GetModels().size());

        for (const auto& model : scene.GetModels()) {
            const auto base_color_image_view = get_image_view(model.GetBaseColorTexture());
            base_color_image_infos.emplace_back(VkDescriptorImageInfo{
                .sampler = texture_samplers_.at(TextureSamplerType::kColor)->GetSampler(),
                .imageView = base_color_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });

            const auto normal_image_view = get_image_view(model.GetNormalTexture());
            normal_image_infos.emplace_back(VkDescriptorImageInfo{
                .sampler = texture_samplers_.at(TextureSamplerType::kNormal)->GetSampler(),
                .imageView = normal_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });

            const auto emissive_image_view = get_image_view(model.GetEmissiveTexture());
            emissive_image_infos.emplace_back(VkDescriptorImageInfo{
                .sampler = texture_samplers_.at(TextureSamplerType::kEmmisive)->GetSampler(),
                .imageView = emissive_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });

            const auto occlusion_roughness_metallic_image_view =
                get_image_view(model.GetMetallicRoughnessTexture());
            occlusion_roughness_metallic_image_infos.emplace_back(VkDescriptorImageInfo{
                .sampler = texture_samplers_.at(TextureSamplerType::kOcclusionRoughnessMetallic)
                               ->GetSampler(),
                .imageView = occlusion_roughness_metallic_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            });
        }

        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto tlas_info = VkWriteDescriptorSetAccelerationStructureKHR{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                .accelerationStructureCount = 1,
                .pAccelerationStructures = &top_level_as_->MutableHandle(),
            };
            const auto camera_matrix_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = camera_matrix_ubo.GetVkBufferUniform(frame_i),
                .offset = 0,
                .range = camera_matrix_ubo.GetUniformBufferObjectSize(),
            };
            const auto light_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = light_ubo.GetVkBufferUniform(frame_i),
                .offset = 0,
                .range = light_ubo.GetUniformBufferObjectSize(),
            };
            const auto transform_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = transform_ubo.GetVkBufferUniform(frame_i),
                .offset = 0,
                .range = transform_ubo.GetUniformBufferObjectSize(),
            };
            const auto finalized_image_info = VkDescriptorImageInfo{
                .imageView =
                    render_targets_.at(RenderTargetType::kFinalized).value().GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };
            const auto geometry_buffer_info = VkDescriptorBufferInfo{
                .buffer = geometry_node_buffer_->GetVkBuffer(),
                .offset = 0,
                .range = geometry_node_buffer_->GetSize(),
            };
            spdlog::debug("geometry buffer size: {}", sizeof(GeometryNode));

            const auto descriptor_write = std::to_array(
                {VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .pNext = &tlas_info,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                     .dstBinding = 0,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                     .dstBinding = 1,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     .pBufferInfo = &camera_matrix_ubo_buffer_info,
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                     .dstBinding = 2,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     .pBufferInfo = &light_ubo_buffer_info,
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                     .dstBinding = 3,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                     .pBufferInfo = &transform_ubo_buffer_info,
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                     .dstBinding = 0,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                     .pImageInfo = &finalized_image_info,
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                     .dstBinding = 1,
                     .dstArrayElement = 0,
                     .descriptorCount = static_cast<uint32_t>(base_color_image_infos.size()),
                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     .pImageInfo = base_color_image_infos.data(),
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                     .dstBinding = 2,
                     .dstArrayElement = 0,
                     .descriptorCount = static_cast<uint32_t>(normal_image_infos.size()),
                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     .pImageInfo = normal_image_infos.data(),
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                     .dstBinding = 3,
                     .dstArrayElement = 0,
                     .descriptorCount = static_cast<uint32_t>(emissive_image_infos.size()),
                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     .pImageInfo = emissive_image_infos.data(),
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(1),
                     .dstBinding = 4,
                     .dstArrayElement = 0,
                     .descriptorCount =
                         static_cast<uint32_t>(occlusion_roughness_metallic_image_infos.size()),
                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                     .pImageInfo = occlusion_roughness_metallic_image_infos.data(),
                 },
                 VkWriteDescriptorSet{
                     .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                     .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(2),
                     .dstBinding = 0,
                     .dstArrayElement = 0,
                     .descriptorCount = 1,
                     .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                     .pBufferInfo = &geometry_buffer_info,
                 }});
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                   descriptor_write.data(), 0, nullptr);
        }
    }();

    spdlog::debug("create pipeline layout");
    [&]() {
        constexpr auto kPushConstantRanges = std::to_array({VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
                          VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .offset = 0,
            .size = sizeof(uint32_t),
        }});
        raytracing_pipeline_layout_.reserve(kMaxFramesInFlight);
        for (auto i = 0; i < kMaxFramesInFlight; i++) {
            auto set_layout = std::vector<VkDescriptorSetLayout>();
            set_layout.reserve(raytracing_descriptor_set_layout_.size());
            for (const auto& layout : raytracing_descriptor_set_layout_) {
                set_layout.emplace_back(layout.GetVkDescriptorSetLayout());
            }
            VkPipelineLayoutCreateInfo layout_info{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = static_cast<uint32_t>(set_layout.size()),
                .pSetLayouts = set_layout.data(),
                .pushConstantRangeCount = static_cast<uint32_t>(kPushConstantRanges.size()),
                .pPushConstantRanges = kPushConstantRanges.data(),
            };
            raytracing_pipeline_layout_.emplace_back(device, layout_info);
        }
    }();

    spdlog::debug("create pipeline");
    [&]() {
        const auto rgen_path = std::filesystem::path("raytracing/raygen.rgen.spv");
        const auto miss_path = std::filesystem::path("raytracing/miss.rmiss.spv");
        const auto shadow_miss_path = std::filesystem::path("raytracing/shadow.rmiss.spv");
        const auto rchit_path = std::filesystem::path("raytracing/closesthit.rchit.spv");
        const auto rahit_path = std::filesystem::path("raytracing/anyhit.rahit.spv");

        const auto rgen_shader = Shader(rgen_path, VK_SHADER_STAGE_RAYGEN_BIT_KHR, device);
        const auto miss_shader = Shader(miss_path, VK_SHADER_STAGE_MISS_BIT_KHR, device);
        const auto shadow_miss_shader =
            Shader(shadow_miss_path, VK_SHADER_STAGE_MISS_BIT_KHR, device);
        const auto rchit_shader = Shader(rchit_path, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, device);
        const auto rahit_shader = Shader(rahit_path, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, device);

        auto shader_stages = std::vector<VkPipelineShaderStageCreateInfo>();

        spdlog::debug("setup raygen");
        [&]() {
            shader_stages.emplace_back(rgen_shader.GetStageInfo());
            const auto shader_group = VkRayTracingShaderGroupCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader = static_cast<uint32_t>(shader_stages.size()) - 1,
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR,
            };
            shader_groups_.emplace_back(shader_group);
        }();

        spdlog::debug("setup miss");
        [&]() {
            shader_stages.emplace_back(miss_shader.GetStageInfo());
            const auto shader_group = VkRayTracingShaderGroupCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader = static_cast<uint32_t>(shader_stages.size()) - 1,
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR,
            };
            shader_groups_.emplace_back(shader_group);

            // second shader for shadow
            shader_stages.emplace_back(shadow_miss_shader.GetStageInfo());
            const auto shadow_shader_group = VkRayTracingShaderGroupCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                .generalShader = static_cast<uint32_t>(shader_stages.size()) - 1,
                .closestHitShader = VK_SHADER_UNUSED_KHR,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
                .intersectionShader = VK_SHADER_UNUSED_KHR,
            };
            shader_groups_.emplace_back(shadow_shader_group);
        }();

        spdlog::debug("setup hit");
        [&]() {
            shader_stages.emplace_back(rchit_shader.GetStageInfo());
            shader_stages.emplace_back(rahit_shader.GetStageInfo());
            const auto shader_group = VkRayTracingShaderGroupCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                .generalShader = VK_SHADER_UNUSED_KHR,
                .closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 2,
                .anyHitShader = static_cast<uint32_t>(shader_stages.size()) - 1,
                .intersectionShader = VK_SHADER_UNUSED_KHR,
            };
            shader_groups_.emplace_back(shader_group);
        }();

        spdlog::debug("create raytracing pipeline");
        raytracing_pipeline_.reserve(kMaxFramesInFlight);
        for (auto i = 0; i < kMaxFramesInFlight; i++) {
            const auto pipeline_info = VkRayTracingPipelineCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
                .stageCount = static_cast<uint32_t>(shader_stages.size()),
                .pStages = shader_stages.data(),
                .groupCount = static_cast<uint32_t>(shader_groups_.size()),
                .pGroups = shader_groups_.data(),
                .maxPipelineRayRecursionDepth = 1,
                .layout = raytracing_pipeline_layout_.at(i).GetVkPipelineLayout(),
            };
            raytracing_pipeline_.emplace_back(device, pipeline_info);
        }
    }();

    spdlog::debug("create shader binding table");

    // TODO: kMaxFramesInFlight of shader binding table is needed?
    CreateShaderBindingTable(device, physical_device,
                             raytracing_pipeline_.at(0).GetVkRaytracingPipeline());
}

void DrawRaytracing::OnRecreateSwapChain(const DeviceResource& device_resource) {}

void DrawRaytracing::RecordCommandBuffer(const uint32_t image_idx,
                                         const VkExtent2D& swapchain_extent,
                                         const VkCommandBuffer command_buffer) {
    spdlog::debug("record command buffer");
    const auto handle_size = raytracing_pipeline_properties_.shaderGroupHandleSize;
    // align
    const auto handle_size_aligned =
        vlux::RoundUp(handle_size, raytracing_pipeline_properties_.shaderGroupHandleAlignment);

    const auto raygen_shader_sbt_entry = VkStridedDeviceAddressRegionKHR{
        .deviceAddress =
            GetBufferDeviceAddress(device_, raygen_shader_binding_table_->GetVkBuffer()),
        .stride = handle_size_aligned,
        .size = handle_size_aligned,
    };

    const auto miss_shader_sbt_entry = VkStridedDeviceAddressRegionKHR{
        .deviceAddress = GetBufferDeviceAddress(device_, miss_shader_binding_table_->GetVkBuffer()),
        .stride = handle_size_aligned,
        .size = handle_size_aligned,
    };

    const auto hit_shader_sbt_entry = VkStridedDeviceAddressRegionKHR{
        .deviceAddress = GetBufferDeviceAddress(device_, hit_shader_binding_table_->GetVkBuffer()),
        .stride = handle_size_aligned,
        .size = handle_size_aligned,
    };

    VkStridedDeviceAddressRegionKHR callable_shader_sbt_entry{};

    spdlog::debug("dispatch ray tracing commands");
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                      raytracing_pipeline_.at(image_idx).GetVkRaytracingPipeline());

    vkCmdBindDescriptorSets(
        command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        raytracing_pipeline_layout_.at(image_idx).GetVkPipelineLayout(), 0,
        static_cast<uint32_t>(raytracing_descriptor_sets_.at(image_idx).GetSize()),
        raytracing_descriptor_sets_.at(image_idx).GetVkDescriptorSetPtr(), 0, 0);

    vkCmdPushConstants(command_buffer,
                       raytracing_pipeline_layout_.at(image_idx).GetVkPipelineLayout(),
                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, 0,
                       sizeof(uint32_t), &mode_);

    vkCmdTraceRaysKHR(command_buffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry,
                      &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                      static_cast<uint32_t>(swapchain_extent.width),
                      static_cast<uint32_t>(swapchain_extent.height), 1);
    spdlog::debug("finish recording command buffer");
}

void DrawRaytracing::CreateBottomLevelAS(const VkDevice device,
                                         const VkPhysicalDevice physical_device,
                                         const VkQueue queue, const VkCommandPool command_pool) {
    // Setup identity transform matrix
    const auto transform_matrix = VkTransformMatrixKHR{
        .matrix =
            {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
            },
    };

    const auto num_models = scene_.GetModels().size();
    bottom_level_as_.reserve(num_models);
    vertex_buffer_.reserve(num_models);
    index_buffer_.reserve(num_models);
    transform_buffer_.reserve(num_models);
    geometry_nodes_.reserve(num_models);
    for (auto model_i = 0; const auto& model : scene_.GetModels()) {
        const auto& vertices = model.GetVertexBuffers().at(0).GetVertices();
        const auto& indices = model.GetIndexBuffers().at(0).GetIndices();

        // Vertex buffer
        spdlog::debug("create vertex buffer");
        vertex_buffer_.emplace_back(
            device, physical_device,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(Vertex) * vertices.size(), vertices.data());

        // Index buffer
        spdlog::debug("create index buffer");
        index_buffer_.emplace_back(
            device, physical_device,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(Index) * indices.size(), indices.data());

        // Transform buffer
        spdlog::debug("create transform buffer");
        transform_buffer_.emplace_back(
            device, physical_device,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(VkTransformMatrixKHR), &transform_matrix);

        // Build
        auto geometries = std::vector<VkAccelerationStructureGeometryKHR>();
        geometries.emplace_back(VkAccelerationStructureGeometryKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            .geometry =
                VkAccelerationStructureGeometryDataKHR{
                    .triangles =
                        VkAccelerationStructureGeometryTrianglesDataKHR{
                            .sType =
                                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                            .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
                            .vertexData =
                                {
                                    .deviceAddress = GetBufferDeviceAddress(
                                        device, vertex_buffer_.back().GetVkBuffer()),
                                },
                            .vertexStride = sizeof(Vertex),
                            .maxVertex = static_cast<uint32_t>(vertices.size() - 1),
                            .indexType = std::is_same<Index, uint16_t>::value
                                             ? VK_INDEX_TYPE_UINT16
                                             : VK_INDEX_TYPE_UINT32,
                            .indexData =
                                {
                                    .deviceAddress = GetBufferDeviceAddress(
                                        device, index_buffer_.back().GetVkBuffer()),
                                },
                            .transformData =
                                {
                                    .deviceAddress = GetBufferDeviceAddress(
                                        device, transform_buffer_.back().GetVkBuffer()),
                                }},
                },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
        });

        geometry_nodes_.emplace_back(GeometryNode{
            .vertex_buffer_device_address =
                GetBufferDeviceAddress(device, vertex_buffer_.back().GetVkBuffer()),
            .index_buffer_device_address =
                GetBufferDeviceAddress(device, index_buffer_.back().GetVkBuffer()),
            .texture_index_base_color =
                model.GetBaseColorTexture() == nullptr ? -1 : static_cast<int32_t>(model_i),
            .texture_index_normal =
                model.GetNormalTexture() == nullptr ? -1 : static_cast<int32_t>(model_i),
            .texture_index_emissive =
                model.GetEmissiveTexture() == nullptr ? -1 : static_cast<int32_t>(model_i),
            .texture_index_occlusion_roughness_metallic =
                model.GetMetallicRoughnessTexture() == nullptr ? -1 : static_cast<int32_t>(model_i),
        });

        // Get size info
        const auto acceleration_structure_build_geometry_info =
            VkAccelerationStructureBuildGeometryInfoKHR{
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
                .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                         VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
                .geometryCount = static_cast<uint32_t>(geometries.size()),
                .pGeometries = geometries.data(),
            };

        auto acceleration_structure_build_sizes_info = VkAccelerationStructureBuildSizesInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        };
        assert(indices.size() % 3 == 0);
        const auto num_triangles = static_cast<uint32_t>(indices.size() / 3);
        vkGetAccelerationStructureBuildSizesKHR(
            device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &acceleration_structure_build_geometry_info, &num_triangles,
            &acceleration_structure_build_sizes_info);

        // Create acceleration structure
        bottom_level_as_.emplace_back(device, physical_device,
                                      acceleration_structure_build_sizes_info);

        const auto acceleration_structure_createInfo = VkAccelerationStructureCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .buffer = bottom_level_as_.back().GetBuffer(),
            .size = acceleration_structure_build_sizes_info.accelerationStructureSize,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        };
        vkCreateAccelerationStructureKHR(device, &acceleration_structure_createInfo, nullptr,
                                         &bottom_level_as_.back().MutableHandle());

        // Create a small scratch buffer used during build of the bottom level acceleration
        // structure
        auto scratch_buffer = RayTracingScratchBuffer(
            device, physical_device, acceleration_structure_build_sizes_info.buildScratchSize);

        const auto geometry_info = VkAccelerationStructureBuildGeometryInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .dstAccelerationStructure = bottom_level_as_.back().GetHandle(),
            .geometryCount = static_cast<uint32_t>(geometries.size()),
            .pGeometries = geometries.data(),
            .scratchData =
                {
                    .deviceAddress = scratch_buffer.GetDeviceAddress(),
                },
        };

        const auto range_info = VkAccelerationStructureBuildRangeInfoKHR{
            .primitiveCount = num_triangles,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0,
        };
        const auto range_infos = std::to_array({
            &range_info,
        });
        const auto command_buffer = BeginSingleTimeCommands(command_pool, device);
        vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &geometry_info, range_infos.data());
        EndSingleTimeCommands(command_buffer, queue, command_pool, device);

        const auto acceleration_device_address_info = VkAccelerationStructureDeviceAddressInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .accelerationStructure = bottom_level_as_.back().GetHandle(),
        };
        bottom_level_as_.back().SetDeviceAddress(
            vkGetAccelerationStructureDeviceAddressKHR(device, &acceleration_device_address_info));

        model_i++;
    }

    geometry_node_buffer_.emplace(
        device, physical_device,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        static_cast<uint32_t>(geometry_nodes_.size()) * sizeof(GeometryNode),
        geometry_nodes_.data());
}

void DrawRaytracing::CreateTopLevelAS(const VkDevice device, const VkPhysicalDevice physical_device,
                                      const VkQueue queue, const VkCommandPool command_pool) {
    const auto transform_matrix = VkTransformMatrixKHR{
        .matrix =
            {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
            },
    };

    auto instances = std::vector<VkAccelerationStructureInstanceKHR>();
    instances.reserve(bottom_level_as_.size());
    for (const auto& blas : bottom_level_as_) {
        instances.emplace_back(VkAccelerationStructureInstanceKHR{
            .transform = transform_matrix,
            .instanceCustomIndex = 0,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = blas.GetDeviceAddress(),
        });
    }
    // Buffer for instance data
    const auto instance_buffer =
        Buffer(device, physical_device,
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               sizeof(VkAccelerationStructureInstanceKHR) * instances.size(), instances.data());
    const auto instance_data_device_address = VkDeviceOrHostAddressConstKHR{
        .deviceAddress = GetBufferDeviceAddress(device, instance_buffer.GetVkBuffer()),
    };
    const auto acceleration_structure_geometry = std::to_array({
        VkAccelerationStructureGeometryKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry =
                {
                    .instances =
                        {
                            .sType =
                                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                            .arrayOfPointers = VK_FALSE,
                            .data = instance_data_device_address,
                        },
                },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
        },
    });

    const auto acceleration_structure_build_geometry_info =
        VkAccelerationStructureBuildGeometryInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                     VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
            .geometryCount = static_cast<uint32_t>(acceleration_structure_geometry.size()),
            .pGeometries = acceleration_structure_geometry.data(),
        };

    const auto primitive_count = static_cast<uint32_t>(bottom_level_as_.size());
    auto acceleration_structure_build_sizes_info = VkAccelerationStructureBuildSizesInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &acceleration_structure_build_geometry_info,
                                            &primitive_count,
                                            &acceleration_structure_build_sizes_info);
    top_level_as_.emplace(device, physical_device, acceleration_structure_build_sizes_info);

    const auto acceleration_structure_create_info = VkAccelerationStructureCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = top_level_as_->GetBuffer(),
        .size = acceleration_structure_build_sizes_info.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };
    vkCreateAccelerationStructureKHR(device, &acceleration_structure_create_info, nullptr,
                                     &top_level_as_->MutableHandle());

    // Create a small scratch buffer used during build of the top level acceleration structure
    const auto scratch_buffer = RayTracingScratchBuffer(
        device, physical_device, acceleration_structure_build_sizes_info.buildScratchSize);

    const auto acceleration_build_geometry_info = VkAccelerationStructureBuildGeometryInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                 VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_DATA_ACCESS_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = top_level_as_->GetHandle(),
        .geometryCount = static_cast<uint32_t>(acceleration_structure_geometry.size()),
        .pGeometries = acceleration_structure_geometry.data(),
        .scratchData =
            {
                .deviceAddress = scratch_buffer.GetDeviceAddress(),
            },
    };

    auto acceleration_structure_build_range_info = VkAccelerationStructureBuildRangeInfoKHR{
        .primitiveCount = primitive_count,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
        acceleration_build_structure_range_infos = {&acceleration_structure_build_range_info};

    const auto command_buffer = BeginSingleTimeCommands(command_pool, device);
    vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &acceleration_build_geometry_info,
                                        acceleration_build_structure_range_infos.data());
    EndSingleTimeCommands(command_buffer, queue, command_pool, device);

    const auto acceleration_device_address_info = VkAccelerationStructureDeviceAddressInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = top_level_as_->GetHandle(),
    };
    top_level_as_->SetDeviceAddress(
        vkGetAccelerationStructureDeviceAddressKHR(device, &acceleration_device_address_info));
}

void DrawRaytracing::CreateShaderBindingTable(const VkDevice device,
                                              const VkPhysicalDevice physical_device,
                                              const VkPipeline pipeline) {
    const auto handle_size = raytracing_pipeline_properties_.shaderGroupHandleSize;
    // align
    const auto handle_size_aligned =
        vlux::RoundUp(handle_size, raytracing_pipeline_properties_.shaderGroupHandleAlignment);

    const auto group_count = static_cast<uint32_t>(shader_groups_.size());
    const auto sbt_size = group_count * handle_size_aligned;

    auto shader_handle_storage = std::vector<uint8_t>(sbt_size);
    if (vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, group_count, sbt_size,
                                             shader_handle_storage.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get ray tracing shader group handles");
    }

    constexpr VkBufferUsageFlags kBufferUsageFlags =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    constexpr VkMemoryPropertyFlags kMemoryPropertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    spdlog::debug("create raygen shader binding table");
    raygen_shader_binding_table_.emplace(device, physical_device, kBufferUsageFlags,
                                         kMemoryPropertyFlags, handle_size,
                                         shader_handle_storage.data());

    spdlog::debug("create miss shader binding table");
    miss_shader_binding_table_.emplace(device, physical_device, kBufferUsageFlags,
                                       kMemoryPropertyFlags, handle_size * 2,
                                       shader_handle_storage.data() + handle_size_aligned);

    spdlog::debug("create hit shader binding table");
    hit_shader_binding_table_.emplace(device, physical_device, kBufferUsageFlags,
                                      kMemoryPropertyFlags, handle_size,
                                      shader_handle_storage.data() + handle_size_aligned * 3);

    spdlog::debug("finish creating shader binding table");
}

uint64_t DrawRaytracing::GetBufferDeviceAddress(const VkDevice device, const VkBuffer buffer) {
    const auto buffer_device_address_info = VkBufferDeviceAddressInfoKHR{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddressKHR(device, &buffer_device_address_info);
}

}  // namespace vlux::draw::raytracing
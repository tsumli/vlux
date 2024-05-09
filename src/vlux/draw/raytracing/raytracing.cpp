#include "raytracing.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

#include "common/buffer.h"
#include "common/descriptor_set_layout.h"
#include "scratch_buffer.h"
#include "shader/shader.h"
namespace vlux::draw::raytracing {
namespace {
constexpr auto kNumDescriptorSetRaytracing = 1;
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
        constexpr auto kLayoutBindings = std::to_array({
            VkDescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            },
            VkDescriptorSetLayoutBinding{
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            },
        });
        const auto layout_info = VkDescriptorSetLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(kLayoutBindings.size()),
            .pBindings = kLayoutBindings.data(),
        };
        raytracing_descriptor_set_layout_.emplace_back(device, layout_info);
    }();

    spdlog::debug("create descriptor pool");
    [&]() {
        const auto pool_sizes = std::to_array({
            // top level acceleration structure
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight),
            },
        });
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = kMaxFramesInFlight * kNumDescriptorSetRaytracing,
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
        for (auto frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto descriptor_acceleration_structure_info =
                VkWriteDescriptorSetAccelerationStructureKHR{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                    .accelerationStructureCount = 1,
                    .pAccelerationStructures = &top_level_as_->MutableHandle(),
                };

            const auto storage_image_info = VkDescriptorImageInfo{
                .imageView =
                    render_targets_.at(RenderTargetType::kFinalized).value().GetVkImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };

            const auto camera_matrix_ubo_buffer_info = VkDescriptorBufferInfo{
                .buffer = camera_matrix_ubo.GetVkBufferUniform(frame_i),
                .offset = 0,
                .range = camera_matrix_ubo.GetUniformBufferObjectSize(),
            };

            const auto descriptor_write = std::to_array({
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = &descriptor_acceleration_structure_info,
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
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo = &storage_image_info,
                },
                VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = raytracing_descriptor_sets_.at(frame_i).GetVkDescriptorSet(0),
                    .dstBinding = 2,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &camera_matrix_ubo_buffer_info,
                },
            });

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_write.size()),
                                   descriptor_write.data(), 0, nullptr);
        }
    }();

    spdlog::debug("create pipeline layout");
    [&]() {
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
            };
            raytracing_pipeline_layout_.emplace_back(device, layout_info);
        }
    }();

    spdlog::debug("create pipeline");
    [&]() {
        const auto rgen_path = std::filesystem::path("raytracing/raygen.rgen.spv");
        const auto miss_path = std::filesystem::path("raytracing/miss.rmiss.spv");
        const auto rchit_path = std::filesystem::path("raytracing/closesthit.rchit.spv");

        const auto rgen_shader = Shader(rgen_path, VK_SHADER_STAGE_RAYGEN_BIT_KHR, device);
        const auto miss_shader = Shader(miss_path, VK_SHADER_STAGE_MISS_BIT_KHR, device);
        const auto rchit_shader = Shader(rchit_path, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, device);

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
        }();

        spdlog::debug("setup hit");
        [&]() {
            shader_stages.emplace_back(rchit_shader.GetStageInfo());
            const auto shader_group = VkRayTracingShaderGroupCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                .generalShader = VK_SHADER_UNUSED_KHR,
                .closestHitShader = static_cast<uint32_t>(shader_stages.size()) - 1,
                .anyHitShader = VK_SHADER_UNUSED_KHR,
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
        (handle_size + raytracing_pipeline_properties_.shaderGroupHandleAlignment - 1) &
        ~(raytracing_pipeline_properties_.shaderGroupHandleAlignment - 1);

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

    vkCmdTraceRaysKHR(command_buffer, &raygen_shader_sbt_entry, &miss_shader_sbt_entry,
                      &hit_shader_sbt_entry, &callable_shader_sbt_entry,
                      static_cast<uint32_t>(swapchain_extent.width),
                      static_cast<uint32_t>(swapchain_extent.height), 1);
    spdlog::debug("finish recording command buffer");
}

void DrawRaytracing::CreateBottomLevelAS(const VkDevice device,
                                         const VkPhysicalDevice physical_device,
                                         const VkQueue queue, const VkCommandPool command_pool) {
    // Setup vertices for a single triangle
    struct Vertex {
        float pos[3];
    };
    const auto vertices =
        std::vector<Vertex>{{{1.0f, 1.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}}};

    // Setup indices
    std::vector<uint16_t> indices = {0, 1, 2};

    // Setup identity transform matrix
    const auto transform_matrix = VkTransformMatrixKHR{
        .matrix =
            {
                {1.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, 1.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
            },
    };

    // Create buffers
    // For the sake of simplicity we won't stage the vertex data to the GPU memory
    // Vertex buffer
    spdlog::debug("create vertex buffer");
    vertex_buffer_.emplace(
        device, physical_device,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(Vertex) * vertices.size(), vertices.data());

    // Index buffer
    spdlog::debug("create index buffer");
    instance_buffer_.emplace(
        device, physical_device,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(uint16_t) * indices.size(), indices.data());

    // Transform buffer
    spdlog::debug("create transform buffer");
    transform_buffer_.emplace(
        device, physical_device,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sizeof(VkTransformMatrixKHR), &transform_matrix);

    // Build
    const auto geometries = std::to_array({
        VkAccelerationStructureGeometryKHR{
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
                                        device, vertex_buffer_->GetVkBuffer()),
                                },
                            .vertexStride = sizeof(Vertex),
                            .maxVertex = 2,
                            .indexType = VK_INDEX_TYPE_UINT16,
                            .indexData =
                                {
                                    .deviceAddress = GetBufferDeviceAddress(
                                        device, instance_buffer_->GetVkBuffer()),
                                },
                            .transformData =
                                {
                                    .deviceAddress = GetBufferDeviceAddress(
                                        device, transform_buffer_->GetVkBuffer()),
                                }},
                },
            .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
        },
    });

    // Get size info
    const auto acceleration_structure_build_geometry_info =
        VkAccelerationStructureBuildGeometryInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .geometryCount = static_cast<uint32_t>(geometries.size()),
            .pGeometries = geometries.data(),
        };

    const auto num_triangles = static_cast<uint32_t>(1);
    auto acceleration_structure_build_sizes_info = VkAccelerationStructureBuildSizesInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &acceleration_structure_build_geometry_info,
                                            &num_triangles,
                                            &acceleration_structure_build_sizes_info);

    // Create acceleration structure
    bottom_level_as_.emplace(device, physical_device, acceleration_structure_build_sizes_info);

    const auto acceleration_structure_createInfo = VkAccelerationStructureCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = bottom_level_as_->GetBuffer(),
        .size = acceleration_structure_build_sizes_info.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
    };
    vkCreateAccelerationStructureKHR(device, &acceleration_structure_createInfo, nullptr,
                                     &bottom_level_as_->MutableHandle());

    // Create a small scratch buffer used during build of the bottom level acceleration
    // structure
    auto scratch_buffer = RayTracingScratchBuffer(
        device, physical_device, acceleration_structure_build_sizes_info.buildScratchSize);

    const auto geometry_info = VkAccelerationStructureBuildGeometryInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .dstAccelerationStructure = bottom_level_as_->GetHandle(),
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
        .accelerationStructure = bottom_level_as_->GetHandle(),
    };
    bottom_level_as_->SetDeviceAddress(
        vkGetAccelerationStructureDeviceAddressKHR(device, &acceleration_device_address_info));
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

    auto instance = VkAccelerationStructureInstanceKHR{
        .transform = transform_matrix,
        .instanceCustomIndex = 0,
        .mask = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference = bottom_level_as_->GetDeviceAddress(),
    };

    // Buffer for instance data
    const auto instance_buffer =
        Buffer(device, physical_device,
               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               sizeof(VkAccelerationStructureInstanceKHR), &instance);

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

    // Get size info
    /*
    The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are
    ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command,
    except that the hostAddress member of
    VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if
    it is NULL.*
    */
    const auto acceleration_structure_build_geometry_info =
        VkAccelerationStructureBuildGeometryInfoKHR{
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
            .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            .geometryCount = static_cast<uint32_t>(acceleration_structure_geometry.size()),
            .pGeometries = acceleration_structure_geometry.data(),
        };

    const auto primitive_count = static_cast<uint32_t>(1);

    auto acceleration_structure_build_sizes_info = VkAccelerationStructureBuildSizesInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };
    vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &acceleration_structure_build_geometry_info,
                                            &primitive_count,
                                            &acceleration_structure_build_sizes_info);
    top_level_as_.emplace(device, physical_device, acceleration_structure_build_sizes_info);

    const auto accelerationStructureCreateInfo = VkAccelerationStructureCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .buffer = top_level_as_->GetBuffer(),
        .size = acceleration_structure_build_sizes_info.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
    };

    vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr,
                                     &top_level_as_->MutableHandle());

    // Create a small scratch buffer used during build of the top level acceleration structure
    const auto scratch_buffer = RayTracingScratchBuffer(
        device, physical_device, acceleration_structure_build_sizes_info.buildScratchSize);

    const auto acceleration_build_geometry_info = VkAccelerationStructureBuildGeometryInfoKHR{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
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
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = {
        &acceleration_structure_build_range_info};

    const auto command_buffer = BeginSingleTimeCommands(command_pool, device);
    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host
    // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
    // but we prefer device builds
    vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &acceleration_build_geometry_info,
                                        accelerationBuildStructureRangeInfos.data());
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
        (handle_size + raytracing_pipeline_properties_.shaderGroupHandleAlignment - 1) &
        ~(raytracing_pipeline_properties_.shaderGroupHandleAlignment - 1);

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
                                       kMemoryPropertyFlags, handle_size,
                                       shader_handle_storage.data() + handle_size_aligned);

    spdlog::debug("create hit shader binding table");
    hit_shader_binding_table_.emplace(device, physical_device, kBufferUsageFlags,
                                      kMemoryPropertyFlags, handle_size,
                                      shader_handle_storage.data() + handle_size_aligned * 2);

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
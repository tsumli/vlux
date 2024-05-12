#include "app.h"

#include <vulkan/vulkan_core.h>

#include <exception>
#include <vector>

#include "camera.h"
#include "common/command_buffer.h"
#include "common/queue.h"
#include "control.h"
#include "cubemap/cubemap.h"
#include "device_resource/device.h"
#include "draw/rasterize/rasterize.h"
#include "draw/raytracing/raytracing.h"
#include "gui.h"
#include "imgui.h"
#include "light.h"
#include "model/gltf.h"
#include "model/model.h"
#include "uniform_buffer.h"
#include "utils/io.h"
#include "utils/path.h"

namespace vlux {
App::App(DeviceResource& device_resource, const std::string_view scene_name)
    : device_resource_(device_resource),
      transform_ubo_(device_resource_.GetDevice().GetVkDevice(),
                     device_resource_.GetVkPhysicalDevice()),
      camera_ubo_(device_resource_.GetDevice().GetVkDevice(),
                  device_resource_.GetVkPhysicalDevice()),
      camera_matrix_ubo_(device_resource_.GetDevice().GetVkDevice(),
                         device_resource_.GetVkPhysicalDevice()),
      light_ubo_(device_resource_.GetDevice().GetVkDevice(),
                 device_resource_.GetVkPhysicalDevice()),
      config_(ReadJsonFile(GetCurrentDir() / "config.json")),
      scene_name_(scene_name) {
    const auto device = device_resource_.GetDevice().GetVkDevice();
    const auto physical_device = device_resource_.GetVkPhysicalDevice();

    // Resource Creation
    command_pool_.emplace(device, physical_device, device_resource_.GetSurface().GetVkSurface());
    command_buffer_.emplace(command_pool_->GetVkCommandPool(), device);

    // Create Scene
    CreateScene();

    // setup control
    control_.emplace(device_resource_.GetGLFWwindow());
    const auto [width, height] = device_resource.GetWindow().GetWindowSize();
    camera_.emplace(glm::vec3{80.0f, 20.0f, -12.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, width, height);

    // setup obejets
    for (const auto& config_light : config_.at("lights")) {
        const auto pos = config_light.at("pos").get<std::array<float, 3>>();
        const auto range = config_light.at("range").get<float>();
        const auto color = config_light.at("color").get<std::array<float, 4>>();
        lights_.emplace_back(LightParams{
            .pos = glm::vec4(pos[0], pos[1], pos[2], 0.0f),
            .range = range,
            .color = glm::vec4(color[0], color[1], color[2], color[3]),
        });
    }

    // setup draw
    spdlog::debug("setup draw");
    draw_mode_ = config_.at("draw_mode").get<std::string>();
    if (draw_mode_ == "rasterize") {
        draw_ = std::make_unique<draw::rasterize::DrawRasterize>(
            transform_ubo_, camera_ubo_, light_ubo_, scene_.value(), device_resource_);
    } else if (draw_mode_ == "raytracing") {
        const auto queue = device_resource_.GetGraphicsComputeQueue();

        draw_ = std::make_unique<draw::raytracing::DrawRaytracing>(
            transform_ubo_, camera_ubo_, camera_matrix_ubo_, light_ubo_, scene_.value(), queue,
            command_pool_->GetVkCommandPool(), device_resource_);
    } else {
        throw std::runtime_error("invalid draw mode");
    }

    // Setup GUI
    spdlog::debug("setup gui");
    const auto queue_family = FindQueueFamilies(device_resource_.GetVkPhysicalDevice(),
                                                device_resource_.GetSurface().GetVkSurface());
    const auto gui_input = GuiInput{
        .instance = device_resource_.GetInstance().GetVkInstance(),
        .device = device,
        .physical_device = physical_device,
        .window = device_resource_.GetGLFWwindow(),
        .queue_family = queue_family.graphics_compute_family.value(),
        .queue = device_resource_.GetGraphicsComputeQueue(),
        .image_count = device_resource_.GetSwapchain().GetImageCount(),
        .swapchain_format = device_resource_.GetSwapchain().GetVkFormat(),
        .swapchain_image_views = device_resource_.GetSwapchain().GetVkImageViews(),
    };
    gui_.emplace(gui_input);
}

App::~App() {}

void App::CreateScene() {
    spdlog::debug("create scene");
    const auto device = device_resource_.GetDevice().GetVkDevice();
    const auto physical_device = device_resource_.GetVkPhysicalDevice();
    const auto command_pool = command_pool_->GetVkCommandPool();
    const auto graphics_queue = device_resource_.GetGraphicsComputeQueue();

    auto models = std::vector<Model>();
    const auto scene_config = config_.at("scenes").at(scene_name_);
    spdlog::debug("load scene: {}", scene_name_);
    timer_.Reset();
    for (const auto& model_config : scene_config.at("models")) {
        // unpack config
        const auto name = model_config.at("name").get<std::string>();
        const auto path = model_config.at("path").get<std::filesystem::path>();
        const auto scale = model_config.at("scale").get<float>();
        const auto translation = model_config.at("translation").get<std::array<float, 3>>();
        const auto rotation = model_config.at("rotation").get<std::array<float, 3>>();

        spdlog::debug("load {}", name);
        const auto gltf_model = LoadTinyGltfModel(path);
        for (const auto& mesh : gltf_model.meshes) {
            for (const auto& primitive : mesh.primitives) {
                // gltf objects
                auto gltf_objects = LoadGltfObjects(
                    primitive, gltf_model, graphics_queue, command_pool, physical_device, device,
                    scale, glm::vec3(translation[0], translation[1], translation[2]),
                    glm::vec3(rotation[0], rotation[1], rotation[2]));
                auto vertex_buffers = std::vector<VertexBuffer>();
                vertex_buffers.emplace_back(device, physical_device, graphics_queue, command_pool,
                                            std::move(gltf_objects.vertices));
                // index
                auto index_buffers = std::vector<IndexBuffer>();
                index_buffers.emplace_back(device, physical_device, graphics_queue, command_pool,
                                           std::move(gltf_objects.indices));

                // create model
                auto model = Model(
                    device, physical_device, std::move(vertex_buffers), std::move(index_buffers),
                    std::move(gltf_objects.base_color_factor), gltf_objects.metallic_factor,
                    gltf_objects.roughness_factor, std::move(gltf_objects.base_color_texture),
                    std::move(gltf_objects.normal_texture),
                    std::move(gltf_objects.emissive_texture),
                    std::move(gltf_objects.occlusion_roughness_metallic_texture));
                models.emplace_back(std::move(model));
            }
        }
    }
    const auto cubemap_path = scene_config.value("cubemap", std::filesystem::path{});
    auto cubemap = [&]() -> std::optional<CubeMap> {
        if (cubemap_path.empty()) {
            return std::nullopt;
        }
        spdlog::debug("load cubemap: {}", cubemap_path.string());
        return CubeMap(cubemap_path);
    }();
    spdlog::debug("scene load time: {} ms", timer_.GetElapsedMilliseconds());
    scene_.emplace(std::move(models), std::move(cubemap));
}

void App::MainLoop() {
    spdlog::debug("main loop");
    while (!glfwWindowShouldClose(device_resource_.GetGLFWwindow())) {
        glfwPollEvents();
        DrawFrame();
    }
    device_resource_.DeviceWaitIdle();
}

void App::DrawFrame() {
    spdlog::debug("draw frame");

    // on init
    gui_->OnStart();
    frame_timer_.Update();
    const auto device = device_resource_.GetDevice().GetVkDevice();
    const auto& sync_object = device_resource_.GetSyncObject();
    const auto& swapchain = device_resource_.GetSwapchain();
    const auto command_buffer = command_buffer_->GetVkCommandBuffer();

    uint32_t image_idx;
    {
        auto result = vkAcquireNextImageKHR(device, swapchain.GetVkSwapchain(), UINT64_MAX,
                                            sync_object.GetVkImageAvailableSemaphore(),
                                            VK_NULL_HANDLE, &image_idx);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            device_resource_.RecreateSwapChain();
            draw_->OnRecreateSwapChain(device_resource_);
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
    }

    spdlog::debug("input");
    [&]() {
        auto& keyboard = control_->MutableKeyboard();
        auto& mouse = control_->MutableMouse();
        const auto key_input = keyboard.GetInput();
        if (key_input.exit == 1) {
            spdlog::info("Escape key was pressed to exit");
            std::terminate();
        }
        const auto elapsed_seconds = frame_timer_.GetElapsedSeconds();
        camera_->UpdatePosition(key_input.move_forward, key_input.move_right, key_input.move_up,
                                100.0f * elapsed_seconds);

        mouse.UpdateButton();
        const auto mouse_button_changes = mouse.GetButtonChanges();
        if (mouse_button_changes.right) {
            switch (mouse_right_button_state_) {
                using enum MouseRightButtonState;
                case kReleased:
                    mouse_right_button_state_ = kTriggered;
                    mouse.MakeCursorHidden();
                    break;
                case kTriggered:
                    mouse_right_button_state_ = kReleased;
                    mouse.RecenterCursor();
                    mouse.MakeCursorVisible();
                    break;
                default:
                    std::runtime_error("invalid mouse right button state");
            }
        }
        if (mouse_right_button_state_ == MouseRightButtonState::kTriggered) {
            const auto mouse_position = mouse.GetMouseDisplacement();
            mouse.RecenterCursor();
            camera_->UpdateRotation(mouse_position.first, mouse_position.second,
                                    mouse_config_.sens);
        }
    }();

    spdlog::debug("update ubo");
    [&]() {
        const auto transform_params = camera_->CreateTransformParams();
        transform_ubo_.UpdateUniformBuffer(transform_params, image_idx);
    }();
    [&]() {
        const auto camera_params = CameraParams{
            .position = glm::vec4(camera_->GetPosition(), 1.0f),
        };
        camera_ubo_.UpdateUniformBuffer(camera_params, image_idx);
    }();
    [&]() {
        const auto camera_matrix_params = camera_->CreateCameraMatrixParams();
        camera_matrix_ubo_.UpdateUniformBuffer(camera_matrix_params, image_idx);
    }();
    [&]() { light_ubo_.UpdateUniformBuffer(lights_.at(0), image_idx); }();

    spdlog::debug("reset and begin command buffer");
    [&]() {
        if (vkResetCommandBuffer(command_buffer, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to reset command buffer!");
        }
        const auto begin_info = VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
    }();

    spdlog::debug("record command buffer");
    draw_->RecordCommandBuffer(image_idx, swapchain.GetVkExtent(), command_buffer);

    const auto& output_render_target = draw_->GetOutputRenderTarget();

    // Write Swapchain
    spdlog::debug("write swapchain");
    [&]() {
        [&]() {
            const auto barrier = std::to_array({
                // Transition Image Layout
                VkImageMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = output_render_target.GetVkImage(),
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                },
                // Swapchain
                VkImageMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask = VK_ACCESS_2_NONE,
                    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = device_resource_.GetSwapchain().GetVkImages().at(image_idx),
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                },
            });

            const auto dependency_info = VkDependencyInfo{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = static_cast<uint32_t>(barrier.size()),
                .pImageMemoryBarriers = barrier.data(),
            };
            vkCmdPipelineBarrier2(command_buffer, &dependency_info);
        }();

        const auto image_copy = VkImageCopy{
            .srcSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                },
            .dstSubresource =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .layerCount = 1,
                },
            .extent =
                {
                    .width = swapchain.GetWidth(),
                    .height = swapchain.GetHeight(),
                    .depth = 1,
                },
        };
        vkCmdCopyImage(command_buffer, output_render_target.GetVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain.GetVkImages().at(image_idx),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

        // Transition Image Layout
        [&]() {
            // output_render_target
            const auto barrier = std::to_array({
                VkImageMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR,
                    .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = output_render_target.GetVkImage(),
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                },
                // Swapchain
                VkImageMemoryBarrier2{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                                    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
                                     VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = device_resource_.GetSwapchain().GetVkImages().at(image_idx),
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = 0,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                },
            });
            const auto dependency_info = VkDependencyInfo{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = static_cast<uint32_t>(barrier.size()),
                .pImageMemoryBarriers = barrier.data(),
            };
            vkCmdPipelineBarrier2(command_buffer, &dependency_info);
        }();
    }();

    // ImGui
    spdlog::debug("imgui pass");
    [&]() {
        ImGui::Begin("Stats");
        ImGui::Text("fps: %f", frame_timer_.GetFPS());
        const auto pos = camera_->GetPosition();
        ImGui::Text("camera pos: (%f, %f, %f)", pos.x, pos.y, pos.z);
        const auto rot = camera_->GetRotation();
        ImGui::Text("camera rot: (%f, %f)", rot.x, rot.y);
        ImGui::Text("Mouse cursor: %s",
                    mouse_right_button_state_ == MouseRightButtonState::kTriggered ? "enabled"
                                                                                   : "disabled");
        ImGui::End();

        ImGui::Begin("Control");
        ImGui::InputFloat("Mouse sens", &mouse_config_.sens);
        ImGui::End();

        ImGui::Begin("Render");
        auto mode = static_cast<int>(draw_->GetMode());
        ImGui::InputInt("Mode", &mode);
        if (mode < 0) {
            mode = 0;
        }
        draw_->SetMode(static_cast<uint32_t>(mode));
        ImGui::End();

        ImGui::Begin("Light");
        ImGui::SliderFloat3("pos", glm::value_ptr(lights_.at(0).pos), -100.0f, 100.0f);
        ImGui::SliderFloat("range", &lights_.at(0).range, 0.0f, 1000.0f);
        ImGui::ColorPicker4("color", glm::value_ptr(lights_.at(0).color));
        ImGui::End();

        gui_->Render(command_buffer, swapchain.GetWidth(), swapchain.GetHeight(), image_idx);
    }();

    // Transition Swapchain Layout
    spdlog::debug("transition swapchain layout");
    [&]() {
        // Swapchain
        const auto barrier = std::to_array({
            VkImageMemoryBarrier2{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .dstAccessMask = VK_ACCESS_2_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = device_resource_.GetSwapchain().GetVkImages().at(image_idx),
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            },
        });
        const auto dependency_info = VkDependencyInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = static_cast<uint32_t>(barrier.size()),
            .pImageMemoryBarriers = barrier.data(),
        };
        vkCmdPipelineBarrier2(command_buffer, &dependency_info);
    }();

    spdlog::debug("end command buffer");
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    spdlog::debug("submit command buffer");
    const auto wait_semaphore_submit_infos = std::vector<VkSemaphoreSubmitInfo>({{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = sync_object.GetVkImageAvailableSemaphore(),
        .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    }});
    const auto signal_semaphore_submit_infos = std::vector<VkSemaphoreSubmitInfo>({{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = sync_object.GetVkRenderFinishedSemaphore(),
    }});
    const auto command_buffers = std::to_array({
        VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = command_buffer,
        },
    });
    const auto submit_info = VkSubmitInfo2{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = static_cast<uint32_t>(wait_semaphore_submit_infos.size()),
        .pWaitSemaphoreInfos = wait_semaphore_submit_infos.data(),
        .commandBufferInfoCount = static_cast<uint32_t>(command_buffers.size()),
        .pCommandBufferInfos = command_buffers.data(),
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signal_semaphore_submit_infos.size()),
        .pSignalSemaphoreInfos = signal_semaphore_submit_infos.data(),
    };
    if (vkQueueSubmit2(device_resource_.GetGraphicsComputeQueue(), 1, &submit_info,
                       sync_object.GetVkInFlightFence()) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    spdlog::debug("present");
    const auto swapchains = std::vector<VkSwapchainKHR>{swapchain.GetVkSwapchain()};
    const auto wait_semaphores = std::vector<VkSemaphore>({
        sync_object.GetVkRenderFinishedSemaphore(),
    });
    auto results = std::vector<VkResult>(swapchains.size());
    const auto present_info = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = wait_semaphores.data(),
        .swapchainCount = static_cast<uint32_t>(swapchains.size()),
        .pSwapchains = swapchains.data(),
        .pImageIndices = &image_idx,
        .pResults = results.data(),
    };
    if (vkQueuePresentKHR(device_resource_.GetPresentQueue(), &present_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    spdlog::debug("fence: wait and reset");
    sync_object.WaitAndResetFences();
}

}  // namespace vlux
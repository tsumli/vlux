#include "app.h"

#include <vulkan/vulkan_core.h>

#include "common/command_buffer.h"
#include "common/queue.h"
#include "control.h"
#include "device_resource/device.h"
#include "draw/rasterize/rasterize.h"
#include "gui.h"
#include "imgui.h"
#include "model/gltf.h"
#include "model/model.h"
#include "uniform_buffer.h"

namespace vlux {
App::App(DeviceResource& device_resource)
    : device_resource_(device_resource),
      transform_ubo_(device_resource_.GetDevice().GetVkDevice(),
                     device_resource_.GetVkPhysicalDevice()) {
    // setup logger lovel
    spdlog::set_level(spdlog::level::debug);

    // Create Scene
    CreateScene();

    // setup control
    control_.emplace(device_resource_.GetGLFWwindow());
    const auto [width, height] = device_resource.GetWindow().GetWindowSize();
    camera_.emplace(glm::vec3{80.0f, 20.0f, -12.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, width, height);

    // setup draw
    draw_ = std::make_unique<draw::rasterize::DrawRasterize>(transform_ubo_, scene_.value(),
                                                             device_resource_);

    // Setup GUI
    const auto queue_family = FindQueueFamilies(device_resource_.GetVkPhysicalDevice(),
                                                device_resource_.GetSurface().GetVkSurface());
    const auto gui_input = GuiInput{
        .instance = device_resource_.GetInstance().GetVkInstance(),
        .device = device_resource_.GetDevice().GetVkDevice(),
        .physical_device = device_resource_.GetVkPhysicalDevice(),
        .window = device_resource_.GetGLFWwindow(),
        .queue_family = queue_family.graphics_family.value(),
        .queue = device_resource_.GetGraphicsQueue(),
        .render_pass = draw_->GetRenderPass(),
        .image_count = device_resource_.GetSwapchain().GetImageCount(),
    };
    gui_.emplace(gui_input);
}

App::~App() {}

void App::CreateScene() {
    spdlog::debug("create scene");

    auto models = std::vector<Model>();

    // create cube
    // [&]() {
    // const auto size = 5.0f;
    // auto vertices = std::vector<Vertex>{
    //     {{-size, -size, size}, {1.0f, 0.0f, 0.0f}},   // 0
    //     {{size, -size, size}, {0.0f, 1.0f, 0.0f}},    // 1
    //     {{size, size, size}, {0.0f, 0.0f, 1.0f}},     // 2
    //     {{-size, size, size}, {1.0f, 1.0f, 1.0f}},    // 3
    //     {{-size, -size, -size}, {1.0f, 0.0f, 1.0f}},  // 4
    //     {{size, -size, -size}, {1.0f, 1.0f, 0.0f}},   // 5
    //     {{size, size, -size}, {0.0f, 1.0f, 1.0f}},    // 6
    //     {{-size, size, -size}, {0.5f, 0.5f, 0.5f}}    // 7
    // };
    // auto indices = std::vector<uint16_t>{0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5, 3, 2, 6, 6, 7, 3,
    //                                      4, 5, 1, 1, 0, 4, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};
    // auto vertex_buffers = std::vector<VertexBuffer>();
    // vertex_buffers.emplace_back(
    //     device_resource_.GetDevice().GetVkDevice(), device_resource_.GetVkPhysicalDevice(),
    //     device_resource_.GetGraphicsQueue(),
    //     device_resource_.GetCommandPool().GetVkCommandPool(), std::move(vertices));
    // auto index_buffers = std::vector<IndexBuffer>();
    // index_buffers.emplace_back(
    //     device_resource_.GetDevice().GetVkDevice(), device_resource_.GetVkPhysicalDevice(),
    //     device_resource_.GetGraphicsQueue(),
    //     device_resource_.GetCommandPool().GetVkCommandPool(), std::move(indices));
    // auto model = Model(std::move(vertex_buffers), std::move(index_buffers));
    // models.emplace_back(std::move(model));
    // }();

    // create sponza
    [&]() {
        spdlog::debug("load sponza");
        const auto gltf_model = LoadTinyGltfModel("assets/sponza/sponza.glb");
        for (const auto& mesh : gltf_model.meshes) {
            for (const auto& primitive : mesh.primitives) {
                auto [indices, vertices, base_color_texture, normal_texture] =
                    LoadGltfObjects(primitive, gltf_model, device_resource_.GetGraphicsQueue(),
                                    device_resource_.GetCommandPool().GetVkCommandPool(),
                                    device_resource_.GetVkPhysicalDevice(),
                                    device_resource_.GetDevice().GetVkDevice(), 0.1f);
                auto vertex_buffers = std::vector<VertexBuffer>();
                vertex_buffers.emplace_back(
                    device_resource_.GetDevice().GetVkDevice(),
                    device_resource_.GetVkPhysicalDevice(), device_resource_.GetGraphicsQueue(),
                    device_resource_.GetCommandPool().GetVkCommandPool(), std::move(vertices));
                auto index_buffers = std::vector<IndexBuffer>();
                index_buffers.emplace_back(
                    device_resource_.GetDevice().GetVkDevice(),
                    device_resource_.GetVkPhysicalDevice(), device_resource_.GetGraphicsQueue(),
                    device_resource_.GetCommandPool().GetVkCommandPool(), std::move(indices));
                auto model = Model(std::move(vertex_buffers), std::move(index_buffers),
                                   std::move(base_color_texture), std::move(normal_texture));
                models.emplace_back(std::move(model));
            }
        }
    }();
    scene_.emplace(std::move(models));
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
    // on init
    gui_->OnStart();
    frame_timer_.Update();
    const auto& device = device_resource_.GetDevice();
    const auto& sync_object = device_resource_.GetSyncObject();
    const auto& swapchain = device_resource_.GetSwapchain();
    const auto& command_buffer = device_resource_.GetCommandBuffer();

    sync_object.WaitAndResetFences();

    uint32_t image_idx;
    {
        auto result = vkAcquireNextImageKHR(device.GetVkDevice(), swapchain.GetVkSwapchain(),
                                            UINT64_MAX, sync_object.GetVkImageAvailableSemaphore(),
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
        auto& keyboard = control_->GetKeyboard();
        auto& mouse = control_->GetMouse();
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
        auto transform = camera_->CreateTransformParams();
        transform_ubo_.UpdateUniformBuffer(transform, image_idx);
    }();

    spdlog::debug("draw frame");
    [&]() {
        vkResetCommandBuffer(command_buffer.GetVkCommandBuffer(), 0);
        const auto begin_info = VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        if (vkBeginCommandBuffer(command_buffer.GetVkCommandBuffer(), &begin_info) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        draw_->RecordCommandBuffer(image_idx, swapchain.GetVkExtent(),
                                   command_buffer.GetVkCommandBuffer());
    }();

    const auto& output_render_target = draw_->GetOutputRenderTarget();

    // Write Swapchain
    spdlog::debug("write swapchain");
    [&]() {
        // Transition Image Layout
        [&]() {
            // output_render_target
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
            };
            vkCmdPipelineBarrier(
                command_buffer.GetVkCommandBuffer(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }();
        [&]() {
            // Swapchain
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
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
            };
            vkCmdPipelineBarrier(
                command_buffer.GetVkCommandBuffer(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
        vkCmdCopyImage(command_buffer.GetVkCommandBuffer(), output_render_target.GetVkImage(),
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain.GetVkImages().at(image_idx),
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

        // Transition Image Layout
        [&]() {
            // output_render_target
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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
            };
            vkCmdPipelineBarrier(command_buffer.GetVkCommandBuffer(),
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                                 nullptr, 1, &barrier);
        }();
        [&]() {
            // Swapchain
            const auto barrier = VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = 0,
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
            };
            vkCmdPipelineBarrier(command_buffer.GetVkCommandBuffer(),
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
                                 nullptr, 1, &barrier);
        }();
    }();

    // ImGui
    [&]() {
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
            .renderPass = draw_->GetRenderPass(),
            .framebuffer = draw_->GetFramebuffer(image_idx),
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = device_resource_.GetSwapchain().GetVkExtent(),
                },
            .clearValueCount = static_cast<uint32_t>(kClearValues.size()),
            .pClearValues = kClearValues.data(),
        };
        vkCmdBeginRenderPass(command_buffer.GetVkCommandBuffer(), &render_pass_info,
                             VK_SUBPASS_CONTENTS_INLINE);
    }();
    ImGui::Begin("Stats");
    ImGui::Text("fps: %f", frame_timer_.GetFPS());
    const auto pos = camera_->GetPosition();
    ImGui::Text("camera pos: (%f, %f, %f)", pos.x, pos.y, pos.z);
    const auto rot = camera_->GetRotation();
    ImGui::Text("camera rot: (%f, %f)", rot.x, rot.y);
    ImGui::Text("Mouse cursor: %s", mouse_right_button_state_ == MouseRightButtonState::kTriggered
                                        ? "enabled"
                                        : "disabled");
    ImGui::End();

    ImGui::Begin("Control");
    ImGui::InputFloat("Mouse sens", &mouse_config_.sens);
    ImGui::End();
    gui_->Render(command_buffer.GetVkCommandBuffer());
    vkCmdEndRenderPass(command_buffer.GetVkCommandBuffer());

    [&]() {
        // Swapchain
        const auto barrier = VkImageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = 0,
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
        };
        vkCmdPipelineBarrier(
            command_buffer.GetVkCommandBuffer(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }();

    spdlog::debug("end command buffer");
    if (vkEndCommandBuffer(command_buffer.GetVkCommandBuffer()) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

    const auto wait_semaphores =
        std::vector<VkSemaphore>({sync_object.GetVkImageAvailableSemaphore()});
    const auto wait_stages =
        std::vector<VkPipelineStageFlags>({VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT});
    const auto signal_semaphores =
        std::vector<VkSemaphore>({sync_object.GetVkRenderFinishedSemaphore()});

    const auto command_buffers = std::to_array({command_buffer.GetVkCommandBuffer()});

    const auto submit_info = VkSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = static_cast<uint32_t>(command_buffers.size()),
        .pCommandBuffers = command_buffers.data(),
        .signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
        .pSignalSemaphores = signal_semaphores.data(),
    };

    if (vkQueueSubmit(device_resource_.GetGraphicsQueue(), 1, &submit_info,
                      sync_object.GetVkInFlightFence()) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    const auto swapchains = std::vector<VkSwapchainKHR>{swapchain.GetVkSwapchain()};
    const auto present_info = VkPresentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size()),
        .pWaitSemaphores = signal_semaphores.data(),
        .swapchainCount = static_cast<uint32_t>(swapchains.size()),
        .pSwapchains = swapchains.data(),
        .pImageIndices = &image_idx,
    };
    vkQueuePresentKHR(device_resource_.GetPresentQueue(), &present_info);
}

}  // namespace vlux
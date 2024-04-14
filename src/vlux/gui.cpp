#include "gui.h"

#include <vulkan/vulkan_core.h>

#include "common/utils.h"

namespace vlux {
static void CheckVkResult(const VkResult err) {
    if (err == 0) return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0) abort();
}

Gui::Gui(const GuiInput& input)
    : window_(input.window),
      instance_(input.instance),
      device_(input.device),
      physical_device_(input.physical_device),
      queue_family_(input.queue_family) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    static_cast<void>(io);
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window_, true);

    // create descriptor pool
    [&]() {
        constexpr auto kPoolSizes = std::to_array<VkDescriptorPoolSize>({
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        });
        const auto pool_info = VkDescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 1,
            .poolSizeCount = static_cast<uint32_t>(kPoolSizes.size()),
            .pPoolSizes = kPoolSizes.data(),
        };
        descriptor_pool_.emplace(device_, pool_info);
    }();

    // create render pass
    [&]() {
        const auto attachment_descs = std::to_array({
            // Color
            VkAttachmentDescription{
                .format = input.swapchain_format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        });

        constexpr auto kColorAttachmentRef = std::to_array<VkAttachmentReference>({
            {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        });
        const auto subpass = VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(kColorAttachmentRef.size()),
            .pColorAttachments = kColorAttachmentRef.data(),
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

        const auto render_pass_info = VkRenderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<uint32_t>(attachment_descs.size()),
            .pAttachments = attachment_descs.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies = dependencies.data(),
        };
        render_pass_.emplace(device_, render_pass_info);
    }();

    // FrameBuffer
    [&]() {
        framebuffer_.reserve(kMaxFramesInFlight);
        const auto [width, height] = GetWindowSize(input.window);
        for (size_t frame_i = 0; frame_i < kMaxFramesInFlight; frame_i++) {
            const auto attachments = std::to_array({input.swapchain_image_views.at(frame_i)});
            const auto framebuffer_info = VkFramebufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass_->GetVkRenderPass(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height),
                .layers = 1,
            };
            framebuffer_.emplace_back(device_, framebuffer_info);
        }
    }();

    auto init_info = ImGui_ImplVulkan_InitInfo{
        .Instance = instance_,
        .PhysicalDevice = physical_device_,
        .Device = device_,
        .QueueFamily = queue_family_,
        .Queue = input.queue,
        .PipelineCache = input.pipeline_cache,
        .DescriptorPool = descriptor_pool_->GetVkDescriptorPool(),
        .Subpass = 0,
        .MinImageCount = kMinImageCount,
        .ImageCount = 2,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = nullptr,
        .CheckVkResultFn = CheckVkResult,
    };
    ImGui_ImplVulkan_Init(&init_info, render_pass_->GetVkRenderPass());
    ImGui_ImplVulkan_SetMinImageCount(kMinImageCount);
}

Gui::~Gui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Gui::OnStart() const {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::Render(const VkCommandBuffer command_buffer, const uint32_t width, const uint32_t height,
                 const int image_idx) const {
    [&]() {
        constexpr auto kClearValues = std::to_array<VkClearValue>({
            // Color
            {
                .color =
                    {
                        {0.0f, 0.0f, 0.0f, 1.0f},
                    },
            },
        });
        const auto render_pass_info = VkRenderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass_->GetVkRenderPass(),
            .framebuffer = framebuffer_.at(image_idx).GetVkFrameBuffer(),
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = {.width = width, .height = height},
                },
            .clearValueCount = static_cast<uint32_t>(kClearValues.size()),
            .pClearValues = kClearValues.data(),
        };
        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    }();
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    vkCmdEndRenderPass(command_buffer);
}
}  // namespace vlux
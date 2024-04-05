#include "gui.h"

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
    auto pool_sizes = std::to_array<VkDescriptorPoolSize>({
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    });
    auto pool_info = VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    CheckVkResult(vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_));

    auto init_info = ImGui_ImplVulkan_InitInfo{
        .Instance = instance_,
        .PhysicalDevice = physical_device_,
        .Device = device_,
        .QueueFamily = queue_family_,
        .Queue = input.queue,
        .PipelineCache = input.pipeline_cache,
        .DescriptorPool = descriptor_pool_,
        .Subpass = 0,
        .MinImageCount = kMinImageCount,
        .ImageCount = 2,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .Allocator = nullptr,
        .CheckVkResultFn = CheckVkResult,
    };
    ImGui_ImplVulkan_Init(&init_info, input.render_pass);
    ImGui_ImplVulkan_SetMinImageCount(kMinImageCount);
}

Gui::~Gui() {
    vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
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

void Gui::Render(const VkCommandBuffer command_buffer) const {
    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
}
}  // namespace vlux
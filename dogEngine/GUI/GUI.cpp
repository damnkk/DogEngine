#include "GUI.h"
#include "Component/GameViewer.h"
#include "Renderer.h"

namespace dg {
float GUI::deltaTime = 0.0f;

void GUI::keycallback() {
  float sensitivity = 0.07;

  if (m_io->KeysDown[ImGuiKey_LeftShift]) {
    sensitivity = 0.7;
  } else {
    sensitivity = 0.07;
  }

  if (glfwGetKey(m_renderer->getContext()->m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, true);
  }
  std::shared_ptr<Camera> camera = m_renderer->getCamera();

  if (m_io->KeysDown[ImGuiKey_W]) {
    camera->getPosition() += sensitivity * camera->getDirectVector();
  }
  if (m_io->KeysDown[ImGuiKey_S]) {
    camera->getPosition() -= sensitivity * camera->getDirectVector();
  }
  if (m_io->KeysDown[ImGuiKey_A]) {
    camera->getPosition() -=
        sensitivity * glm::normalize(glm::cross(camera->getDirectVector(), camera->getUpVector()));
  }
  if (m_io->KeysDown[ImGuiKey_D]) {
    camera->getPosition() +=
        sensitivity * glm::normalize(glm::cross(camera->getDirectVector(), camera->getUpVector()));
  }
  if (m_io->KeysDown[ImGuiKey_Space]) {
    camera->getPosition() += sensitivity * glm::vec3(0, 1, 0);
  }
  if (m_io->KeysDown[ImGuiKey_LeftCtrl]) {
    camera->getPosition() -= sensitivity * glm::vec3(0, 1, 0);
  }
}

void guiVulkanInit(GUI &ui) {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = ui.getRenderer()->getContext()->m_graphicQueueIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  auto res = vkCreateCommandPool(ui.getRenderer()->getContext()->m_logicDevice, &poolInfo, nullptr,
                                 &ui.getCommandPool());
  DGASSERT(res == VK_SUCCESS);
  for (int i = 0; i < 3; ++i) {
    VkCommandBufferAllocateInfo cbAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbAllocInfo.commandBufferCount = 1;
    cbAllocInfo.commandPool = ui.getCommandPool();
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    res = vkAllocateCommandBuffers(ui.getRenderer()->getContext()->m_logicDevice, &cbAllocInfo,
                                   &ui.getCommandBuffer()[i].m_commandBuffer);
    DGASSERT(res == VK_SUCCESS);
    ui.getCommandBuffer()[i].m_context = ui.getRenderer()->getContext().get();
    ui.getCommandBuffer()[i].m_handle = i;
    ui.getCommandBuffer()[i].reset();
  }
  VkSemaphoreCreateInfo smInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  vkCreateSemaphore(ui.getRenderer()->getContext()->m_logicDevice, &smInfo, nullptr, &ui.getSemaphore());
}

void GUI::init(Renderer *render) {
  m_renderer = render;
  guiVulkanInit(GUI::getInstance());
  std::shared_ptr<DeviceContext> context = render->getContext();
  m_window = context->m_window;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  this->m_io = &ImGui::GetIO();
  setConfigFlag(ImGuiConfigFlags_DockingEnable);
  setConfigFlag(ImGuiConfigFlags_ViewportsEnable);
  setBackEndFlag(ImGuiBackendFlags_PlatformHasViewports);
  ImGui::StyleColorsDark();//dark theme
                           //ImGui::StyleColorsLight();
  ImGui_ImplGlfw_InitForVulkan(m_window, true);
  // Prepare the init struct for ImGui_ImplVulkan_Init in LoadFontsToGPU
  init_info.Instance = context->m_instance;
  init_info.PhysicalDevice = context->m_physicalDevice;
  init_info.Device = context->m_logicDevice;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.Subpass = 0;
  init_info.Allocator = nullptr;
  init_info.QueueFamily = context->m_graphicQueueIndex;
  init_info.Queue = context->m_graphicsQueue;
  init_info.DescriptorPool = context->m_descriptorPool;
  init_info.MinImageCount = dg::k_max_swapchain_images;
  init_info.ImageCount = dg::k_max_swapchain_images;
  init_info.CheckVkResultFn = nullptr;
  ImGui_ImplVulkan_Init(&init_info, context->accessRenderPass(context->m_swapChainPass)->m_renderPass);

  m_SettingsData = new SettingsData();
  m_LightSpeed = new float(1.0f);
  m_LightCol = new glm::vec3(1.0f);
  m_LightIdx = new int(0);

  addViewer(std::make_shared<MaterialViewer>("Material"));
  addViewer(std::make_shared<GameViewer>("GameViewer"));
}

// void GUI::LoadFontsToGPU(){
//     VkCommandPool command_pool = m_Data.command_pool;
//     VkCommandBuffer command_buffer = m_Data.command_buffers[0];
//     vkResetCommandPool(m_Data.main_device.logicalDevice, command_pool, 0);
//     VkCommandBufferBeginInfo begin_info = {};
//     begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//     begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//     vkBeginCommandBuffer(command_buffer,&begin_info);
//
//     ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
//
//     VkSubmitInfo end_info = {};
//     end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//     end_info.commandBufferCount = 1;
//     end_info.pCommandBuffers = &command_buffer;
//     vkEndCommandBuffer(command_buffer);
//     vkQueueSubmit(m_Data.graphic_queue, 1, &end_info, VK_NULL_HANDLE);
//     vkDeviceWaitIdle(m_Data.main_device.logicalDevice);
//     ImGui_ImplVulkan_DestroyFontUploadObjects();
// }

void GUI::newGUIFrame() {
  vkResetCommandBuffer(getCurrentFramCb()->m_commandBuffer, 0);
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void GUI::OnGUI() {
  ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
  ImGui::BeginMainMenuBar();

  ImGui::EndMainMenuBar();
  for (int i = 0; i < m_compontents.size(); ++i) {
    auto currComponent = m_compontents[i];
    currComponent->OnGUI();
  }
  //你可以理解这个函数可以初始化ImDrawData
  ImGui::Render();
}

void GUI::eventListen() {
  //keycallback();
}

void GUI::endGUIFrame() {
  if (m_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    if (m_window) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(m_window);
    }
  }
}

ImDrawData *GUI::GetDrawData() {
  return ImGui::GetDrawData();
}

void GUI::KeysControl(bool *keys) {
  if (keys[GLFW_KEY_1]) {
    m_SettingsData->render_target = 0;
  }

  if (keys[GLFW_KEY_2]) {
    m_SettingsData->render_target = 1;
  }

  if (keys[GLFW_KEY_3]) {
    m_SettingsData->render_target = 2;
  }

  if (keys[GLFW_KEY_4]) {
    m_SettingsData->render_target = 3;
  }
}

void GUI::Destroy() {
  vkDestroyCommandPool(m_renderer->getContext()->m_logicDevice, m_commandPool, nullptr);
  vkDestroySemaphore(m_renderer->getContext()->m_logicDevice, m_uiFinishSemaphore, nullptr);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

}//namespace dg
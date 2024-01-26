#define IMGUI_DEFINE_MATH_OPERATORS
#include "GameViewer.h"
#include "Renderer.h"
namespace dg {
GameViewer::GameViewer() {
}

GameViewer::GameViewer(std::string name) {
  m_name = name;
}

float lastX = 300, lastY = 300;
bool click = true;
ImVec2 OldWindowSize;
void GameViewer::OnGUI() {
  ImGui::Begin("GameView");
  auto desc = m_renderer->getContext()->accessDescriptorSet(m_renderer->getContext()
                                                                ->m_gameViewTextureDescs[0]);
  Texture *text = m_renderer->getContext()->accessTexture(m_renderer->getContext()
                                                              ->m_gameViewFrameTextures[0]);
  bool windowHovered = ImGui::IsWindowHovered();
  if (windowHovered) {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      click = false;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) click = true;
    ImVec2 cursorPos = ImGui::GetMousePos();
    if (click) {

      lastX = cursorPos.x;
      lastY = cursorPos.y;
    }

    float sensitive = 0.35;
    float xDif = cursorPos.x - lastX;
    float yDif = cursorPos.y - lastY;
    DeviceContext::m_camera->pitch += sensitive * yDif;
    DeviceContext::m_camera->pitch = glm::clamp(DeviceContext::m_camera->pitch, -89.0f, 89.0f);
    DeviceContext::m_camera->yaw += sensitive * xDif;
    lastX = cursorPos.x;
    lastY = cursorPos.y;
  }

  ImVec2 currWindowSize = ImGui::GetWindowSize();
  //  if (currWindowSize != OldWindowSize) {
  //    //m_renderer->getContext();
  //    //m_renderer->getContext()->resizeGameView();
  //    m_renderer->getContext()->m_resized = true;
  //    m_renderer->getContext()->m_gameViewWidth = currWindowSize.x;
  //    m_renderer->getContext()->m_gameViewHeight = currWindowSize.y;
  //    std::cout << "DeviceContext resize the framebuffer" << std::endl;
  //    OldWindowSize = currWindowSize;
  //  }

  ImGui::Image((ImTextureID) desc->m_vkdescriptorSet, ImVec2(text->m_extent.width, text->m_extent.height));
  ImGui::End();
}
}// namespace dg
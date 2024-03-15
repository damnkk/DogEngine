#include "GameViewer.h"
#include "Renderer.h"
namespace dg {
GameViewer::GameViewer() {}

GameViewer::GameViewer(std::string name) { m_name = name; }

void GameViewer::OnGUI() {
  std::shared_ptr<Camera> camera = m_renderer->getCamera();
  ImGui::Begin("GameView");
  auto desc = m_renderer->getContext()->accessDescriptorSet(
      m_renderer->getContext()->m_gameViewTextureDescs[0]);
  Texture* text =
      m_renderer->getContext()->accessTexture(m_renderer->getContext()->m_gameViewFrameTextures[0]);
  bool windowHovered = ImGui::IsWindowHovered();
  if (ImGui::IsWindowFocused()) {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) { camera->rightButtonPressState() = true; }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) camera->rightButtonPressState() = false;
    ImVec2 cursorPos = ImGui::GetMousePos();
    camera->updateDirection(m_renderer->deltaTime, glm::vec2(cursorPos.x, cursorPos.y),m_renderer->getContext());
    camera->updatePosition(m_renderer->deltaTime,m_renderer->getContext());

  }

  const ImVec2 currWindowSize = ImGui::GetWindowSize();
  if (currWindowSize != m_oldWindowSize) {
    //m_oldWindowSize = ImVec2(currWindowSize.x - 15, currWindowSize.y - 35);
    m_oldWindowSize = currWindowSize;
    m_renderer->getContext()->gameViewResize = true;
    m_renderer->getContext()->m_gameViewWidth = glm::max((u32) (currWindowSize.x - 15), (u32) 1);
    m_renderer->getContext()->m_gameViewHeight = glm::max((u32) (currWindowSize.y - 35), (u32) 1);
    //m_renderer->getContext()->resizeGameViewPass({(u32) currWindowSize.x, (u32) currWindowSize.y});
    camera->getAspect() = (float) (currWindowSize.x - 15) / (float) (currWindowSize.y - 35);
  }

  ImGui::Image((ImTextureID) desc->m_vkdescriptorSet,
               ImVec2(currWindowSize.x - 15, currWindowSize.y - 35));
  ImGui::End();
}
}// namespace dg
#include "GameViewer.h"
#include "Renderer.h"
namespace dg {
GameViewer::GameViewer() {
}

GameViewer::GameViewer(std::string name) {
  m_name = name;
}

ImVec2 OldWindowSize;
void GameViewer::OnGUI() {
  std::shared_ptr<Camera> camera = m_renderer->getCamera();
  ImGui::Begin("GameView");
  auto desc = m_renderer->getContext()->accessDescriptorSet(m_renderer->getContext()
                                                                ->m_gameViewTextureDescs[0]);
  Texture *text = m_renderer->getContext()->accessTexture(m_renderer->getContext()
                                                              ->m_gameViewFrameTextures[0]);
  bool windowHovered = ImGui::IsWindowHovered();
  if (ImGui::IsWindowFocused()) {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      camera->rightButtonPressState() = true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Right)) camera->rightButtonPressState() = false;
    ImVec2 cursorPos = ImGui::GetMousePos();
    camera->updateDirection(m_renderer->deltaTime, glm::vec2(cursorPos.x, cursorPos.y));
    camera->updatePosition(m_renderer->deltaTime);
  }

  const ImVec2 currWindowSize = ImGui::GetWindowSize();
  if (currWindowSize != OldWindowSize) {
    OldWindowSize = currWindowSize;
    m_renderer->getContext()->gameViewResize = true;
    m_renderer->getContext()->m_gameViewWidth = (u32) currWindowSize.x;
    m_renderer->getContext()->m_gameViewHeight = (u32) currWindowSize.y;
    //m_renderer->getContext()->resizeGameViewPass({(u32) currWindowSize.x, (u32) currWindowSize.y});
    camera->getAspect() = (float) currWindowSize.x / (float) currWindowSize.y;
  }

  ImGui::Image((ImTextureID) desc->m_vkdescriptorSet, currWindowSize);
  ImGui::End();
}
}// namespace dg
#include "MainBar.h"
#include "Renderer.h"

namespace dg {

MainBar::MainBar() {}

MainBar::MainBar(std::string name) { m_name = name; }

void MainBar::OnGUI() {
  //ImGui::Begin(m_name.c_str(), nullptr, ImGuiWindowFlags_MenuBar);
  ImGui::BeginMainMenuBar();
  if (ImGui::BeginMenu("File")) { ImGui::EndMenu(); }
  if (ImGui::BeginMenu("Setting")) {
    if (ImGui::BeginMenu("RenderMode")) {
      if (ImGui::MenuItem("Rasterize")) { m_renderer->m_renderMode = eRasterize; }
      if (ImGui::MenuItem("RayTracing")) { m_renderer->m_renderMode = eRayTracing; };
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
  //ImGui::End();
}
}// namespace dg
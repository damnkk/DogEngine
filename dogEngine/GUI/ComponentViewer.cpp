#include "ComponentViewer.h"
namespace dg {
namespace dgUI {
float itemTab = 150;  //set the item position per line
float itemWidth = 200;// set the item width, like:combo|slideFloat etc.
}// namespace dgUI
ComponentViewer::ComponentViewer() {}

ComponentViewer::ComponentViewer(std::string componentName) : m_name(componentName) {}

ComponentViewer::~ComponentViewer() {}

void ComponentViewer::HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::BeginItemTooltip()) {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}
}// namespace dg
#include "ComponentViewer.h"
namespace dg{
    ComponentViewer::ComponentViewer(){

    }

    ComponentViewer::ComponentViewer(std::string componentName):m_name(componentName){

    }

    ComponentViewer::~ComponentViewer(){
        
    }

    void ComponentViewer::HelpMarker(const char* desc)
    {
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}
#ifndef COMPONENTVIEWER_H
#define COMPONENTVIEWER_H
#include "dgpch.h"
#include "dgLog.hpp"
// DEAR IMGUI
#include "imguiBase/imgui.h"
#include "imguiBase/imgui_impl_glfw.h"
#include "imguiBase/imgui_impl_vulkan.h"
#include "glm/gtc/type_ptr.hpp"
namespace dg{
    namespace dgUI{
        static float itemTab=150;//set the item position per line
        static float itemWidth = 200;// set the item width, like:combo|slideFloat etc.
    }

struct Renderer;
class ComponentViewer{
public:
    ComponentViewer();
    ComponentViewer(std::string conponentName);
    virtual ~ComponentViewer();
    void HelpMarker(const char* desc);
    virtual void OnGUI(){
        DG_ERROR("You are trying to innovate an unoverloaded function")
        exit(-1);};
    std::string m_name;
    Renderer* m_renderer = nullptr;
};
}
#endif //COMPONENTVIEWER_H
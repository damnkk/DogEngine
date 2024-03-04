#ifndef COMPONENTVIEWER_H
#define COMPONENTVIEWER_H
//#include "GUI.h"
#include "dgLog.hpp"
#include "dgpch.h"

// DEAR IMGUI
#include "glm/gtc/type_ptr.hpp"
#include "imguiBase/imgui.h"
#include "imguiBase/imgui_impl_glfw.h"
#include "imguiBase/imgui_impl_vulkan.h"

namespace dg {
namespace dgUI {
extern float itemTab;  //set the item position per line
extern float itemWidth;// set the item width, like:combo|slideFloat etc.
}// namespace dgUI

struct Renderer;
class ComponentViewer {
 public:
  ComponentViewer();
  ComponentViewer(std::string conponentName);
  virtual ~ComponentViewer();
  void         HelpMarker(const char* desc);
  virtual void OnGUI() {
    DG_ERROR("You are trying to innovate an unoverloaded function")
    exit(-1);
  };
  std::string m_name;
  Renderer*   m_renderer = nullptr;
};
}// namespace dg
#endif//COMPONENTVIEWER_H
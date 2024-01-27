#ifndef GUI_H
#define GUI_H
#include "../dgpch.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "CommandBuffer.h"
#include "Renderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
// DEAR IMGUI
#include "imguiBase/imgui.h"
#include "imguiBase/imgui_impl_glfw.h"
#include "imguiBase/imgui_impl_vulkan.h"

#include "Component/MaterialViewer.h"
#include "ComponentViewer.h"
namespace dg {

struct SettingsData {
  int render_target;
};

struct DeviceContext;
struct CommandBuffer;
class GUI {
 public:
  static GUI &getInstance() {
    static GUI m_instance;
    return m_instance;
  }

  void init(Renderer *render);
  void Destroy();

  void LoadFontsToGPU();
  void newGUIFrame();
  void OnGUI();
  void endGUIFrame();

  std::vector<CommandBuffer> &getCommandBuffer() { return m_commandBuffers; }
  CommandBuffer *getCurrentFramCb() { return &m_commandBuffers[m_renderer->getContext()
                                                                   ->m_currentFrame]; }
  Renderer *getRenderer() { return m_renderer; }
  VkCommandPool &getCommandPool() { return m_commandPool; }
  VkSemaphore &getSemaphore() { return m_uiFinishSemaphore; }

  template<typename T>
  void addViewer(std::shared_ptr<T> pViewer);

  void setConfigFlag(ImGuiConfigFlags_ flag) { this->m_io->ConfigFlags |= flag; }
  void setBackEndFlag(ImGuiBackendFlags_ flag) { this->m_io->BackendFlags |= flag; }
  ImDrawData *GetDrawData();
  void eventListen();
  void KeysControl(bool *keys);
  ImGuiIO *getIO() { return m_io; }

  static float deltaTime;

 private:
  void keycallback();
  GUI(){};
  GUI(const GUI &){};
  ~GUI(){};
  GUI &operator=(const GUI &);

  Renderer *m_renderer;
  VkCommandPool m_commandPool;
  std::vector<CommandBuffer> m_commandBuffers = std::vector<CommandBuffer>(3);
  VkSemaphore m_uiFinishSemaphore;

  //Imgui about
  ImGuiIO *m_io = nullptr;
  GLFWwindow *m_window = nullptr;
  ImGui_ImplVulkan_InitInfo init_info = {};
  std::vector<std::shared_ptr<ComponentViewer>> m_compontents;

  SettingsData *m_SettingsData = nullptr;
  float *m_LightSpeed = nullptr;
  int *m_LightIdx;
  glm::vec3 *m_LightCol;
  float m_Col[3];
};

template<typename T>
void GUI::addViewer(std::shared_ptr<T> pViewer) {
  m_compontents.push_back(pViewer);
  pViewer->m_renderer = this->m_renderer;
}

}//namespace dg

#endif//GUI_H
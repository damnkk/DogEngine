#pragma once
#include"../dgpch.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

// DEAR IMGUI
#include "imguiBase/imgui.h"
#include "imguiBase/imgui_impl_glfw.h"
#include "imguiBase/imgui_impl_vulkan.h"

namespace dg{

struct SettingsData {
	int render_target;
};

struct DeviceContext;
struct CommandBuffer;
class GUI{
public:
    static GUI& getInstance(){
        static GUI m_instance;
        return m_instance;
    }

    void init(std::shared_ptr<DeviceContext> context);
    void LoadFontsToGPU();
    void newGUIFrame();
    void OnGUI();
    void endGUIFrame();
    void setConfigFlag(ImGuiConfigFlags_ flag){this->m_io->ConfigFlags |=flag;}
    void setBackEndFlag(ImGuiBackendFlags_ flag){this->m_io->BackendFlags|=flag;}
    ImDrawData* GetDrawData();
    void eventListen();
    void KeysControl(bool* keys);
    void Destroy();
    ImGuiIO* getIO(){return m_io;}
private:
    void keycallback();
    GUI(){};
    GUI(const GUI& ){};
    ~GUI(){};
    GUI& operator=(const GUI &);

    VkRenderPass  m_uiRenderPass;
    ImGuiIO*     m_io = nullptr;
    GLFWwindow* m_window = nullptr;
    SettingsData* m_SettingsData = nullptr;
    float* m_LightSpeed = nullptr;
    int* m_LightIdx;
    glm::vec3* m_LightCol;
    float m_Col[3];
    ImGui_ImplVulkan_InitInfo init_info = {};
};

} //namespace dg
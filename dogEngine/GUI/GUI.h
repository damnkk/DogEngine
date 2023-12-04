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
    void Render();
    ImDrawData* GetDrawData();
    void KeysControl(bool* keys);
    void Destroy();
private:
    GUI(){};
    GUI(const GUI& ){};
    ~GUI(){};
    GUI& operator=(const GUI &);
    
    GLFWwindow* m_Window = nullptr;
    SettingsData* m_SettingsData = nullptr;
    float* m_LightSpeed = nullptr;
    int* m_LightIdx;
    glm::vec3* m_LightCol;
    float m_Col[3];
    ImGui_ImplVulkan_InitInfo init_info = {};
};

} //namespace dg
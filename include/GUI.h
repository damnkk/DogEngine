#pragma once
#include"common.h"
#include "DataStructures.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// DEAR IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace dg{

struct SettingsData {
	int render_target;
};
class GUI{
public:
    static GUI* getInstance(){
        if(!s_Instance){
            s_Instance = new GUI();
        }
        return s_Instance;
    }
    void SetRenderData(VulkanRenderData data, GLFWwindow* window, SettingsData* ubo_settings, 
        float* lights_speed, int* light_idx, glm::vec3* light_col)
    {
        m_Data              = data;
        m_Window            = window;
        m_SettingsData      = ubo_settings;
        m_LightSpeed        = lights_speed;
        m_LightIdx          = light_idx;
        m_LightCol          = light_col;
    }
    void Init();
    void LoadFontsToGPU();
    void Render();
    ImDrawData* GetDrawData();
    void KeysControl(bool* keys);
    void Destroy();
private:
    GUI(){};
    static GUI* s_Instance;
    GLFWwindow* m_Window = nullptr;
    SettingsData* m_SettingsData = nullptr;
    float* m_LightSpeed = nullptr;
    int* m_LightIdx;
    glm::vec3* m_LightCol;
    float m_Col[3];
    VulkanRenderData m_Data;
    ImGui_ImplVulkan_InitInfo init_info = {};
};

} //namespace dg
#include "GUI.h"
#include "Renderer.h"
namespace dg {

    void GUI::keycallback() {
        float sensitivity = 0.07;

        if (m_io->KeysDown[ImGuiKey_LeftShift]) {
            sensitivity = 0.7;
        } else {
            sensitivity = 0.07;
        }

        if (m_io->KeysDown[ImGuiKey_Escape]) {
            glfwSetWindowShouldClose(m_window, true);
        }
        if (m_io->KeysDown[ImGuiKey_W]) {
            DeviceContext::m_camera->pos += sensitivity * DeviceContext::m_camera->direction;
        }
        if (m_io->KeysDown[ImGuiKey_S]) {
            DeviceContext::m_camera->pos -= sensitivity * DeviceContext::m_camera->direction;
        }
        if (m_io->KeysDown[ImGuiKey_A]) {
            DeviceContext::m_camera->pos -=
                    sensitivity * glm::normalize(glm::cross(DeviceContext::m_camera->direction, DeviceContext::m_camera->up));
        }
        if (m_io->KeysDown[ImGuiKey_D]) {
            DeviceContext::m_camera->pos +=
                    sensitivity * glm::normalize(glm::cross(DeviceContext::m_camera->direction, DeviceContext::m_camera->up));
        }
        if (m_io->KeysDown[ImGuiKey_Space]) {
            DeviceContext::m_camera->pos += sensitivity * glm::vec3(0, 1, 0);
        }
        if (m_io->KeysDown[ImGuiKey_LeftCtrl]) {
            DeviceContext::m_camera->pos -= sensitivity * glm::vec3(0, 1, 0);
        }
    }

    void GUI::init(Renderer *render) {
        m_renderer = render;
        std::shared_ptr<DeviceContext> context = render->getContext();
        m_window = context->m_window;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        this->m_io = &ImGui::GetIO();
        setConfigFlag(ImGuiConfigFlags_DockingEnable);
        setConfigFlag(ImGuiConfigFlags_ViewportsEnable);
        setBackEndFlag(ImGuiBackendFlags_PlatformHasViewports);
        ImGui::StyleColorsDark();//dark theme
                                 //ImGui::StyleColorsLight();
        ImGui_ImplGlfw_InitForVulkan(m_window, true);
        // Prepare the init struct for ImGui_ImplVulkan_Init in LoadFontsToGPU
        init_info.Instance = context->m_instance;
        init_info.PhysicalDevice = context->m_physicalDevice;
        init_info.Device = context->m_logicDevice;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.Subpass = 0;
        init_info.Allocator = nullptr;
        init_info.QueueFamily = context->m_graphicQueueIndex;
        init_info.Queue = context->m_graphicsQueue;
        init_info.DescriptorPool = context->m_descriptorPool;
        init_info.MinImageCount = dg::k_max_swapchain_images;
        init_info.ImageCount = dg::k_max_swapchain_images;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info, context->accessRenderPass(context->m_swapChainPass)->m_renderPass);

        m_SettingsData = new SettingsData();
        m_LightSpeed = new float(1.0f);
        m_LightCol = new glm::vec3(1.0f);
        m_LightIdx = new int(0);

        addViewer(std::make_shared<MaterialViewer>("Material"));
    }

    // void GUI::LoadFontsToGPU(){
    //     VkCommandPool command_pool = m_Data.command_pool;
    //     VkCommandBuffer command_buffer = m_Data.command_buffers[0];
    //     vkResetCommandPool(m_Data.main_device.logicalDevice, command_pool, 0);
    //     VkCommandBufferBeginInfo begin_info = {};
    //     begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //     begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    //     vkBeginCommandBuffer(command_buffer,&begin_info);
    //
    //     ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    //
    //     VkSubmitInfo end_info = {};
    //     end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //     end_info.commandBufferCount = 1;
    //     end_info.pCommandBuffers = &command_buffer;
    //     vkEndCommandBuffer(command_buffer);
    //     vkQueueSubmit(m_Data.graphic_queue, 1, &end_info, VK_NULL_HANDLE);
    //     vkDeviceWaitIdle(m_Data.main_device.logicalDevice);
    //     ImGui_ImplVulkan_DestroyFontUploadObjects();
    // }

    void GUI::newGUIFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void GUI::OnGUI() {
        //ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        ImGui::BeginMainMenuBar();

        ImGui::EndMainMenuBar();
        for (int i = 0; i < m_compontents.size(); ++i) {
            auto currComponent = m_compontents[i];
            currComponent->OnGUI();
        }
    }

    void GUI::eventListen() {
        keycallback();
    }

    void GUI::endGUIFrame() {
        //你可以理解这个函数可以初始化ImDrawData
        ImGui::Render();
        if (m_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            if (m_window) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(m_window);
            }
        }
    }

    ImDrawData *GUI::GetDrawData() {
        return ImGui::GetDrawData();
    }

    void GUI::KeysControl(bool *keys) {
        if (keys[GLFW_KEY_1]) {
            m_SettingsData->render_target = 0;
        }

        if (keys[GLFW_KEY_2]) {
            m_SettingsData->render_target = 1;
        }

        if (keys[GLFW_KEY_3]) {
            m_SettingsData->render_target = 2;
        }

        if (keys[GLFW_KEY_4]) {
            m_SettingsData->render_target = 3;
        }
    }

    void GUI::Destroy() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

}//namespace dg
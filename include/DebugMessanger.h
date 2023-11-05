# pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<iostream>
#include"Common.h"

namespace dg{

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

// so the error message

class DebugMessanger{
public:
    static DebugMessanger* GetInstance();   //这个单例是静态的,外部可以直接调用,此外类内其他
    //函数的调用都得经过这个东西
    void SetupDebugMessenger(VkInstance& instance);  //initialize
    void Clear(); 

private:
    DebugMessanger() = default;
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserdata
    );

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );

    void DestroyDebugUtilsMessengerEXT(
        VkInstance& instance,
        VkDebugUtilsMessengerEXT &debugMessenger,
        const VkAllocationCallbacks* pAllocator
    );

private:
    static DebugMessanger* s_Instance;
    VkInstance m_VulkanInstance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
};

} //namespace dg
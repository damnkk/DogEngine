#ifndef DEBUGMESSANGER_H
#define DEBUGMESSANGER_H
#define GLFW_INCLUDE_VULKAN
#include "dgLog.hpp"
#include "dgpch.h"
#include <GLFW/glfw3.h>
#include <iostream>

namespace dg {
struct DeviceContext;

class DebugMessanger {
 public:
  static DebugMessanger* GetInstance();//这个单例是静态的,外部可以直接调用,此外类内其他
  //函数的调用都得经过这个东西
  void SetupDebugMessenger(DeviceContext* context);//initialize
  void Clear();

  void setObjectName(uint64_t object, const std::string name, VkObjectType T);

 private:
  DebugMessanger() = default;
  static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT             messageType,
      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void*                                       pUserdata);

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance                                instance,
      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
      const VkAllocationCallbacks*              pAllocator,
      VkDebugUtilsMessengerEXT*                 pDebugMessenger);

  void DestroyDebugUtilsMessengerEXT(
      VkInstance&                  instance,
      VkDebugUtilsMessengerEXT&    debugMessenger,
      const VkAllocationCallbacks* pAllocator);

 private:
  static DebugMessanger*   s_Instance;
  DeviceContext*           m_context;
  VkDebugUtilsMessengerEXT m_DebugMessenger;
};

}//namespace dg

#endif//DEBUGMESSANGER_H
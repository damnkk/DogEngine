#include "DebugMessanger.h"
#include "DeviceContext.h"
namespace dg {

DebugMessanger* DebugMessanger::s_Instance = nullptr;

void DebugMessanger::Clear() {
  DestroyDebugUtilsMessengerEXT(m_context->m_instance, m_DebugMessenger, nullptr);
}

DebugMessanger* DebugMessanger::GetInstance() {
  if (s_Instance == nullptr) { s_Instance = new DebugMessanger(); }
  return s_Instance;
}

void DebugMessanger::SetupDebugMessenger(DeviceContext* context) {
  m_context = context;
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.flags = 0;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = DebugCallback;
  createInfo.pUserData = nullptr;

  if (CreateDebugUtilsMessengerEXT(m_context->m_instance, &createInfo, nullptr, &m_DebugMessenger)
      != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messanger!");
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessanger::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserdata) {
  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: std::cerr << "[Verbose]"; break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: std::cerr << "[Info]"; break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: std::cerr << "[Warning]"; break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: std::cerr << "[Error]"; break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: break;
  }

  switch (messageType) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: std::cerr << "[Gerneal]"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: std::cerr << "[Validation]"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: std::cerr << "[Performance]"; break;
  }

  std::cerr << "\nCount: " << pCallbackData->objectCount << std::endl;
  std::cerr << "MessageID name: " << pCallbackData->pMessageIdName << std::endl;
  std::cerr << "Type: " << pCallbackData->sType;
  std::cerr << "Message: " << pCallbackData->pMessage << std::endl;
  // if (pCallbackData->objectCount > 0) {
  //   for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
  //     std::cerr << "Object [" << i
  //               << "] :" << pCallbackData->pObjects[i].pObjectName << std::endl;
  //   }
  // }
  return VK_FALSE;
}

VkResult DebugMessanger::CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) { return func(instance, pCreateInfo, pAllocator, &m_DebugMessenger); }

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// OK
void DebugMessanger::DestroyDebugUtilsMessengerEXT(VkInstance&                  instance,
                                                   VkDebugUtilsMessengerEXT&    DebugMessenger,
                                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) { func(instance, m_DebugMessenger, pAllocator); }
}

void DebugMessanger::setObjectName(uint64_t object, const std::string name, VkObjectType T) {
  VkDebugUtilsObjectNameInfoEXT nameinfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
  nameinfo.objectType = T;
  nameinfo.objectHandle = object;
  nameinfo.pObjectName = name.c_str();
  vkSetDebugUtilsObjectNameEXT(m_context->m_logicDevice, &nameinfo);
}

}//namespace dg
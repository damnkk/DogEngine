#pragma once

#include "common.h"
#include "Utilities.h"
namespace dg{
struct SwapChainImage{
    VkImage image;
    VkImageView imageView;
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain{
public:
    SwapChain();// swapchain is not exclusive, we can create many of it.
    SwapChain(MainDevice* main_device, VkSurfaceKHR* surface,
    GLFWwindow* window, QueueFamilyIndices& QueueFamilyIndices);

    /* Generic*/
    void CreateSwapChain();
    void ReCreateSwapChain();
    void CreateFrameBuffers(const VkImageView& depth_buffer);
    void CleanUpSwapChain();
    void DestroyFrameBuffers();
    void DestroySwapChainImageViews();
    void DestroySwapChain();

    // Getters
    VkSwapchainKHR* GetSwapChainData();
    VkSwapchainKHR& GetSwapChain();
    uint32_t GetExtentWidth() const;
    uint32_t GetExtentHeight() const;
    VkExtent2D& GetExtent();
    VkFormat& GetSwapChainImageFormat();
    SwapChainSupportDetails GetSwapChainDetails(VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface);

    // Setters
    void SetRenderPass(VkRenderPass* renderPass);
    void SetRecreationStatus(bool const status);

    // Vectors operations
    std::vector<VkFramebuffer>& GetFrameBuffers();
    size_t SwapChainImagesSize() const;
    size_t FrameBufferSize() const;
    SwapChainImage* GetImage(uint32_t index);
    VkImageView& GetSwapChainImageView(uint32_t index);
    VkFramebuffer& GetFrameBuffer(uint32_t index);
    void PushImage(SwapChainImage swapChainImage);
    void PushFrameBuffer(VkFramebuffer frameBuffer);
    void ResizeFrameBuffers();

private:
    MainDevice* m_MainDevice;
    VkSurfaceKHR* m_VulkanSurface;
    GLFWwindow* m_Window;
    VkRenderPass *m_RenderPass;

    QueueFamilyIndices m_QueueFamilyIndices;

private:
    VkSwapchainKHR m_SwapChain;
    std::vector<SwapChainImage> m_SwapChainImages;
    std::vector<VkFramebuffer> m_SwapChainFrameBuffers;

    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;

    bool m_IsRecreateing = false;

private:
    VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
};

}  //namespace dg
#pragma once
#include "common.h"
#include "Utilities.h"
#include "SwapChainHandler.h"


namespace dg{
// RenderPass is created as a component of graphics pipline, when recording command buffer
// user need to set the right renderPass
class RenderPassHandler{
public:
    RenderPassHandler();
    RenderPassHandler(MainDevice* mainDevice, SwapChain* swapChainHandler);

    VkRenderPass* GetOffScreenRenderPassReference(){return &m_OffScreenRenderPass;}
    VkRenderPass& GetOffScreenRenderPass(){return m_OffScreenRenderPass;}
    VkRenderPass* GetRenderPassReference(){return &m_RenderPass;}
    VkRenderPass& GetRenderPass(){return m_RenderPass;}

    void CreateOffScreenRenderPass();
    void CreateRenderPass();
    VkAttachmentDescription SwapchainColourAttachment(const VkFormat& imageFormat);
    VkAttachmentDescription InputPositionAttachment(const VkFormat& imageFormat);
    VkAttachmentDescription InputColourAttachment(const VkFormat& iamgeFormat);
    VkAttachmentDescription InputDepthAttachment();

    std::array<VkSubpassDependency, 2> SetSubpassDependencies();
    void DestroyRenderPass();
private:
    MainDevice* m_MainDevice;
    SwapChain* m_SwapChainHandler;
    VkRenderPass m_RenderPass = {};
    VkRenderPass m_OffScreenRenderPass = {};
};

}
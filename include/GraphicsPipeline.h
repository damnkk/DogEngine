#pragma once

#include "Utilities.h"
#include "RenderPassHandler.h"
#include "SwapChainHandler.h"
#include "DataStructures.h"


namespace dg{
class GraphicPipeline{

public:
    GraphicPipeline();
    GraphicPipeline(MainDevice* mainDevice, SwapChain* swapChain, RenderPassHandler* renderPassHandler);
    VkPipeline& getPipeline() {return m_FirstPipeline;}
    VkPipeline& getSecondPipline(){ return m_SecondPipeline;}
    VkPipelineLayout& getLayout(){return m_FirstPipelineLayout;}
    VkPipelineLayout& getSecondLayout(){return m_SecondPipelineLayout;}
    VkShaderModule CreateShaderModule(const char* path);
    VkPipelineShaderStageCreateInfo CreateVertexShaderStage(const char* vert_str);
    VkPipelineShaderStageCreateInfo CreateFragmentShaderStage(const char* frag_str);
    void CreateGraphicPipeline();

    void SetDescriptorSetLayouts(VkDescriptorSetLayout& descriptorSetLayout, VkDescriptorSetLayout& textureObject,
                                VkDescriptorSetLayout& inputSetLayout, VkDescriptorSetLayout& lightSetLayout, VkDescriptorSetLayout& settings_set_layout);
    void SetPushConstantRange(VkPushConstantRange& pushConstantRange);

    void DestroyShaderModules();
    void DestroyPipeline();
private:
    MainDevice* m_MainDevice;
    SwapChain* m_SwapChain;
    RenderPassHandler* m_RenderPassHandler;

    VkDescriptorSetLayout m_ViewProjectionSetLayout;
    VkDescriptorSetLayout m_TextureSetLayout;
    VkDescriptorSetLayout m_InputSetLayout;
    VkDescriptorSetLayout m_LightSetLayout;
    VkDescriptorSetLayout m_SettingsSetLayout;

    VkPushConstantRange m_PushConstantRange;
private:
    VkPipeline m_FirstPipeline;
    VkPipeline m_SecondPipeline;

    VkPipelineLayout m_FirstPipelineLayout;
    VkPipelineLayout m_SecondPipelineLayout;

    VkPipelineShaderStageCreateInfo m_ShaderStages[2] = {};
    VkPipelineVertexInputStateCreateInfo m_VertexInputStage = {};
    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStage = {};
    VkPipelineViewportStateCreateInfo m_ViewPortScissorStage = {};
    VkPipelineRasterizationStateCreateInfo m_Rasterizer = {};
    VkPipelineMultisampleStateCreateInfo m_MultiSampleStage = {};
    VkPipelineColorBlendStateCreateInfo m_ColourBlendingStage = {};
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStage = {};
    VkPipelineDynamicStateCreateInfo m_DynamicStateStage = {};

private:
    VkShaderModule m_VertexShaderModule;
    VkShaderModule m_FragmentShaderModule;

    VkVertexInputBindingDescription m_VertexStageBindingDescription;
    std::array<VkVertexInputAttributeDescription,3> m_VertexStageAttributeDescription;
    std::vector<VkDynamicState> m_DynamicStates;
    VkRect2D m_Scissor = {};
    VkViewport m_ViewPort = {};

    VkPipelineColorBlendAttachmentState m_ColourStateAttachment = {};
};

} //namespace dg
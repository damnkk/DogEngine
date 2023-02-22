#include "GraphicsPipeline.h"
#include "common.h"


GraphicPipeline::GraphicPipeline(){
    m_MainDevice = {};
    m_FirstPipeline = 0;
    m_FirstPipelineLayout = 0;
    m_RenderPassHandler = new RenderPassHandler();
}

GraphicPipeline::GraphicPipeline(MainDevice* main_device, SwapChain* swaph_chain, RenderPassHandler* renderPassHandler){
    m_MainDevice = main_device;
    m_SwapChain = swaph_chain;
    m_RenderPassHandler = renderPassHandler;
}

void GraphicPipeline::CreateGraphicPipeline(){
    m_ShaderStages[0] = CreateVertexShaderStage("shaders/vert.spv");
    m_ShaderStages[1] = CreateFragmentShaderStage("shaders/frag.spv");

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescriptions = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescriptions;
    
    // INPUT ASSEMBLY
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology =VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount =1;
    viewportStateCreateInfo.scissorCount = 1;

    //RASTERIZER
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    // MULTISAMPLING
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //DEPTH STENCIL TESTING
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_info.stencilTestEnable = VK_FALSE;
    depth_stencil_info.front = {};
    depth_stencil_info.back = {};

    //Blending
    VkPipelineColorBlendAttachmentState colorState = {};
    colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|
                                VK_COLOR_COMPONENT_G_BIT|
                                VK_COLOR_COMPONENT_B_BIT|
                                VK_COLOR_COMPONENT_A_BIT;
    colorState.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorState;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicInfo.pDynamicStates = dynamicStates.data();
    
    std::array<VkDescriptorSetLayout, 2> desc_set_layouts = {
        m_ViewProjectionSetLayout,
        m_TextureSetLayout
    };

    VkPipelineLayoutCreateInfo fst_pipeline_layout = {};
    fst_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    fst_pipeline_layout.pushConstantRangeCount = 1;
    fst_pipeline_layout.pPushConstantRanges = &m_PushConstantRange;   // initialized before pipeline creating.
    fst_pipeline_layout.setLayoutCount = static_cast<uint32_t>(desc_set_layouts.size());
    fst_pipeline_layout.pSetLayouts = desc_set_layouts.data();
    
    if(vkCreatePipelineLayout(m_MainDevice->logicalDevice,&fst_pipeline_layout,nullptr,&m_FirstPipelineLayout) !=VK_SUCCESS){
        throw std::runtime_error("failed to create pipeline layout!");
    }
    
    //GRAPHICS PIPELINE CREATION
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = m_ShaderStages;
    pipeline_info.pVertexInputState = &vertexInputCreateInfo;
    pipeline_info.pInputAssemblyState = &inputAssembly;
    pipeline_info.pViewportState = &viewportStateCreateInfo;
    pipeline_info.pRasterizationState = &rasterizerCreateInfo;
    pipeline_info.pMultisampleState = &multisamplingCreateInfo;
    pipeline_info.pColorBlendState = &colorBlendInfo;
    pipeline_info.pDynamicState = &dynamicInfo;
    pipeline_info.layout = m_FirstPipelineLayout;
    //pipeline_info.renderPass = *m_RenderPassHandler->GetOffScreenRenderPassReference();
    pipeline_info.renderPass = m_RenderPassHandler->GetRenderPass();  //temply use secondRenderpass
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineIndex = 0;

    if(vkCreateGraphicsPipelines(m_MainDevice->logicalDevice,VK_NULL_HANDLE,1,& pipeline_info, nullptr,&m_FirstPipeline) != VK_SUCCESS){
        throw std::runtime_error("failed to create first pipeline!");
    }
    
    DestroyShaderModules();

    // // cuz shader module can be reused, so we dont need to declare m_shaderModule
    // //for every member pipeline

    // m_ShaderStages[0] = CreateVertexShaderStage("./shaders/second_vert.spv");
    // m_ShaderStages[1] = CreateVertexShaderStage("./Shaders/second_frag.spv");

    // vertexInputCreateInfo.vertexBindingDescriptionCount= 0;
    // vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    // vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    // vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    // colorBlendInfo.attachmentCount = 1;
    // colorBlendInfo.pAttachments = &colorState;

    // rasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
    // rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // depth_stencil_info.depthWriteEnable = VK_FALSE;

    // std::array<VkDescriptorSetLayout, 3> snd_pipeline_desc_set_layouts = {
    //     m_InputSetLayout,
    //     m_LightSetLayout,
    //     m_SettingsSetLayout
    // };

    // VkPipelineLayoutCreateInfo snd_pipeline_layout = {};
    // snd_pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // snd_pipeline_layout.setLayoutCount = static_cast<uint32_t>(snd_pipeline_desc_set_layouts.size());
    // snd_pipeline_layout.pSetLayouts = snd_pipeline_desc_set_layouts.data();
    // snd_pipeline_layout.pushConstantRangeCount = 0;
    // snd_pipeline_layout.pPushConstantRanges = nullptr;

    // if(vkCreatePipelineLayout(m_MainDevice->logicalDevice, &snd_pipeline_layout, nullptr, &m_SecondPipelineLayout)!=VK_SUCCESS){
    //     throw std::runtime_error("failed to create snd pipeline layout!");
    // }

    // pipeline_info.pStages = m_ShaderStages;
    // pipeline_info.layout = m_SecondPipelineLayout;
    // pipeline_info.subpass = 0;
    // pipeline_info.renderPass = *m_RenderPassHandler->GetRenderPassReference();

    // if(vkCreateGraphicsPipelines(m_MainDevice->logicalDevice, VK_NULL_HANDLE, 1,&pipeline_info,nullptr,&m_SecondPipeline)!=VK_SUCCESS){
    //     throw std::runtime_error("failed to create snd pipeline!");
    // }

    // DestroyShaderModules();
}

VkShaderModule GraphicPipeline::CreateShaderModule(const char* path){
    std::vector<char> shaderCode = Utility::ReadFile(path);
    return Utility::createShaderModule(shaderCode);
}

void GraphicPipeline::DestroyShaderModules(){
    vkDestroyShaderModule(m_MainDevice->logicalDevice, m_VertexShaderModule, nullptr);
    vkDestroyShaderModule(m_MainDevice->logicalDevice, m_FragmentShaderModule, nullptr);
}

void GraphicPipeline::SetDescriptorSetLayouts(
    VkDescriptorSetLayout& descriptorSetLayout,
    VkDescriptorSetLayout& textureObjects,
    VkDescriptorSetLayout& inputSetLayouts,
    VkDescriptorSetLayout& light_set_layout,
    VkDescriptorSetLayout& settings_set_layout
){
    m_ViewProjectionSetLayout = descriptorSetLayout;
    m_TextureSetLayout = textureObjects;
    m_InputSetLayout = inputSetLayouts;
    m_LightSetLayout = light_set_layout;
    m_SettingsSetLayout = settings_set_layout;

    m_FirstPipeline = 0;
    m_FirstPipelineLayout = 0;

    m_SecondPipeline = 0;
    m_SecondPipelineLayout = 0;
}

void GraphicPipeline::SetPushConstantRange(VkPushConstantRange& pushConstantRange){
    m_PushConstantRange = pushConstantRange;
}

// void GraphicPipeline::SetVertexStageBindingDescription(){
//     m_VertexStageAttributeDescription = Vertex::getAttributeDescriptions();
// }

// void GraphicPipeline::SetViewPort(){
//     m_ViewPort.x = 0.f;
//     m_ViewPort.y = 0.f;
//     m_ViewPort.width = static_cast<float>(m_SwapChain->GetExtentWidth());
//     m_ViewPort.height = static_cast<float>(m_SwapChain->GetExtentHeight());
//     m_ViewPort.minDepth = 0.0f;
//     m_ViewPort.height = 1.0f;
// }

// void GraphicPipeline::SetScissor(){
//     m_Scissor.offset = {0,0};
//     m_Scissor.extent = m_SwapChain->GetExtent();
// }

// void GraphicPipeline::CreateVertexInputStage(){
//     SetVertexStageBindingDescription();
//     SetVertexAttriuteDescriptions();
//     m_VertexInputStage = {};
//     m_VertexInputStage.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     m_VertexInputStage.vertexBindingDescriptionCount = 1;
//     m_VertexInputStage.pVertexBindingDescriptions =&m_VertexStageBindingDescription;
//     m_VertexInputStage.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_VertexStageAttributeDescription.size());
//     m_VertexInputStage.pVertexAttributeDescriptions = m_VertexStageAttributeDescription.data();
// }

// void GraphicPipeline::CreateInputAssemblyStage(){
//     m_InputAssemblyStage = {};
//     m_InputAssemblyStage.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     m_InputAssemblyStage.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     m_InputAssemblyStage.primitiveRestartEnable = VK_FALSE;
// }

// void GraphicPipeline::CreateViewportScissorStage(){
//     SetViewPort();
//     SetScissor();

//     m_ViewPortScissorStage = {};
//     m_ViewPortScissorStage.sType =VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     m_ViewPortScissorStage.pViewports = &m_ViewPort;
//     m_ViewPortScissorStage.pScissors = &m_Scissor;
//     m_ViewPortScissorStage.viewportCount = 1;
//     m_ViewPortScissorStage.scissorCount = 1;
// }

// void GraphicPipeline::CreateDynamicStatesStage(){
//     m_DynamicStateStage = {};
//     m_DynamicStateStage.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//     m_DynamicStateStage.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size());
//     m_DynamicStateStage.pDynamicStates = m_DynamicStates.data();
// }

// void GraphicPipeline::CreateRasterizerStage(){
//     m_Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     m_Rasterizer.depthClampEnable =VK_FALSE;
//     m_Rasterizer.rasterizerDiscardEnable = VK_FALSE;
//     m_Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//     m_Rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//     m_Rasterizer.lineWidth = 1.0f;
//     m_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//     m_Rasterizer.depthBiasEnable = VK_FALSE;
// }

// void GraphicPipeline::CreateMultisampleStage(){
//     m_MultiSampleStage.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     m_MultiSampleStage.sampleShadingEnable = VK_FALSE;
//     m_MultiSampleStage.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
// }

// void GraphicPipeline::CreateColourBlendingStage(){
//     m_ColourBlendingStage = {};
//     m_ColourBlendingStage.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     m_ColourBlendingStage.
// }

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateVertexShaderStage(const char* vertex_str){
    m_VertexShaderModule = CreateShaderModule(vertex_str);
    VkPipelineShaderStageCreateInfo vertexShaderCreaInfo = {};
    vertexShaderCreaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreaInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreaInfo.module = m_VertexShaderModule;
    vertexShaderCreaInfo.pName = "main";
    return vertexShaderCreaInfo;
}

VkPipelineShaderStageCreateInfo GraphicPipeline::CreateFragmentShaderStage(const char* frag_str){
    m_FragmentShaderModule = CreateShaderModule(frag_str);
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = m_FragmentShaderModule;
    fragmentShaderCreateInfo.pName ="main";
    return fragmentShaderCreateInfo;
}

void GraphicPipeline::DestroyPipeline(){
    vkDestroyPipeline(m_MainDevice->logicalDevice, m_FirstPipeline, nullptr);
    vkDestroyPipelineLayout(m_MainDevice->logicalDevice, m_FirstPipelineLayout, nullptr);
    vkDestroyPipeline(m_MainDevice->logicalDevice, m_SecondPipeline,nullptr);
    vkDestroyPipelineLayout(m_MainDevice->logicalDevice, m_SecondPipelineLayout, nullptr);
}
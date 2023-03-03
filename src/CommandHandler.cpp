#include "common.h"
#include "CommandHandler.h"
CommandHandler::CommandHandler(){
    m_MainDevice = {};
    m_GraphicsCommandPool = {};
    m_CommandBuffers = {};
    m_graphicPipeline = nullptr;
}

CommandHandler::CommandHandler(MainDevice* mainDevice, GraphicPipeline* pipeline, RenderPassHandler* renderPassHandler){
    m_MainDevice = mainDevice;
    m_graphicPipeline = pipeline;
    m_RenderPassHandler = renderPassHandler;
    m_GraphicsCommandPool = {};
    m_CommandBuffers = {};
}

void CommandHandler::CreateCommandPool(QueueFamilyIndices& queueIndices){
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
    if(vkCreateCommandPool(m_MainDevice->logicalDevice,& poolInfo,nullptr,&m_GraphicsCommandPool)!=VK_SUCCESS){
        throw std::runtime_error("failed to create command Pool!");
    }
}

void CommandHandler::CreateCommandBuffers(size_t const numFrameBuffers){
    m_CommandBuffers.resize(numFrameBuffers);
    VkCommandBufferAllocateInfo bufferAllocInfo = {};
    bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocInfo.commandPool = m_GraphicsCommandPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
    if(vkAllocateCommandBuffers(m_MainDevice->logicalDevice, &bufferAllocInfo,m_CommandBuffers.data())!=VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void CommandHandler::RecordOffScreenCommands(ImDrawData* draw_data, uint32_t currImage, VkExtent2D& imageExtent,
    std::vector<VkFramebuffer>& offScreenFrameBuffers, std::vector<Model>& scene, std::vector<VkDescriptorSet>& descriptorsets){
    VkCommandBufferBeginInfo buffer_begin_info = {};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    std::array<VkClearValue,4> clear_values;
    clear_values[0].color = {0.0f,0.0f,0.0f,0.0f};//position
    clear_values[1].color = {0.0f,0.0f,0.0f,0.0f};//Color
    clear_values[2].color = {0.0f,0.0f,0.0f,0.0f};//normal
    clear_values[3].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo renderPass_begin_info = {};
    renderPass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPass_begin_info.renderPass = *m_RenderPassHandler->GetOffScreenRenderPassReference();
    renderPass_begin_info.renderArea.offset = {0,0};
    renderPass_begin_info.renderArea.extent = imageExtent;
    renderPass_begin_info.pClearValues = clear_values.data();
    renderPass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    renderPass_begin_info.framebuffer = offScreenFrameBuffers[currImage];

    if(vkBeginCommandBuffer(m_CommandBuffers[currImage],&buffer_begin_info)!=VK_SUCCESS){
        throw std::runtime_error("Failed to begin off screen commandBuffer!");
    }
    vkCmdBeginRenderPass(m_CommandBuffers[currImage],&renderPass_begin_info,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_CommandBuffers[currImage],VK_PIPELINE_BIND_POINT_GRAPHICS,m_graphicPipeline->getPipeline());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)imageExtent.width;
    viewport.height =(float)imageExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_CommandBuffers[currImage],0,1,&viewport);

    VkRect2D scissor{};
    scissor.extent = imageExtent;
    scissor.offset = {0,0};
    vkCmdSetScissor(m_CommandBuffers[currImage],0,1,&scissor);
    for(auto& model:scene){
        model.draw(m_MainDevice->logicalDevice,m_CommandBuffers[currImage],descriptorsets[currImage],m_graphicPipeline->getLayout());
    }
    vkCmdEndRenderPass(m_CommandBuffers[currImage]);
    if(vkEndCommandBuffer(m_CommandBuffers[currImage])!=VK_SUCCESS){
        throw std::runtime_error("Failed to stop record offscreen command buffer!");
    }
}

void CommandHandler::RecordCommands(ImDrawData* draw_data,uint32_t currImage,VkExtent2D& imageExtent,
    std::vector<VkFramebuffer>& frameBuffers,
    std::vector<VkDescriptorSet>& vp_desc_sets,
    std::vector<VkDescriptorSet>& light_desc_sets,
    std::vector<VkDescriptorSet>& input_desc_sets,
    std::vector<VkDescriptorSet>& settings_desc_set){
    VkCommandBufferBeginInfo buffer_begin_info = {};
    buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    std::array<VkClearValue, 2> clear_values;
    clear_values[0].color = {0.0f,0.0f,0.0f,0.0f};
    clear_values[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo renderpass_begin_info = {};
    renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begin_info.renderPass = *m_RenderPassHandler->GetRenderPassReference();
    renderpass_begin_info.renderArea.offset = {0,0};
    renderpass_begin_info.renderArea.extent = imageExtent;
    renderpass_begin_info.pClearValues = clear_values.data();
    renderpass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    renderpass_begin_info.framebuffer = frameBuffers[currImage];

    if(vkBeginCommandBuffer(m_CommandBuffers[currImage],& buffer_begin_info)!=VK_SUCCESS){
        throw std::runtime_error("Failed to start recording command buffer for presentation!");
    }
    vkCmdBeginRenderPass(m_CommandBuffers[currImage],&renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_CommandBuffers[currImage],VK_PIPELINE_BIND_POINT_GRAPHICS,m_graphicPipeline->getSecondPipline());
    VkViewport viewPort{};
    viewPort.x = 0.0f;
    viewPort.y = 0.0f;
    viewPort.maxDepth = 1.0f;
    viewPort.minDepth = 0.0f;
    viewPort.width = (float)imageExtent.width;
    viewPort.height = (float)imageExtent.height;
    vkCmdSetViewport(m_CommandBuffers[currImage],0,1,&viewPort);

    VkRect2D scissor{};
    scissor.extent = imageExtent;
    scissor.offset = {0,0};
    vkCmdSetScissor(m_CommandBuffers[currImage],0,1,&scissor);
    {
        std::array<VkDescriptorSet, 4> desc_set_group = {
            vp_desc_sets[currImage],
            input_desc_sets[currImage],
            light_desc_sets[currImage],
            settings_desc_set[currImage]
        };
        vkCmdBindDescriptorSets(m_CommandBuffers[currImage],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_graphicPipeline->getSecondLayout(),0,
                                static_cast<uint32_t>(desc_set_group.size()),desc_set_group.data(),0, nullptr);
    }
    vkCmdDraw(m_CommandBuffers[currImage],3,1,0,0);
    ImGui_ImplVulkan_RenderDrawData(draw_data, m_CommandBuffers[currImage]);
    vkCmdEndRenderPass(m_CommandBuffers[currImage]);
    if(vkEndCommandBuffer(m_CommandBuffers[currImage])!=VK_SUCCESS){
        throw std::runtime_error("Failed to stop recording present command buffer!");
    }
}

void CommandHandler::RecordCommandBuffers_forward(ImDrawData* draw_data, uint32_t currFrame,uint32_t currImage, VkExtent2D imgExtent,
    std::vector<VkFramebuffer>& frameBuffers,
    std::vector<Model>& scene,
    std::vector<VkDescriptorSet>& descriptorSets){
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if(vkBeginCommandBuffer(m_CommandBuffers[currFrame], &beginInfo)!=VK_SUCCESS){
        throw std::runtime_error("Failed to begin temp commmand buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPassHandler->GetRenderPass();
    renderPassInfo.framebuffer = frameBuffers[currImage];
    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = imgExtent;
    std::array<VkClearValue,2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f,0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(m_CommandBuffers[currFrame],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_CommandBuffers[currFrame],VK_PIPELINE_BIND_POINT_GRAPHICS,m_graphicPipeline->getPipeline());

    VkViewport viewPort{};
    viewPort.x = 0.0f;
    viewPort.y = 0.0f;
    viewPort.maxDepth = 1.0f;
    viewPort.minDepth = 0.0f;
    viewPort.width = (float)imgExtent.width;
    viewPort.height = (float)imgExtent.height;
    vkCmdSetViewport(m_CommandBuffers[currFrame],0,1,&viewPort);

    VkRect2D scissor{};
    scissor.extent = imgExtent;
    scissor.offset = {0,0};
    vkCmdSetScissor(m_CommandBuffers[currFrame],0,1,&scissor);
    for(auto& model:scene){
        model.draw(m_MainDevice->logicalDevice, m_CommandBuffers[currFrame], descriptorSets[currFrame],m_graphicPipeline->getLayout());
    }
    ImGui_ImplVulkan_RenderDrawData(draw_data,m_CommandBuffers[currFrame]);
    vkCmdEndRenderPass(m_CommandBuffers[currFrame]);
    if(vkEndCommandBuffer(m_CommandBuffers[currFrame])!=VK_SUCCESS){
        throw std::runtime_error("Failed to end temp command buffer!");
    }
}

void CommandHandler::DestroyCommandPool(){
    vkDestroyCommandPool(m_MainDevice->logicalDevice, m_GraphicsCommandPool, nullptr);
}

void CommandHandler::FreeCommandBuffers(){
    vkFreeCommandBuffers(m_MainDevice->logicalDevice, m_GraphicsCommandPool,static_cast<uint32_t>(m_CommandBuffers.size()),m_CommandBuffers.data());
}



#include "common.h"
#include "CommandHandler.h"
CommandHandler::CommandHandler(){
    m_MainDevice = {};
    m_GraphicsCommadnPool = {};
    m_CommandBuffers = {};
    m_graphicPipeline = nullptr;
}

CommandHandler::CommandHandler(MainDevice* mainDevice, GraphicPipeline* pipeline, RenderPassHandler* renderPassHandler){
    m_MainDevice = mainDevice;
    m_graphicPipeline = pipeline;
    m_RenderPassHandler = renderPassHandler;
    m_GraphicsCommadnPool = {};
    m_CommandBuffers = {};
}

void CommandHandler::CreateCommandPool(QueueFamilyIndices& queueIndices){
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndices.graphicsFamily.value();
    if(vkCreateCommandPool(m_MainDevice->logicalDevice,& poolInfo,nullptr,&m_GraphicsCommadnPool)!=VK_SUCCESS){
        throw std::runtime_error("failed to create command Pool!");
    }
}

void CommandHandler::CreateCommandBuffers(size_t const numFrameBuffers){
    m_CommandBuffers.resize(numFrameBuffers);
    VkCommandBufferAllocateInfo bufferAllocInfo = {};
    bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocInfo.commandPool = m_GraphicsCommadnPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
    if(vkAllocateCommandBuffers(m_MainDevice->logicalDevice, nullptr,m_CommandBuffers.data())!=VK_SUCCESS){
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}
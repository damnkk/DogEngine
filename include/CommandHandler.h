#pragma once 
#include "common.h"

#include "GraphicsPipeline.h"
#include "RenderPassHandler.h"
#include "Mesh&Model.h"

class CommandHandler{
public:
    CommandHandler();
    CommandHandler(MainDevice* main_device, GraphicPipeline* pipeline, RenderPassHandler* RenderPassHandler);
    void CreateCommandPool(QueueFamilyIndices& queueIndices);
    void CreateCommandBuffers(size_t const numFrameBuffers);
    void RecordOffScreenCommands(ImDrawData* draw_data, uint32_t currentImage);
    void RecordCommands(ImDrawData* draw_data, uint32_t current_img,
        std::vector<VkDescriptorSet>& light_desc_sets,
        std::vector<VkDescriptorSet>& inputDescriptorSet,
        std::vector<VkDescriptorSet>& settings_desc_set);

    void RecordCommandBuffers_temp(VkCommandBuffer& commandBuffer, uint32_t currImage);
    void DestroyCommandPool();
    void FreeCommandBuffers();

    VkCommandPool& GetCommandPool(){ return m_GraphicsCommadnPool;}
    VkCommandBuffer& GetCommadnBuffer(uint32_t const index) { return m_CommandBuffers[index];}
    std::vector<VkCommandBuffer>& GetCommandBuffers() {return m_CommandBuffers;}
private:
    MainDevice *m_MainDevice;
    RenderPassHandler *m_RenderPassHandler;
    GraphicPipeline *m_graphicPipeline;

    VkCommandPool m_GraphicsCommadnPool;
    std::vector<VkCommandBuffer> m_CommandBuffers;
};
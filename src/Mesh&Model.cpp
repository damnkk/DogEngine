#include "Mesh&Model.h"

void Mesh::draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
              VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets){
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    constentData constant;
    constant.textureIndex = textureIndex;
    extern int textureNum;
    constant.textureNum = textureNum;

    vkCmdPushConstants(commandbuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constentData), &textureIndex);
    vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Model::draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout, VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets){
    for (int i = 0; i < meshes.size();++i)
    {
        meshes.at(i).draw(device, commandBuffer, descriptorSet, pipelineLayout, sampler, descriptorsets);
    }
}
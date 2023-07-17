#include "MeshModel.h"

void Mesh::draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
              VkSampler& sampler, glm::mat4 model){
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    constentData constant;
    constant.modelMatrix = model;
    extern std::unordered_map<std::string, Texture> textures;
    VkDescriptorSet textureDescriptorset = textures[textureIndex].textureDescriptor;
    std::vector<VkDescriptorSet> descriptorsetss = {descriptorSet, textureDescriptorset};

    vkCmdPushConstants(commandbuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constentData), &constant);
    vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0,descriptorsetss.size(), descriptorsetss.data(), 0, nullptr);
    vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Model::draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout, VkSampler& sampler){
    for (int i = 0; i < meshes.size();++i)
    {
        meshes.at(i).draw(device, commandBuffer, descriptorSet, pipelineLayout, sampler, model);
    }
}
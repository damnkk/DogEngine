#include "Mesh&Model.h"

void Mesh::draw(VkCommandBuffer& commandBuffer, VkDescriptorSet& VkDescriptorSet, VkPipelineLayout& pipelineLayout){
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &VkDescriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Model::draw(VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout){
    for (int i = 0; i < meshes.size();++i)
    {
        meshes.at(i).draw(commandBuffer, descriptorSet, pipelineLayout);
    }
}

void Texture::destroy(VkDevice& device,VmaAllocator &allocator){
    vkDestroyImageView(device, textureImageView, nullptr);
    vmaDestroyImage(allocator, textureImage, allocation);
}
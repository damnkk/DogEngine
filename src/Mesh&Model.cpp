#include "Mesh&Model.h"

void Mesh::draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout, VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets){
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = diffuseTexture.textureImageView;
    imageInfo.sampler = sampler;

    std::vector<VkWriteDescriptorSet> descriptorWrites(1);
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSet;
    descriptorWrites[0].dstBinding = 1;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device,static_cast<uint32_t>(descriptorWrites.size()),descriptorWrites.data(), 0, nullptr);
    

    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Model::draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout, VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets){
    for (int i = 0; i < meshes.size();++i)
    {
        meshes.at(i).draw(device, commandBuffer, descriptorSet, pipelineLayout, sampler, descriptorsets);
    }
}
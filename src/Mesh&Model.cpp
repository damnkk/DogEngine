#include "Mesh&Model.h"

namespace dg{
    std::unordered_map<std::string, Texture> Mesh::textures = {};
void Mesh::draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
    glm::mat4 model){
    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    constentData constant;
    constant.modelMatrix = model;
    VkDescriptorSet textureDescriptorset = textures[textureIndex].textureDescriptor;
    std::vector<VkDescriptorSet> descriptorsetss = {descriptorSet, textureDescriptorset};

    vkCmdPushConstants(commandbuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(constentData), &constant);
    vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0,descriptorsetss.size(), descriptorsetss.data(), 0, nullptr);
    vkCmdDrawIndexed(commandbuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Mesh::createVertexBuffer(std::vector<Vertex>& vertices, Buffer& vertexBuffer, VmaAllocator* allocator)
  {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    Buffer stagingBuffer;
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    void *data;
    vmaMapMemory(*allocator, stagingBuffer.allocation,&data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vmaUnmapMemory(*allocator, stagingBuffer.allocation);

    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer);

    Utility::CopyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);

    vmaDestroyBuffer(*allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  }

  void Mesh::createIndexBuffer(std::vector<uint32_t> indices, Buffer& indexBuffer, VmaAllocator* allocator)
  {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    Buffer stagingBuffer;
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

    void *data;
    vmaMapMemory(*allocator, stagingBuffer.allocation, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vmaUnmapMemory(*allocator, stagingBuffer.allocation);
    Utility::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer);

    Utility::CopyBuffer(stagingBuffer.buffer, indexBuffer.buffer, bufferSize);

    vmaDestroyBuffer(*allocator, stagingBuffer.buffer, stagingBuffer.allocation);
  }


void Model::draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet, VkPipelineLayout& pipelineLayout){
    for (int i = 0; i < meshes.size();++i)
    {
        meshes.at(i).draw(device, commandBuffer, descriptorSet, pipelineLayout, model);
    }
}

} //namespace dg
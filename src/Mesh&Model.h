#pragma once
#include<iostream>
#include "Vertex.h"
#include "allocateObject.h"
class Mesh{
public:
    Mesh(){}
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t textureIndex;

    void draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
              VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets);
};

class Model{
public:
    Model(){}
    std::vector<Mesh> meshes;
    glm::mat4 model;

    void draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, VkSampler& sampler, std::vector<VkDescriptorSet>& descriptorsets);
};
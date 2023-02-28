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
    std::string textureIndex;

    void draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
              glm::mat4 Model);
};

class Model{
public:
    Model(){}
    std::vector<Mesh> meshes;
    glm::mat4 model;

    void draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout);
};
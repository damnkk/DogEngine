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
    Texture diffuseTexture;

    void draw(VkCommandBuffer& commandbuffer, VkDescriptorSet &VkDescriptorSet, VkPipelineLayout &layout);
};

class Model{
public:
    Model(){}
    std::vector<Mesh> meshes;
    glm::mat4 model;

    void draw(VkCommandBuffer &VkCommandBuffer, VkDescriptorSet &VkDescriptorSet, VkPipelineLayout& layout);
};
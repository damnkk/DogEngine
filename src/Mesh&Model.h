#pragma once
#include<iostream>
#include "Vertex.h"



struct Texture{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VmaAllocation allocation;
};

class Mesh{
public:
    Mesh(){}
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    Texture meshTexture;

    void draw(VkCommandBuffer& commandbuffer, VkDescriptorSet &VkDescriptorSet, VkPipelineLayout &layout);
};

class Model{
public:
    Model(){}
    std::vector<Mesh> meshes;
    glm::mat4 model;

    void draw(VkCommandBuffer &VkCommandBuffer, VkDescriptorSet &VkDescriptorSet, VkPipelineLayout& layout);
};
#pragma once
#include<iostream>
#include "Vertex.h"
#include "allocateObject.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

class GltfModel{
public:
    VkDevice* logicalDevice;
    VkQueue copyQueue;

    struct Vertex{
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 color;
        VkVertexInputBindingDescription getVertexBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescription();
    };

    struct{
        VkBuffer vertexBuffer;
        VkDeviceMemory bufferMemory;
    } Vertices;

    struct{
        VkBuffer indexBuffer;
        VkDeviceMemory indexMemory;
    }Indices;

    struct mTexture{
    VkImage textureImage;
    VmaAllocation allocation;
    VkDescriptorSet textureDescriptor;
    VkFormat Format		= {};
    VkDescriptorImageInfo textureInfo;
    uint32_t miplevels = 0;
    void destroy(VkDevice& device,VmaAllocator &allocator){
        vkDestroyImageView(device, textureInfo.imageView, nullptr);
    vmaDestroyImage(allocator, textureImage, allocation);
    }
    };
    struct Node{
        uint32_t firstIndex;
        uint32_t indexCount;
        mTexture albedoTex;
        mTexture emissiveTex;
        mTexture normalTex;
        glm::mat4 modelMatrix;
    };
    


};




class Mesh{
public:
    Mesh(){}
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::string textureIndex;

    void draw(VkDevice& device, VkCommandBuffer& commandbuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, 
              VkSampler& sampler, glm::mat4 Model);
};

class Model{
public:
    Model(){}
    std::vector<Mesh> meshes;
    glm::mat4 model;

    void draw(VkDevice& device, VkCommandBuffer& commandBuffer, VkDescriptorSet &descriptorSet, VkPipelineLayout& layout, VkSampler& sampler);
};


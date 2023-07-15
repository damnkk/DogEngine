
#ifndef _MESH_MODEL_
#define _MESH_MODEL_

#include<iostream>
#include "Vertex.h"
#include "allocateObject.h"
#include "Utilities.h"

#include "tiny_gltf.h"

class GltfModel{
public:
    static VkDevice* logicalDevice;
    static VkQueue copyQueue;
    static VmaAllocator* vmaAllocator;
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
        VmaAllocation allocation;
    } Vertices;
    struct{
        VkBuffer indexBuffer;
        VmaAllocation allocation;
    } Indices;

    struct Primitive{
        uint32_t firstIndex;
        uint32_t indexCount;
        int32_t  materialIndex;
    };

    struct Mesh{
        std::vector<Primitive> primitives;
    };

    struct Material{
        glm::vec4 baseColorFactor = glm::vec4(1.0f);
        uint32_t baseColorTextureIndex;
        uint32_t metallicRoughnessTextureIndex;
        uint32_t occlusionTextureIndex;
        uint32_t normalTextureIndex;
        glm::vec3 emmissiveFactor = glm::vec3(1.0f);
        uint32_t emissiveTextureIndex;
    };

    struct TTexture{
        uint32_t textureIndex;
    };

    struct Node{
       Node* parent;
       std::vector<Node*> children;
       Mesh mesh;
       glm::mat4 matrix;
       ~Node(){
            for(auto& node:children){
                delete node;
            }
        }
    };
    std::vector<Node*> mNodes;
    std::vector<TTexture> textures;
    std::vector<Material> materials;
    std::vector<Texture> images;

    void loadImage(tinygltf::Model& input){
        images.resize(input.images.size());
        for(size_t i = 0;i<images.size();++i){
            tinygltf::Image& gltfImage = input.images[i];
            unsigned char* buffer = nullptr;
            VkDeviceSize bufferSize = 0;
            bool deleteBuffer = false;
            if(gltfImage.component == 3){
                bufferSize = gltfImage.width*gltfImage.height*4;
                buffer = new unsigned char[bufferSize];
                unsigned char* rgba = buffer;
                unsigned char* rgb  = gltfImage.image.data();
                for( size_t i = 0;i<gltfImage.width*gltfImage.height;++i){
                    memcpy(rgba,rgb,3*sizeof(unsigned char));
                    rgba += 4;
					rgb += 3;
                }
                deleteBuffer = true;
            }else{
                buffer = &gltfImage.image[0];
                bufferSize = gltfImage.image.size();
            }
            Buffer stagingBuffer;
            Utility::CreateBuffer(bufferSize,VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer);
            void* data ;
            vmaMapMemory(*vmaAllocator,stagingBuffer.allocation,&data);
            memcpy(data,buffer,static_cast<size_t>(bufferSize));
            vmaUnmapMemory(*vmaAllocator,stagingBuffer.allocation);
            if(deleteBuffer) {
                delete buffer;
                buffer = nullptr;
            }
            Utility::createImage(gltfImage.width,gltfImage.height,0,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_TILING_OPTIMAL,VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|
                                    VK_IMAGE_USAGE_SAMPLED_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,images[i]);
            Utility::createImageView(images[i].textureImage,VK_FORMAT_R8G8B8A8_SRGB,VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,1);
            Utility::CreateTextureSampler(images[i]);
        }
    }

    void drawNode(VkCommandBuffer comdBuffer,VkPipelineLayout pipeLayout, Node* node){
        if(node->mesh.primitives.size()>0){
            glm::mat4 nodeMatrix = node->matrix;
            Node* parentNode = node->parent;
            while(parentNode){
                nodeMatrix = parentNode->matrix*nodeMatrix;
                parentNode = parentNode->parent;
            }
            vkCmdPushConstants(comdBuffer,pipeLayout,VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(glm::mat4),&nodeMatrix);
            for(auto& primitive:node->mesh.primitives){
               if(primitive.indexCount>0){
                    TTexture baseColor = textures[materials[primitive.materialIndex].baseColorTextureIndex];
                    TTexture metallicRoughness = textures[materials[primitive.materialIndex].metallicRoughnessTextureIndex];
                    TTexture emissive =textures[materials[primitive.materialIndex].emissiveTextureIndex];
                    TTexture normalTex =textures[materials[primitive.materialIndex].normalTextureIndex];
                    TTexture occulusion =textures[materials[primitive.materialIndex].occlusionTextureIndex];
                    std::vector<VkDescriptorSet> descSet{images[baseColor.textureIndex].textureDescriptor,
                                                        images[metallicRoughness.textureIndex].textureDescriptor,
                                                        images[emissive.textureIndex].textureDescriptor,
                                                        images[normalTex.textureIndex].textureDescriptor,
                                                        images[occulusion.textureIndex].textureDescriptor};
                    vkCmdBindDescriptorSets(comdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,pipeLayout,0,5, descSet.data(),0,0);
                    vkCmdDrawIndexed(comdBuffer, primitive.indexCount,1,primitive.firstIndex,0,0);
               }
            }
        }
    }

    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout){
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer,0,1,&Vertices.vertexBuffer,0);
        vkCmdBindIndexBuffer(commandBuffer,Indices.indexBuffer,0,VK_INDEX_TYPE_UINT32);
        for(auto node:mNodes){
            drawNode(commandBuffer, pipelineLayout,node);
        }
    }

    ~GltfModel(){
        for(auto node:mNodes){
            delete node;
        }
        vmaDestroyBuffer(*vmaAllocator,Vertices.vertexBuffer,Vertices.allocation);
        vmaDestroyBuffer(*vmaAllocator, Indices.indexBuffer,Indices.allocation);
        for(auto& image:images){
            image.destroy(*logicalDevice,*vmaAllocator);
        }
    }
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

#endif //_MESH_MODEL_

#ifndef _MESH_MODEL_
#define _MESH_MODEL_

#include<iostream>
#include "Vertex.h"
#include "allocateObject.h"
#include "Utilities.h"
#include <glm/gtc/type_ptr.hpp>

#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"

class GltfModel{
public:
    VkDevice* logicalDevice;
    VkQueue* copyQueue;
    VmaAllocator* vmaAllocator;
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
        int count;
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
        glm::vec3 emissiveFactor = glm::vec3(1.0f);
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
                deleteBuffer = true;
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

            Utility::createImage(gltfImage.width,gltfImage.height,images[i].miplevels,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_TILING_OPTIMAL,VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|
                                    VK_IMAGE_USAGE_SAMPLED_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,images[i]);
            Utility::createImageView(images[i].textureImage,VK_FORMAT_R8G8B8A8_SRGB,VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,1);
            Utility::CreateTextureSampler(images[i]);
        }
    }

    void loadTextures(tinygltf::Model& input){
        textures.resize(input.textures.size());
        for(size_t i = 0;i<input.textures.size();++i){
            textures[i].textureIndex = input.textures[i].source;
        }
    }

    void loadMaterials(tinygltf::Model& input){
        materials.resize(input.materials.size());
        for(uint32_t i = 0;i<input.materials.size();++i){
            tinygltf::Material gltfMaterial = input.materials[i];
            decltype(gltfMaterial.values)& value = gltfMaterial.values;
            if(value.find("baseColorFactor")!= value.end()){
                materials[i].baseColorFactor = glm::make_vec4(value["baseColorFactor"].ColorFactor().data());
            }
            if(value.find("emissiveFactor") != value.end()){
                materials[i].emissiveFactor = glm::make_vec3(value["emissiveFactor"].ColorFactor().data());
            }
            if(value.find("baseColorTexture")!= value.end()){
                materials[i].baseColorTextureIndex = value["baseColorTexture"].TextureIndex();
            }
            if(value.find("emissiveTexture") != value.end()){
                materials[i].emissiveTextureIndex = value["emissiveTexture"].TextureIndex();
            }
            if(value.find("occlusionTexture") != value.end()){
                materials[i].occlusionTextureIndex = value["occlusionTexture"].TextureIndex();
            }
            if(value.find("normalTexture")!= value.end()){
                materials[i].normalTextureIndex = value["normalTexture"].TextureIndex();
            }
            if(value.find("metallicRoughnessTexture")!=value.end()){
                materials[i].metallicRoughnessTextureIndex = value["metallicRoughnessTexture"].TextureIndex();
            }
        }
    }

    void loadNode(const tinygltf::Node& inputNode,const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer){
        Node* node = new Node();
        node->matrix = glm::mat4(1.0f);
        node->parent = parent;

        if(inputNode.translation.size()==3){
            node->matrix = glm::translate(node->matrix,glm::vec3(glm::make_vec3(inputNode.translation.data())));
        }
        if (inputNode.rotation.size() == 4) {
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node->matrix *= glm::mat4(q);
		}
		if (inputNode.scale.size() == 3) {
			node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
		}
		if (inputNode.matrix.size() == 16) {
			node->matrix = glm::make_mat4x4(inputNode.matrix.data());
		};

        if(inputNode.children.size()>0){
            for(uint32_t i = 0;i<inputNode.children.size();++i){
                loadNode(input.nodes[inputNode.children[i]],input,node,indexBuffer,vertexBuffer);
            }
        }

        if(inputNode.mesh>-1){
            const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
            for(size_t i = 0;i<mesh.primitives.size();++i){
                const tinygltf::Primitive& gltfPrimit = mesh.primitives[i];
                uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
                uint32_t indexCount = 0;
                uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());

                //vertex
                {
                    const float* positionBuffer = nullptr;
                    const float* normalsBuffer = nullptr;
                    const float* texCoordsBuffer = nullptr;
                    size_t vertexCount = 0;
                    const decltype(gltfPrimit.attributes)& attrib = gltfPrimit.attributes;
                    if(attrib.find("POSITION") != attrib.end()){
                        const tinygltf::Accessor& accessor = input.accessors[attrib.find("POSITION")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }
                    if(attrib.find("NORMAL")!=attrib.end()){
                        const tinygltf::Accessor& accessor = input.accessors[attrib.find("NORMAL")->second];
                        const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset+view.byteOffset]));

                    }
                    if (attrib.find("TEXCOORD_0") != attrib.end()) {
						const tinygltf::Accessor& accessor = input.accessors[attrib.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

                    for (size_t v = 0;v<vertexCount;++v){
                        Vertex vert{};
                        vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v*3]),1.0f);
                        vert.normal = glm::normalize(glm::vec3(normalsBuffer?glm::make_vec3(&normalsBuffer[v*3]):glm::vec3(0.0f)));
                        vert.uv = texCoordsBuffer? glm::make_vec2(&texCoordsBuffer[v*3]):glm::vec2(0.0f);
                        vert.color = glm::vec3(1.0f);
                        vertexBuffer.push_back(vert);
                    }
                }

                //Indices
                {
                    const tinygltf::Accessor& accessor = input.accessors[gltfPrimit.indices];
                    const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

                    indexCount =static_cast<uint32_t>( accessor.count);
                    switch (accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:{
                            const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset+bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
							    indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                            const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
                                indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                            const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                            for (size_t index = 0; index < accessor.count; index++) {
                                indexBuffer.push_back(buf[index] + vertexStart);
                            }
                            break;
                        }
                        default:
                            std::cerr<<"Index component type "<<accessor.componentType<<" not supported!"<<std::endl;
                            return;
                        }
                }
                Primitive prim{};
                prim.firstIndex = firstIndex;
                prim.indexCount = indexCount;
                prim.materialIndex = gltfPrimit.material;
                node->mesh.primitives.push_back(prim);

            } 
        }
        if(parent){
            parent->children.push_back(node);
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
#include "ModelLoader.h"
#include "DeviceContext.h"
#include "tiny_obj_loader.h"
#include "stb_image.h"
#include "tiny_gltf.h"
#include "CommandBuffer.h"
#include "Renderer.h"
namespace dg{
    std::array<glm::vec3, 2> SceneNode::getAABB(){
        std::array<glm::vec3, 2> temp;
        return temp;
    }

    void SceneNode::update(){
        m_isUpdate = true;
    }
    
    template<typename T>
    BufferHandle upLoadBufferToGPU(Renderer* renderer, std::vector<T>& bufferData, const char* meshName){
        if(bufferData.size()==0) return {k_invalid_index};
        VkDeviceSize BufSize = bufferData.size()*sizeof(T);
        BufferCreateInfo bufferInfo;
        bufferInfo.reset();
        bufferInfo.setData(bufferData.data());
        bufferInfo.setDeviceOnly(true);
        if(std::is_same<T, Vertex>::value){
            std::string vertexBufferName = "vt";
            vertexBufferName +=meshName;
            bufferInfo.setUsageSize(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, BufSize);
            bufferInfo.setName(vertexBufferName.c_str());
        }else if(std::is_same<T,u32>::value){
            std::string IndexBufferName = "id";
            IndexBufferName +=meshName;
            bufferInfo.setUsageSize(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, BufSize);\
            bufferInfo.setName(IndexBufferName.c_str());
        }
        BufferHandle GPUHandle = renderer->createBuffer(bufferInfo)->handle;
        return GPUHandle;
    }

    TextureHandle upLoadTextureToGPU(Renderer* renderer, std::string texPath){
        if(texPath.empty()) return {k_invalid_index};
        int imgWidth,imgHeight,imgChannel; 
        auto ptr = stbi_load(texPath.c_str(),&imgWidth,&imgHeight,&imgChannel,4);
        TextureCreateInfo texInfo;
        u32 foundHead = texPath.find_last_of("/\\");
        u32 foundEnd = texPath.find_last_not_of(".");
        std::string texName = texPath.substr(foundHead+1,foundEnd);
        texInfo.setName(texName.c_str()).setData(ptr).setExtent({(u32)imgWidth,(u32)imgHeight,1})
        .setFormat(VK_FORMAT_R8G8B8A8_SRGB).setMipmapLevel(1).setUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
        TextureHandle texHandle = renderer->createTexture(texInfo)->handle;
        delete ptr;
        ptr = nullptr;
        return texHandle;
    }
    
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    //renderObject的提取顺序：先renderpass,再加载纹理和顶点缓冲,再设置Uniform buffer,
    //再创建描述符集和描述符集布局,有了这些再去创建管线(后期学习一下管线缓存)
    std::vector<RenderObject> BaseLoader::Execute(const SceneNode& rootNode){
        if(rootNode.m_subNodes.empty()&&rootNode.m_meshIndex==-1) return {};
        if(rootNode.m_meshIndex!=-1){
            Mesh& mesh = m_meshes[rootNode.m_meshIndex];
            RenderObject rj;
            rj.m_renderPass = m_renderer->getContext()->m_swapChainPass;
            if(!mesh.m_diffuseTexturePath.empty()){
                rj.m_diffuseTex = upLoadTextureToGPU(m_renderer, mesh.m_diffuseTexturePath);
            }
            if(!mesh.m_emissiveTexturePath.empty()){
                rj.m_emissiveTex = upLoadTextureToGPU(m_renderer, mesh.m_emissiveTexturePath);
            }
            if(!mesh.m_MRTexturePath.empty()){
                rj.m_MRTtex = upLoadTextureToGPU(m_renderer, mesh.m_MRTexturePath);
            }
            if(!mesh.m_normalTexturePath.empty()){
                rj.m_normalTex = upLoadTextureToGPU(m_renderer, mesh.m_normalTexturePath);
            }
            if(!mesh.m_occlusionTexturePath.empty()){
                rj.m_occlusionTex = upLoadTextureToGPU(m_renderer, mesh.m_occlusionTexturePath);
            }
            //---------+++++++++++++++++++++++++-----------------------
            //DescriptorSetLayoutHandle slHandle = m_renderer->getContext()->createDescriptorSetLayout(descLayoutInfo);
            //---------+++++++++++++++++++++++++-----------------------


            // del with buffer
            rj.m_vertexBuffer = upLoadBufferToGPU(m_renderer, mesh.m_vertices, mesh.name.c_str());
            rj.m_indexBuffer = upLoadBufferToGPU(m_renderer, mesh.m_indeices, mesh.name.c_str());
            rj.m_indexCount = mesh.m_indeices.size();
            rj.m_vertexCount = mesh.m_vertices.size();
            //好像没有卵用;
            rj.m_uniformBuffer = m_renderer->getContext()->m_viewProjectUniformBuffer;

            DescriptorSetLayoutCreateInfo descLayoutInfo;
            descLayoutInfo.reset();
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,0,1,"diffuseTexture"});
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,1,"MRTTexture"});
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2,1,"normalTexture"});
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3,1,"emissiveTexture"});
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,4,1,"occlusionTexture"});
            descLayoutInfo.addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,5,1,"viewProjectUniform"});
            descLayoutInfo.setName("textureAndUniformBufferDesc");
            pipelineCreateInfo pipelineInfo;
            //pipelineInfo.addDescriptorSetlayout(slHandle);
            pipelineInfo.m_renderPassHandle = rj.m_renderPass;
            pipelineInfo.m_depthStencil.setDepth(true, VK_COMPARE_OP_LESS_OR_EQUAL);
            pipelineInfo.m_vertexInput.Attrib = Vertex::getAttributeDescriptions();
            pipelineInfo.m_vertexInput.Binding = Vertex::getBindingDescription();
            pipelineInfo.m_shaderState.reset();
            auto vsCode = readFile("./shaders/vert.spv");
            auto fsCode = readFile("./shaders/frag.spv");
            pipelineInfo.m_shaderState.addStage(vsCode.data(), vsCode.size(), VK_SHADER_STAGE_VERTEX_BIT);
            pipelineInfo.m_shaderState.addStage(fsCode.data(), fsCode.size(), VK_SHADER_STAGE_FRAGMENT_BIT);
            pipelineInfo.m_shaderState.setName("pbrPipeline");
            pipelineInfo.name = pipelineInfo.m_shaderState.name;
            ProgramCreateInfo programInfo{};
            programInfo.pipelineInfo = pipelineInfo;
            programInfo.dslyaoutInfo = descLayoutInfo;
            programInfo.setName(pipelineInfo.name.c_str());
            auto programe = m_renderer->createProgram(programInfo);
            MaterialCreateInfo matInfo{};
            matInfo.setProgram(programe);
            matInfo.name = "pbr_mat";
            auto material = m_renderer->createMaterial(matInfo);
            rj.m_material = material;

            DescriptorSetCreateInfo descInfo;
            descInfo.setName("base descriptor Set");
            descInfo.reset().setLayout(rj.m_material->program->passes[0].descriptorSetLayout).texture(rj.m_diffuseTex, 0).texture(rj.m_MRTtex,1).texture(rj.m_normalTex,2)
            .texture(rj.m_emissiveTex,3).texture(rj.m_occlusionTex,4).buffer(m_renderer->getContext()->m_viewProjectUniformBuffer, 5);
            rj.m_descriptors.push_back(m_renderer->getContext()->createDescriptorSet(descInfo));
            // additional descriptors, like HDR env map or sth;
            //-------
            return {rj};
        }
        else if(!rootNode.m_subNodes.empty()){
            std::vector<RenderObject> renderobjects;
            for(auto& i: rootNode.m_subNodes){
                auto subRenderObjects = Execute(i);
                renderobjects.insert(renderobjects.end(),subRenderObjects.begin(),subRenderObjects.end());
            }
            m_renderObjects = renderobjects;
            return m_renderObjects;
        }
        DG_WARN("There is an empty node in your Scene Tree!");
        return {};
    }

    void BaseLoader::destroy(){
        
    }

    objLoader::objLoader(){
        m_renderer = nullptr;
        m_renderObjects.clear();
        m_meshes.clear();
        m_haveContent = false;
    }

    objLoader::objLoader(Renderer*renderer){
        m_renderer = renderer;
        m_renderObjects.clear();
        m_meshes.clear();
        m_haveContent = false;
    }

    void objLoader::loadFromPath(std::string path){
        tinyobj::ObjReaderConfig reader_config;
        size_t found = path.find_last_of("/\\");
        reader_config.mtl_search_path = path.substr(0,found);
        tinyobj::ObjReader reader;
        if(!reader.ParseFromFile(path, reader_config)){
            if(!reader.Error().empty()){
                DG_ERROR(reader.Error());
            }
            DG_ERROR("Failed to load model named", path.substr(found+1));
            exit(-1);
        }
        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();
        
        SceneNode model;

        for(size_t s = 0;s<shapes.size();++s){
            SceneNode meshNode;
            size_t index_offset = 0;
            std::vector<Vertex> vertices;
            std::vector<u32>    indices;
            for(size_t f = 0;f<shapes[s].mesh.num_face_vertices.size();++f){
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                for(size_t v = 0;v<fv;++v){
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset+v];
                    indices.push_back(indices.size());
                    Vertex vert;
                    vert.pos.x = attrib.vertices[3*size_t(idx.vertex_index)+0];
                    vert.pos.y = attrib.vertices[3*size_t(idx.vertex_index)+1];
                    vert.pos.z = attrib.vertices[3*size_t(idx.vertex_index)+2];
                    if(idx.normal_index>=0){
                        vert.normal.x = attrib.normals[3*size_t(idx.normal_index)+0];
                        vert.normal.y = attrib.normals[3*size_t(idx.normal_index)+1];
                        vert.normal.z = attrib.normals[3*size_t(idx.normal_index)+2];
                    }
                    if(idx.texcoord_index>=0){
                        vert.texCoord.x = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                        vert.texCoord.y = 1.0-attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    }
                    vertices.push_back(vert);
                }
                index_offset += fv;
            }
            Mesh mesh;
            //texture load(For obj file, diffuse tex, specular tex, )
            tinyobj::material_t material = materials[shapes[s].mesh.material_ids[0]];
            std::string basePath = reader_config.mtl_search_path;
            mesh.m_diffuseTexturePath = basePath + '/' + material.diffuse_texname;
            mesh.m_MRTexturePath =material.specular_texname.empty() ? "" : basePath + '/' + material.specular_texname;
            mesh.m_vertices = vertices;
            mesh.m_indeices = indices;
            mesh.name = shapes[s].name;
            meshNode.m_meshIndex = m_meshes.size();
            model.m_subNodes.push_back(meshNode);
            m_meshes.push_back(mesh);
        }
        model.update();
        m_sceneRoot.m_subNodes.push_back(model);
        m_haveContent = true;
    }

    void objLoader::destroy(){
        if(m_renderObjects.empty()) return;
        for(int i = 0;i<m_renderObjects.size();++i){
            auto& rj = m_renderObjects[i];
            for(auto& j:rj.m_descriptors){
                m_renderer->getContext()->DestroyDescriptorSet(j);
            }
        }
    }

    gltfLoader::gltfLoader(){
        m_renderer = nullptr;
    }

    gltfLoader::gltfLoader(Renderer* renderer){
        m_renderer = renderer;
    }

    void gltfLoader::loadFromPath(std::string path){
        
    }

    void gltfLoader::destroy(){

    }
}
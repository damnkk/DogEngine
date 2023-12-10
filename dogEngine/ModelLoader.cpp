#include "ModelLoader.h"
#include "DeviceContext.h"
#include "tiny_obj_loader.h"
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
        std::vector<RenderObject> renderobjects;
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

            // del with buffer
            rj.m_vertexBuffer = upLoadBufferToGPU(m_renderer, mesh.m_vertices, mesh.name.c_str());
            rj.m_indexBuffer = upLoadBufferToGPU(m_renderer, mesh.m_indices, mesh.name.c_str());
            rj.m_indexCount = mesh.m_indices.size();
            rj.m_vertexCount = mesh.m_vertices.size();
            //下面这行好像没有什么卵用;
            rj.m_uniformBuffer = m_renderer->getContext()->m_viewProjectUniformBuffer;
            Material* matPtr = m_renderer->getCurrentMaterial();
            if(matPtr!=nullptr){

                rj.m_material = matPtr;

                DescriptorSetCreateInfo descInfo;
                descInfo.setName("base descriptor Set");
                descInfo.reset().setLayout(rj.m_material->program->passes[0].descriptorSetLayout).texture(rj.m_diffuseTex, 0).texture(rj.m_MRTtex,1).texture(rj.m_normalTex,2)
                .texture(rj.m_emissiveTex,3).texture(rj.m_occlusionTex,4).buffer(m_renderer->getContext()->m_viewProjectUniformBuffer, 5);
                rj.m_descriptors.push_back(m_renderer->getContext()->createDescriptorSet(descInfo));
                // additional descriptors, like HDR env map or sth;
                //-------
                renderobjects.push_back(rj);
            }
        }
        if(!rootNode.m_subNodes.empty()){
            std::vector<RenderObject> subNodeRenderobjects;
            for(auto& i: rootNode.m_subNodes){
                auto subRenderObjects = Execute(i);
                subNodeRenderobjects.insert(subNodeRenderobjects.end(),subRenderObjects.begin(),subRenderObjects.end());
            }
            renderobjects.insert(renderobjects.end(),subNodeRenderobjects.begin(),subNodeRenderobjects.end());
        }
        if(renderobjects.empty()) DG_WARN("There is an empty node in your Scene Tree!");
        m_renderObjects = renderobjects;
        return m_renderObjects;
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
            mesh.m_indices = indices;
            mesh.name = shapes[s].name;
            meshNode.m_meshIndex = m_meshes.size();
            meshNode.m_parentNodePtr= &model;
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

    void gltfLoader::loadNode(tinygltf::Node& inputNode, tinygltf::Model& model, SceneNode* parent){
        parent->m_subNodes.push_back({});
        auto& currNode = parent->m_subNodes.back();
        currNode.m_parentNodePtr = parent;
        if(inputNode.translation.size()==3){
            currNode.m_modelMatrix = glm::translate(currNode.m_modelMatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
        }

        if(inputNode.rotation.size()==4){
            glm::quat q = glm::make_quat(inputNode.rotation.data());
            //currNode.m_modelMatrix *=glm::mat4(q);
        }
    
        if(inputNode.scale.size()==3){
            currNode.m_modelMatrix = glm::scale(currNode.m_modelMatrix,glm::vec3(glm::make_vec3(inputNode.scale.data())));
        }
        if(inputNode.matrix.size()==16){
            currNode.m_modelMatrix = glm::make_mat4x4(inputNode.matrix.data());
        }
        if(inputNode.children.size()>0){
            for(int i = 0;i<inputNode.children.size();++i){
                loadNode(model.nodes[inputNode.children[i]], model, &currNode);
            }
        }

        if(inputNode.mesh>-1){
            const tinygltf::Mesh gltfMesh = model.meshes[inputNode.mesh];
            for(int i = 0;i<gltfMesh.primitives.size();++i){
                currNode.m_subNodes.push_back({});
                auto& primitiveNode = currNode.m_subNodes.back();
                primitiveNode.m_parentNodePtr = &currNode;
                Mesh mesh;
                if(!gltfMesh.name.empty()) mesh.name = gltfMesh.name;
                else mesh.name = "mesh"+std::to_string(m_meshes.size());
                auto& primitive = gltfMesh.primitives[i];
                u32 indexCount = 0;
                //vertices
                {
                    const float* positionBuffer = nullptr;
                    const float* normalBuffer = nullptr;
                    const float* texcoordsBuffer = nullptr;
                    u32 vertexCount = 0;
                    //get the position data
                    if(primitive.attributes.find("POSITION") != primitive.attributes.end()){
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
                        vertexCount = accessor.count;
                    }

                    // get the normal data
                    if(primitive.attributes.find("NORMAL") != primitive.attributes.end()){
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        normalBuffer = reinterpret_cast<float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
                    }

                    //get the texcoord data
                    if(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()){
                        const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
                        texcoordsBuffer = reinterpret_cast<float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
                    }

                    for(size_t v = 0;v<vertexCount;++v){
                        Vertex vt;
                        vt.pos = glm::vec3(glm::make_vec3(&positionBuffer[v*3]));
                        vt.normal = glm::vec3(glm::make_vec3(&normalBuffer[v*3]));
                        vt.texCoord = glm::vec2(glm::make_vec2(&texcoordsBuffer[v*2]));
                        mesh.m_vertices.push_back(vt);
                    }
                }
                //indices
                {
                    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
                    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
                    switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							mesh.m_indices.push_back(buf[index]);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							mesh.m_indices.push_back(buf[index]);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
						for (size_t index = 0; index < accessor.count; index++) {
							mesh.m_indices.push_back(buf[index]);
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
					}
                }

                if(primitive.material>-1){
                    tinygltf::Material& material = model.materials[primitive.material];
                    mesh.m_diffuseTexturePath =model.extras_json_string + model.images[model.textures[material.values["baseColorTexture"].TextureIndex()].source].uri;
                    mesh.m_emissiveTexturePath = material.emissiveTexture.index==-1?"":model.extras_json_string + model.images[model.textures[material.emissiveTexture.index].source].uri;
                    mesh.m_occlusionTexturePath = material.occlusionTexture.index==-1?"":model.extras_json_string+ model.images[model.textures[material.occlusionTexture.index].source].uri;
                    mesh.m_MRTexturePath = material.pbrMetallicRoughness.metallicRoughnessTexture.index ==-1?"":model.extras_json_string+model.images[model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source].uri;
                    mesh.m_normalTexturePath = material.normalTexture.index==-1?"":model.extras_json_string + model.images[model.textures[material.normalTexture.index].source].uri;
                }
                primitiveNode.m_meshIndex = m_meshes.size();
                m_meshes.push_back(mesh);
                m_haveContent = true;
            }
            
        }

    }

    void gltfLoader::loadFromPath(std::string path){
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;
        bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        if(!warn.empty()){
            DG_WARN(warn);
        }
        if(!err.empty()){
            DG_ERROR(err);
        }
        if(!res){
            DG_ERROR("Failed to load gltf model, name: ", path);
            exit(-1);
        }
        u32 found = path.find_last_of("/\\");
        
        model.extras_json_string = path.substr(0, found+1);
        auto& nodes = model.nodes;
        auto& scenes = model.scenes;
        for(int sce = 0;sce<scenes.size();++sce){
            auto& scene = scenes[sce];
            m_sceneRoot.m_subNodes.push_back({});
            m_sceneRoot.m_subNodes.back().m_parentNodePtr = &m_sceneRoot;
            auto& sceneNode = m_sceneRoot.m_subNodes.back();
            for(int nd = 0;nd<scene.nodes.size();++nd){
                auto& inputNode = nodes[scene.nodes[nd]];
                loadNode(inputNode, model, &sceneNode);
            }
        }
    }

    void gltfLoader::destroy(){

    }
}
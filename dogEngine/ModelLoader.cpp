#include "ModelLoader.h"
#include "CommandBuffer.h"
#include "DeviceContext.h"
#include "Renderer.h"
#include "tiny_obj_loader.h"

namespace dg {
std::array<glm::vec3, 2> SceneNode::getAABB() {
  std::array<glm::vec3, 2> temp;
  return temp;
}

void SceneNode::update() {
  m_isUpdate = true;
}

std::vector<char> readFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }
  size_t            fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

//renderObject的提取顺序：先renderpass,再加载纹理和顶点缓冲,再设置Uniform buffer,
//再创建描述符集和描述符集布局,有了这些再去创建管线(后期学习一下管线缓存)
std::vector<RenderObject> BaseLoader::Execute(const SceneNode& rootNode, glm::mat4 modelMatrix) {
  if (rootNode.m_subNodes.empty() && rootNode.m_meshIndex == -1) return {};
  std::vector<RenderObject> renderobjects;
  if (rootNode.m_meshIndex != -1) {
    Mesh&        mesh = m_meshes[rootNode.m_meshIndex];
    RenderObject rj;
    rj.m_renderPass = m_renderer->getContext()->m_gameViewPass;
    rj.m_material = mesh.m_meshMaterial;
    // del with buffer
    rj.m_vertexBuffer = m_renderer->upLoadBufferToGPU(mesh.m_vertices, mesh.name.c_str());
    rj.m_indexBuffer = m_renderer->upLoadBufferToGPU(mesh.m_indices, mesh.name.c_str());
    rj.m_indexCount = mesh.m_indices.size();
    rj.m_vertexCount = mesh.m_vertices.size();
    rj.m_GlobalUniform = m_renderer->getContext()->m_viewProjectUniformBuffer;
    rj.m_modelMatrix = modelMatrix;
    BufferCreateInfo matUniformBufferInfo{};
    std::string      bufferName = mesh.name + "MaterialUniformBuffer";
    matUniformBufferInfo.reset().setName(bufferName.c_str()).setDeviceOnly(false).setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::UniformMaterial));
    rj.m_MaterialUniform = m_renderer->createBuffer(matUniformBufferInfo)->handle;
    Material* matPtr;
    if (mesh.m_meshMaterial) matPtr = mesh.m_meshMaterial;
    else {
      matPtr = m_renderer->getDefaultMaterial();
    }
    matPtr->uniformMaterial.modelMatrix = rj.m_modelMatrix;
    if (matPtr != nullptr) {
      rj.m_material = matPtr;
      rj.m_material->setIblMap(m_renderer, "./models/skybox/small_empty_room_1_2k.hdr");
      DescriptorSetCreateInfo descInfo;
      descInfo.setName("base descriptor Set");
      descInfo.reset().setLayout(matPtr->program->passes[0].descriptorSetLayout[0]);
      descInfo.buffer(rj.m_GlobalUniform, 0).buffer(rj.m_MaterialUniform, 1);
      rj.m_descriptors.push_back(m_renderer->getContext()->createDescriptorSet(descInfo));
      renderobjects.push_back(rj);
    }
  }
  if (!rootNode.m_subNodes.empty()) {
    std::vector<RenderObject> subNodeRenderobjects;
    for (auto& i : rootNode.m_subNodes) {
      auto subRenderObjects = Execute(i, rootNode.m_modelMatrix * modelMatrix);
      subNodeRenderobjects.insert(subNodeRenderobjects.end(), subRenderObjects.begin(), subRenderObjects.end());
    }
    renderobjects.insert(renderobjects.end(), subNodeRenderobjects.begin(), subNodeRenderobjects.end());
  }
  if (renderobjects.empty()) DG_WARN("There is an empty node in your Scene Tree!");
  m_renderObjects = renderobjects;
  return m_renderObjects;
}

void BaseLoader::destroy() {
}

objLoader::objLoader() {
  m_renderer = nullptr;
  m_renderObjects.clear();
  m_meshes.clear();
  m_haveContent = false;
}

objLoader::objLoader(Renderer* renderer) {
  m_renderer = renderer;
  m_renderObjects.clear();
  m_meshes.clear();
  m_haveContent = false;
}

void objLoader::loadFromPath(std::string path) {
  tinyobj::ObjReaderConfig reader_config;
  size_t                   found = path.find_last_of("/\\");
  reader_config.mtl_search_path = path.substr(0, found);
  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(path, reader_config)) {
    if (!reader.Error().empty()) {
      DG_ERROR(reader.Error());
    }
    DG_ERROR("Failed to load model named", path.substr(found + 1));
    exit(-1);
  }
  auto& attrib = reader.GetAttrib();
  auto& shapes = reader.GetShapes();
  auto& materials = reader.GetMaterials();

  SceneNode model;

  for (size_t s = 0; s < shapes.size(); ++s) {
    SceneNode           meshNode;
    size_t              index_offset = 0;
    std::vector<Vertex> vertices;
    std::vector<u32>    indices;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
      size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
      for (size_t v = 0; v < fv; ++v) {
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        indices.push_back(indices.size());
        Vertex vert;
        vert.pos.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
        vert.pos.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
        vert.pos.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
        if (idx.normal_index >= 0) {
          vert.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
          vert.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
          vert.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
        }
        if (idx.texcoord_index >= 0) {
          vert.texCoord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
          vert.texCoord.y = 1.0 - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
        }
        vertices.push_back(vert);
      }
      index_offset += fv;
    }
    Mesh mesh;

    //texture load(For obj file, diffuse tex, specular tex, )
    tinyobj::material_t material = materials[shapes[s].mesh.material_ids[0]];
    std::string         basePath = reader_config.mtl_search_path;
    mesh.m_vertices = vertices;
    mesh.m_indices = indices;
    mesh.name = shapes[s].name;

    MaterialCreateInfo matInfo{};
    matInfo.setName(mesh.name);
    mesh.m_meshMaterial = m_renderer->createMaterial(matInfo);
    mesh.m_meshMaterial->setProgram(m_renderer->getDefaultMaterial()->program);
    std::string       diffuseTexturePath = basePath + '/' + material.diffuse_texname;
    std::string       specularTexturePath = material.specular_texname.empty() ? "" : basePath + '/' + material.specular_texname;
    std::string       normalTexturePath = material.normal_texname.empty() ? "" : basePath + '/' + material.normal_texname;
    TextureCreateInfo texInfo{};
    texInfo.setMipmapLevel(1).setBindLess(true).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setFlag(TextureFlags::Mask::Default_mask);
    mesh.m_meshMaterial->setDiffuseTexture(m_renderer->upLoadTextureToGPU(diffuseTexturePath, texInfo));
    mesh.m_meshMaterial->setMRTexture(m_renderer->upLoadTextureToGPU(specularTexturePath, texInfo));
    mesh.m_meshMaterial->setNormalTexture(m_renderer->upLoadTextureToGPU(normalTexturePath, texInfo));
    meshNode.m_meshIndex = m_meshes.size();
    meshNode.m_parentNodePtr = &model;
    model.m_subNodes.push_back(meshNode);
    m_meshes.push_back(mesh);
  }
  model.update();
  m_sceneRoot.m_subNodes.push_back(model);
  m_haveContent = true;
}

void objLoader::destroy() {
  if (m_renderObjects.empty()) return;
  for (int i = 0; i < m_renderObjects.size(); ++i) {
    auto& rj = m_renderObjects[i];
    for (auto& j : rj.m_descriptors) {
      m_renderer->getContext()->DestroyDescriptorSet(j);
    }
  }
}

gltfLoader::gltfLoader() {
  m_renderer = nullptr;
}

gltfLoader::gltfLoader(Renderer* renderer) {
  m_renderer = renderer;
}

void gltfLoader::loadNode(tinygltf::Node& inputNode, tinygltf::Model& model, SceneNode* parent) {
  parent->m_subNodes.emplace_back();
  auto& currNode = parent->m_subNodes.back();
  currNode.m_parentNodePtr = parent;
  if (inputNode.translation.size() == 3) {
    currNode.m_modelMatrix = glm::translate(currNode.m_modelMatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
  }

  if (inputNode.rotation.size() == 4) {
    glm::quat q = glm::make_quat(inputNode.rotation.data());
    currNode.m_modelMatrix *= glm::mat4(q);
  }

  if (inputNode.scale.size() == 3) {
    currNode.m_modelMatrix = glm::scale(currNode.m_modelMatrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
  }
  if (inputNode.matrix.size() == 16) {
    currNode.m_modelMatrix = glm::make_mat4x4(inputNode.matrix.data());
  }
  if (!inputNode.children.empty()) {
    for (int i = 0; i < inputNode.children.size(); ++i) {
      loadNode(model.nodes[inputNode.children[i]], model, &currNode);
    }
  }

  if (inputNode.mesh > -1) {
    const tinygltf::Mesh gltfMesh = model.meshes[inputNode.mesh];

    for (int i = 0; i < gltfMesh.primitives.size(); ++i) {
      // Mesh mesh;
      auto& primitive = gltfMesh.primitives[i];
      Mesh* mesh;

      currNode.m_subNodes.emplace_back();
      auto& primitiveNode = currNode.m_subNodes.back();
      primitiveNode.m_parentNodePtr = &currNode;
      primitiveNode.m_meshIndex = m_meshes.size();
      m_meshes.emplace_back();
      mesh = &m_meshes.back();
      if (!gltfMesh.name.empty()) mesh->name = gltfMesh.name + std::to_string(m_meshes.size());
      else
        mesh->name = "mesh" + std::to_string(m_meshes.size());
      m_haveContent = true;

      MaterialCreateInfo matInfo{};
      std::string        matName = mesh->name + "material";
      matInfo.setName(matName);
      mesh->m_meshMaterial = m_renderer->createMaterial(matInfo);
      mesh->m_meshMaterial->setProgram(m_renderer->getDefaultMaterial()->program);
      u32 indexCount = 0;
      u32 vertexCount = 0;
      //vertices
      {
        const float* positionBuffer = nullptr;
        const float* normalBuffer = nullptr;
        const float* tangentBuffer = nullptr;
        const float* texcoordsBuffer = nullptr;
        //get the position data
        if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
          const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.find("POSITION")->second];
          const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
          positionBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
          vertexCount = accessor.count;
        }

        // get the normal data
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
          const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
          const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
          normalBuffer = reinterpret_cast<float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
        }

        // get tangent data
        if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
          const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.find("TANGENT")->second];
          const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
          tangentBuffer = reinterpret_cast<float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
        }

        //get the texcoord data
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
          const tinygltf::Accessor&   accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
          const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
          texcoordsBuffer = reinterpret_cast<float*>(&(model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]));
        }

        for (size_t v = 0; v < vertexCount; ++v) {
          Vertex vt;
          vt.pos = glm::vec3(glm::make_vec3(&positionBuffer[v * 3]));
          vt.normal = glm::vec3(glm::make_vec3(&normalBuffer[v * 3]));
          vt.tangent = tangentBuffer ? glm::vec4(glm::make_vec4(&tangentBuffer[v * 4])) : glm::vec4(0.0f);
          vt.texCoord = texcoordsBuffer ? glm::vec2(glm::make_vec2(&texcoordsBuffer[v * 2])) : glm::vec2(0.0f);
          mesh->m_vertices.push_back(vt);
        }
      }
      //indices
      {
        const tinygltf::Accessor&   accessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer = model.buffers[bufferView.buffer];
        switch (accessor.componentType) {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              mesh->m_indices.push_back(buf[index]);
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              mesh->m_indices.push_back(buf[index]);
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
            for (size_t index = 0; index < accessor.count; index++) {
              mesh->m_indices.push_back(buf[index]);
            }
            break;
          }
          default:
            std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
            return;
        }
      }

      if (primitive.material > -1) {
        tinygltf::Material& material = model.materials[primitive.material];
        mesh->m_meshMaterial->uniformMaterial.emissiveFactor = glm::make_vec3(material.emissiveFactor.data());
        mesh->m_meshMaterial->uniformMaterial.baseColorFactor = glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
        mesh->m_meshMaterial->uniformMaterial.mrFactor.x = material.pbrMetallicRoughness.metallicFactor;
        mesh->m_meshMaterial->uniformMaterial.mrFactor.y = material.pbrMetallicRoughness.roughnessFactor;
        std::string       diffuseTexturePath = material.pbrMetallicRoughness.baseColorTexture.index == -1 ? "" : model.extras_json_string + model.images[model.textures[material.values["baseColorTexture"].TextureIndex()].source].uri;
        std::string       emissiveTexturePath = material.emissiveTexture.index == -1 ? "" : model.extras_json_string + model.images[model.textures[material.emissiveTexture.index].source].uri;
        std::string       occlusionTexturePath = material.occlusionTexture.index == -1 ? "" : model.extras_json_string + model.images[model.textures[material.occlusionTexture.index].source].uri;
        std::string       MRTexturePath = material.pbrMetallicRoughness.metallicRoughnessTexture.index == -1 ? "" : model.extras_json_string + model.images[model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source].uri;
        std::string       normalTexturePath = material.normalTexture.index == -1 ? "" : model.extras_json_string + model.images[model.textures[material.normalTexture.index].source].uri;
        TextureCreateInfo texInfo{};
        texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setBindLess(true).setMipmapLevel(1);
        mesh->m_meshMaterial->setDiffuseTexture(m_renderer->upLoadTextureToGPU(diffuseTexturePath, texInfo));
        mesh->m_meshMaterial->setEmissiveTexture(m_renderer->upLoadTextureToGPU(emissiveTexturePath, texInfo));
        mesh->m_meshMaterial->setAoTexture(m_renderer->upLoadTextureToGPU(occlusionTexturePath, texInfo));
        mesh->m_meshMaterial->setMRTexture(m_renderer->upLoadTextureToGPU(MRTexturePath, texInfo));
        texInfo.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
        mesh->m_meshMaterial->setNormalTexture(m_renderer->upLoadTextureToGPU(normalTexturePath, texInfo));
      }
    }
  }
}

void gltfLoader::loadFromPath(std::string path) {
  tinygltf::Model    model;
  tinygltf::TinyGLTF loader;
  std::string        err;
  std::string        warn;
  bool               res = loader.LoadASCIIFromFile(&model, &err, &warn, path);
  if (!warn.empty()) {
    DG_WARN(warn);
  }
  if (!err.empty()) {
    DG_ERROR(err);
  }
  if (!res) {
    DG_ERROR("Failed to load gltf model, name: ", path);
    exit(-1);
  }
  u32 found = path.find_last_of("/\\");

  model.extras_json_string = path.substr(0, found + 1);
  auto& nodes = model.nodes;
  auto& scenes = model.scenes;
  for (int sce = 0; sce < scenes.size(); ++sce) {
    auto& scene = scenes[sce];
    m_sceneRoot.m_subNodes.emplace_back();
    m_sceneRoot.m_subNodes.back().m_parentNodePtr = &m_sceneRoot;
    auto& sceneNode = m_sceneRoot.m_subNodes.back();
    for (int nd = 0; nd < scene.nodes.size(); ++nd) {
      auto& inputNode = nodes[scene.nodes[nd]];
      if (inputNode.mesh >= 0 || (!inputNode.children.empty())) {
        loadNode(inputNode, model, &sceneNode);
      }
    }
  }
}

void gltfLoader::destroy() {
  if (m_renderObjects.empty()) return;
  for (int i = 0; i < m_renderObjects.size(); ++i) {
    auto& rj = m_renderObjects[i];
    for (auto& j : rj.m_descriptors) {
      m_renderer->getContext()->DestroyDescriptorSet(j);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------------

modiMesh ResourceLoader::convertAIMesh(aiMesh* mesh) {
  modiMesh           dgMesh;
  bool               hasTexCoords = mesh->HasTextureCoords(0);
  bool               hasTagent = mesh->HasTangentsAndBitangents();
  std::vector<float> srcVertices;
  std::vector<u32>   srcIndices;

  std::vector<Vertex> vertices;
  for (size_t i = 0; i < mesh->mNumVertices; ++i) {
    const aiVector3D v = mesh->mVertices[i];
    const aiVector3D n = mesh->mNormals[i];
    const aiVector3D t = hasTexCoords ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f);
    const aiVector3D tangent = hasTagent ? mesh->mTangents[i] : aiVector3D(0.0f);

    Vertex vert;
    vert.pos = glm::vec3(v.x, v.y, v.z);
    vert.normal = glm::vec3(n.x, n.y, n.z);
    vert.texCoord = glm::vec2(t.x, 1.0f - t.y);
    vert.tangent = glm::vec4(tangent.x, tangent.y, tangent.z, 1.0f);
    vertices.push_back(vert);
  }
  for (size_t i = 0; i < mesh->mNumFaces; ++i) {
    if (mesh->mFaces[i].mNumIndices != 3) continue;
    for (size_t j = 0; j < mesh->mFaces[i].mNumIndices; ++j) {
      srcIndices.push_back(mesh->mFaces[i].mIndices[j]);
    }
  }
  dgMesh.indexCount = mesh->mNumFaces * 3;
  dgMesh.vertexBufferHandle = m_renderer->upLoadBufferToGPU(vertices, mesh->mName.C_Str());
  dgMesh.indexBufferHandle = m_renderer->upLoadBufferToGPU(srcIndices, mesh->mName.C_Str());
  BufferCreateInfo bufferInfo{};
  std::string      uniformName = std::string(mesh->mName.C_Str()) + std::string("uniform");
  bufferInfo.reset().setName(uniformName.c_str()).setDeviceOnly(false).setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::UniformMaterial));
  dgMesh.matUniformBufferHandle = m_renderer->createBuffer(bufferInfo)->handle;

  return dgMesh;
}

glm::mat4 toMat4(const aiMatrix4x4& from) {
  glm::mat4 to;
  to[0][0] = (float) from.a1;
  to[0][1] = (float) from.b1;
  to[0][2] = (float) from.c1;
  to[0][3] = (float) from.d1;
  to[1][0] = (float) from.a2;
  to[1][1] = (float) from.b2;
  to[1][2] = (float) from.c2;
  to[1][3] = (float) from.d2;
  to[2][0] = (float) from.a3;
  to[2][1] = (float) from.b3;
  to[2][2] = (float) from.c3;
  to[2][3] = (float) from.d3;
  to[3][0] = (float) from.a4;
  to[3][1] = (float) from.b4;
  to[3][2] = (float) from.c4;
  to[3][3] = (float) from.d4;
  return to;
}

void ResourceLoader::traverse(const aiScene* sourceScene, SceneGraph& sceneGraph, aiNode* node, int parent, int level) {
  int newNodeId;
  //sceneGraph.maxLevel = std::max(sceneGraph.maxLevel, level);
  if (node->mName.length != 0) {
    newNodeId = sceneGraph.addNode(parent, level, std::string(node->mName.C_Str()));
  } else {
    newNodeId = sceneGraph.addNode(parent, level);
  }
  for (size_t i = 0; i < node->mNumMeshes; ++i) {
    int newSubNodeID = sceneGraph.addNode(newNodeId, level + 1, std::string(node->mName.C_Str()) + "_Mesh_" + std::to_string(i));
    int mesh = node->mMeshes[i];
    sceneGraph.m_meshMap[newSubNodeID] = m_meshOffset + mesh;
    sceneGraph.m_boundingBoxMap[newSubNodeID] = m_boundingBoxOffset + mesh;
    sceneGraph.m_materialMap[newSubNodeID] = sourceScene->mMeshes[mesh]->mMaterialIndex + m_materialOffset;
    sceneGraph.m_globalTransforms[newSubNodeID] = glm::mat4(1.0f);
    sceneGraph.m_localTransforms[newSubNodeID] = glm::mat4(1.0f);
  }
  sceneGraph.m_globalTransforms[newNodeId] = glm::mat4(1.0f);
  sceneGraph.m_localTransforms[newNodeId] = toMat4(node->mTransformation);
  for (u32 n = 0; n < node->mNumChildren; ++n) {
    traverse(sourceScene, sceneGraph, node->mChildren[n], newNodeId, level + 1);
  }
}

int       EmptyNameCount = 0;
Material* ResourceLoader::convertAIMaterialToDescription(const aiMaterial* aiMat, std::string basePath) {
  MaterialCreateInfo matinfo{};
  matinfo.setName(aiMat->GetName().length == 0 ? ("DefaultMaterialName" + std::to_string(EmptyNameCount)) : std::string(aiMat->GetName().C_Str()));
  if (aiMat->GetName().length == 0) EmptyNameCount += 1;
  Material* matPtr = m_renderer->createMaterial(matinfo);
  Material& mat = *matPtr;
  aiColor4D color;
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_AMBIENT, &color) == AI_SUCCESS) {
    mat.uniformMaterial.emissiveFactor = glm::vec3(color.r, color.g, color.b);
  }
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS) {
    mat.uniformMaterial.baseColorFactor = glm::vec4(color.r, color.g, color.b, color.a);
    if (mat.uniformMaterial.baseColorFactor.w > 1.0f) mat.uniformMaterial.baseColorFactor.w = 1.0f;
    if (glm::length(glm::vec3(color.r, color.g, color.b)) == 0.0f) {
      aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_SPECULAR, &color);
      mat.uniformMaterial.baseColorFactor = glm::vec4(color.r, color.g, color.b, color.a);
    }
  }
  if (aiGetMaterialColor(aiMat, AI_MATKEY_COLOR_EMISSIVE, &color) == aiReturn_SUCCESS) {
    mat.uniformMaterial.emissiveFactor.r += color.r;
    mat.uniformMaterial.emissiveFactor.g += color.g;
    mat.uniformMaterial.emissiveFactor.b += color.b;
  }
  float tmp;
  if (aiGetMaterialFloat(aiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &tmp) == AI_SUCCESS) {
    mat.uniformMaterial.mrFactor.x = tmp;
  }

  if (aiGetMaterialFloat(aiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &tmp) == AI_SUCCESS) {
    mat.uniformMaterial.mrFactor.y = tmp;
  }

  aiString         texPath;
  aiTextureMapping Mapping;
  unsigned int     UVIndex = 0;
  float            Blend = 1.0f;
  aiTextureOp      TextureOp = aiTextureOp_Add;
  aiTextureMapMode TextureMapMode[2] = {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap};
  unsigned int     TextureFlags = 0;

  TextureCreateInfo texInfo{};
  texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setBindLess(true).setMipmapLevel(1);

  if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
    std::string diffusePath = basePath + '/' + std::string(texPath.C_Str());
    mat.setDiffuseTexture(m_renderer->upLoadTextureToGPU(diffusePath, texInfo));
  }

  if (aiMat->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
    std::string emissivePath = basePath + '/' + std::string(texPath.C_Str());
    mat.setEmissiveTexture(m_renderer->upLoadTextureToGPU(emissivePath, texInfo));
  }

  if (aiMat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS) {
    std::string mrPath = basePath + '/' + std::string(texPath.C_Str());
    mat.setMRTexture(m_renderer->upLoadTextureToGPU(mrPath, texInfo));
  }

  if (aiMat->GetTexture(aiTextureType_LIGHTMAP, 0, &texPath) == AI_SUCCESS) {
    std::string aoPath = basePath + '/' + std::string(texPath.C_Str());
    mat.setAoTexture(m_renderer->upLoadTextureToGPU(aoPath, texInfo));
  }

  texInfo.setFormat(VK_FORMAT_R8G8B8A8_UNORM);
  if (aiMat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
    std::string normalPath = basePath + '/' + std::string(texPath.C_Str());
    mat.setNormalTexture(m_renderer->upLoadTextureToGPU(normalPath, texInfo));
  }

  mat.setProgram(m_renderer->getDefaultMaterial()->program);
  mat.setIblMap(m_renderer, "./models/skybox/graveyard_pathways_2k.hdr");
  return matPtr;
}

SceneGraph ResourceLoader::loadModel(std::string modelPath) {
  if (!m_sceneGraph) {
    m_sceneGraph = std::make_shared<SceneGraph>();
    int rootNodeId = m_sceneGraph->addNode(-1, 0, "RootSceneNode");
  }
  const size_t      pathSeparator = modelPath.find_last_of("/\\");
  const std::string basePath = (pathSeparator != std::string::npos) ? modelPath.substr(0, pathSeparator + 1) : "";
  if (basePath.empty()) DG_WARN("Model base path is null, this may lead to potential mistake");
  const unsigned int loadFlags = 0 | aiProcess_GenNormals | aiProcess_GenBoundingBoxes | aiProcess_FindInvalidData;
  DG_INFO("Loading model from path {}", modelPath);
  const aiScene* scene = m_modelImporter.ReadFile(modelPath.c_str(), loadFlags);
  if (!scene || !scene->HasMeshes()) {
    DG_WARN("This model maybe invalid or have no meshes in the scene");
    return {};
  }

  {
    this->m_meshOffset = m_meshes.size();
    this->m_materialOffset = m_materials.size();
    this->m_boundingBoxOffset = m_boundingBoxes.size();
  }

  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    modiMesh mesh = convertAIMesh(scene->mMeshes[i]);
    m_meshes.push_back(mesh);
    auto& AABB = scene->mMeshes[i]->mAABB;
    m_boundingBoxes.emplace_back(glm::vec3(AABB.mMax.x, AABB.mMax.y, AABB.mMax.z), glm::vec3(AABB.mMin.x, AABB.mMin.y, AABB.mMin.z));
  }

  for (u32 i = 0; i < scene->mNumMaterials; ++i) {
    aiMaterial* assimpMat = scene->mMaterials[i];
    Material*   mat = convertAIMaterialToDescription(assimpMat, basePath);
    m_materials.push_back(mat);
  }
  //SceneGraph graph;

  traverse(scene, *m_sceneGraph, scene->mRootNode, 0, 1);
  return {};
}

void ResourceLoader::loadSceneGraph(std::string sceneGraphPath) {
}

void ResourceLoader::executeScene(std::shared_ptr<SceneGraph> scene) {
  if (!scene) {
    if (!m_sceneGraph) {
      DG_WARN("An invalide scene graph ptr was input,check again");
      return;
    } else {
      scene = m_sceneGraph;
    }
  }
  for (const auto& c : scene->m_meshMap) {
    auto material = scene->m_materialMap.find(c.first);
    if (material == scene->m_materialMap.end()) {
      continue;
    }
    Material* mat = m_materials[material->second];
    //create descriptor set here
    DescriptorSetCreateInfo setInfo{};
    setInfo.reset().setName(scene->m_nodeNames[scene->m_nameForNodeMap[c.first]].c_str()).setLayout(mat->program->passes[0].descriptorSetLayout[0]).buffer(m_renderer->getContext()->m_viewProjectUniformBuffer, 0).buffer(m_meshes[c.second].matUniformBufferHandle, 1);
    DescriptorSetHandle desc = m_renderer->getContext()->createDescriptorSet(setInfo);
    m_renderObjects.push_back(modiRenderObject{
        c.second,
        c.first,
        m_meshes[c.second].indexCount,
        m_materials[material->second],
        {desc},
        m_renderer->getContext()->m_gameViewPass,
        m_meshes[c.second].vertexBufferHandle,
        m_meshes[c.second].indexBufferHandle,
        m_renderer->getContext()->m_viewProjectUniformBuffer,
        m_meshes[c.second].matUniformBufferHandle});
  }
  scene->recalculateAllTransforms();
}
}// namespace dg
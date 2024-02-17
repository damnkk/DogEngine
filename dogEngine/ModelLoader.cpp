#include "ModelLoader.h"
#include "CommandBuffer.h"
#include "DeviceContext.h"
#include "Renderer.h"
#include "tiny_obj_loader.h"

#include "glm/gtx/euler_angles.hpp"

namespace dg {

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

Mesh ResourceLoader::convertAIMesh(aiMesh* mesh) {
  Mesh               dgMesh;
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
  dgMesh.vertexBufferHandle = m_renderer->upLoadVertexDataToGPU(vertices, mesh->mName.C_Str());
  dgMesh.indexBufferHandle = m_renderer->upLoadVertexDataToGPU(srcIndices, mesh->mName.C_Str());
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
  glm::vec3& rotateRecord = sceneGraph.m_rotateRecord[newNodeId];
  glm::extractEulerAngleXYZ(sceneGraph.m_localTransforms[newNodeId], rotateRecord.x, rotateRecord.y, rotateRecord.z);

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
  texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setBindLess(true).setMipmapLevel(10);

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
    Mesh mesh = convertAIMesh(scene->mMeshes[i]);
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
    m_renderObjects.push_back(RenderObject{
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
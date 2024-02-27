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

ResourceLoader::ResourceLoader(Renderer* renderer) : m_renderer(renderer) {
  m_rtBuilder.setup(renderer->getContext().get());
}

void ResourceLoader::destroy() {
  m_rtBuilder.destroy();
}

Mesh ResourceLoader::convertAIMesh(aiMesh* mesh) {
  Mesh               dgMesh;
  bool               hasTexCoords = mesh->HasTextureCoords(0);
  bool               hasTagent = mesh->HasTangentsAndBitangents();
  std::vector<float> srcVertices;
  std::vector<u32>   srcIndices;

  std::vector<Vertex> vertices;
  vertices.reserve(mesh->mNumVertices);
  for (int i = 0; i < mesh->mNumVertices; ++i) {
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
  VkBufferUsageFlags rayTracingFlags = 0;
  VkBufferUsageFlags flag = 0;
  if (m_renderer->getContext()->m_supportRayTracing) {
    flag = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    rayTracingFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }
  dgMesh.indexCount = mesh->mNumFaces * 3;
  dgMesh.vertexBuffer = m_renderer->upLoadVertexDataToGPU(vertices, mesh->mName.C_Str(), flag | rayTracingFlags);
  dgMesh.indexBuffer = m_renderer->upLoadVertexDataToGPU(srcIndices, mesh->mName.C_Str(), flag | rayTracingFlags);
  BufferCreateInfo bufferInfo{};
  std::string      uniformName = std::string(mesh->mName.C_Str()) + std::string("uniform");
  bufferInfo.reset().setName(uniformName.c_str()).setDeviceOnly(false).setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::UniformMaterial));
  dgMesh.matUniformBuffer = m_renderer->createBuffer(bufferInfo)->handle;
  std::vector<u32> primitiveMaterialIndex(mesh->mNumFaces, mesh->mMaterialIndex);
  std::string      name("primitiveMaterialIndex");
  bufferInfo.reset().setName((uniformName + name).c_str()).setDeviceOnly(true).setUsageSize(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | flag, sizeof(u32) * primitiveMaterialIndex.size()).setData(primitiveMaterialIndex.data());
  dgMesh.primitiveMaterialIndexBuffer = m_renderer->createBuffer(bufferInfo)->handle;
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
    setInfo.reset().setName(scene->m_nodeNames[scene->m_nameForNodeMap[c.first]].c_str()).setLayout(mat->program->passes[0].descriptorSetLayout[0]).buffer(m_renderer->getContext()->m_viewProjectUniformBuffer, 0).buffer(m_meshes[c.second].matUniformBuffer, 1);
    DescriptorSetHandle desc = m_renderer->getContext()->createDescriptorSet(setInfo);
    m_renderObjects.push_back(RenderObject{
        c.second,
        c.first,
        m_meshes[c.second].indexCount,
        m_materials[material->second],
        {desc},
        m_renderer->getContext()->m_gameViewPass,
        m_meshes[c.second].vertexBuffer,
        m_meshes[c.second].indexBuffer,
        m_renderer->getContext()->m_viewProjectUniformBuffer,
        m_meshes[c.second].matUniformBuffer});
  }
  scene->markAsChanged(0);
  scene->recalculateAllTransforms();

  executeSceneRT(scene);
}

inline VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix) {
  glm::mat4            temp = glm::transpose(matrix);
  VkTransformMatrixKHR outMatrix;
  memcpy(&outMatrix, &temp, sizeof(VkTransformMatrixKHR));
  return outMatrix;
}

void ResourceLoader::executeSceneRT(std::shared_ptr<SceneGraph> scene) {
  std::vector<Material::UniformMaterial> rtMaterials;
  BufferHandle                           materialArray;
  {
    for (int i = 0; i < m_materials.size(); ++i) {

      Material::UniformMaterial mat;
      mat = m_materials[i]->uniformMaterial;
      for (auto& tex : m_materials[i]->textureMap) {
        mat.textureIndices[tex.second.bindIdx] = {(int) tex.second.texture.index};
      }
      rtMaterials.push_back(mat);
    }
    BufferCreateInfo materialArrayBufferCI{};
    materialArrayBufferCI.reset().setDeviceOnly(true).setUsageSize(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizeof(Material::UniformMaterial) * rtMaterials.size()).setName("Material array").setData(rtMaterials.data());
    materialArray = m_renderer->createBuffer(materialArrayBufferCI)->handle;
  }

  std::vector<MeshDescRT> meshDescRts;
  BufferHandle            meshDescArray;

  //---------------create blas -- mesh to vkGeometryKHR----------------
  std::vector<RayTracingBuilder::BlasInput> allBlasInput;
  allBlasInput.reserve(m_meshes.size());
  {
    for (int i = 0; i < m_meshes.size(); ++i) {
      // get meshDesc
      auto                      context = m_renderer->getContext();
      Buffer*                   vertexBuffer = context->accessBuffer(m_meshes[i].vertexBuffer);
      Buffer*                   indexBuffer = context->accessBuffer(m_meshes[i].indexBuffer);
      Buffer*                   primitiveMatBuffer = context->accessBuffer(m_meshes[i].primitiveMaterialIndexBuffer);
      Buffer*                   matArray = context->accessBuffer(materialArray);
      MeshDescRT                meshDesc;
      VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
      addressInfo.buffer = vertexBuffer->m_buffer;
      meshDesc.vertexAddress = vkGetBufferDeviceAddress(context->m_logicDevice, &addressInfo);
      addressInfo.buffer = indexBuffer->m_buffer;
      meshDesc.indexAddress = vkGetBufferDeviceAddress(context->m_logicDevice, &addressInfo);
      addressInfo.buffer = primitiveMatBuffer->m_buffer;
      meshDesc.primitiveMatIdxAddress = vkGetBufferDeviceAddress(context->m_logicDevice, &addressInfo);
      addressInfo.buffer = matArray->m_buffer;
      meshDesc.materialAddress = vkGetBufferDeviceAddressKHR(context->m_logicDevice, &addressInfo);
      meshDescRts.push_back(meshDesc);

      //get BlasInput
      int                                             maxPrimitiveCount = m_meshes[i].indexCount / 3;
      VkAccelerationStructureGeometryTrianglesDataKHR triangles{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
      triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
      triangles.vertexData.deviceAddress = meshDesc.vertexAddress;
      triangles.vertexStride = sizeof(Vertex);
      triangles.indexType = VK_INDEX_TYPE_UINT32;
      triangles.indexData.deviceAddress = meshDesc.indexAddress;
      triangles.maxVertex = m_meshes[i].indexCount - 1;

      VkAccelerationStructureGeometryKHR asGeom{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
      asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      asGeom.geometry.triangles = triangles;
      VkAccelerationStructureBuildRangeInfoKHR offset;
      offset.primitiveOffset = 0;
      offset.primitiveCount = maxPrimitiveCount;
      offset.transformOffset = 0;
      offset.firstVertex = 0;
      RayTracingBuilder::BlasInput input;
      input.asGeometry.emplace_back(asGeom);
      input.asBuildOffsetInfo.emplace_back(offset);
      allBlasInput.push_back(input);
    }
    BufferCreateInfo meshDescRtsBufferCI{};
    meshDescRtsBufferCI.reset().setDeviceOnly(true).setUsageSize(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, sizeof(MeshDescRT) * meshDescRts.size()).setName("MeshDescArray").setData(meshDescRts.data());
    meshDescArray = m_renderer->createBuffer(meshDescRtsBufferCI)->handle;
  }

  m_rtBuilder.buildBlas(allBlasInput, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);

  //build tlas
  std::vector<VkAccelerationStructureInstanceKHR> tlas;
  tlas.reserve(m_renderObjects.size());
  for (const auto& c : scene->m_meshMap) {
    VkAccelerationStructureInstanceKHR rayInst{};
    rayInst.transform = toTransformMatrixKHR(scene->getGlobalTransformsFromIdx(c.first));
    rayInst.instanceCustomIndex = c.second;
    rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(c.second);
    rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    rayInst.mask = 0xFF;
    rayInst.instanceShaderBindingTableRecordOffset = 0;
    tlas.emplace_back(rayInst);
  }
  m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
}
}// namespace dg
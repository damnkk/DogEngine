#include "Renderer.h"
#include "CommandBuffer.h"
#include "GUI/gui.h"
#include "ModelLoader.h"
#include "dgGeometry.h"
#include "dgShaderCompiler.h"
#include "ktx.h"
#include "ktxvulkan.h"

namespace dg {

#define RDOC_CATCH_START renderer->getContext()->m_renderDoc_api->StartFrameCapture(NULL, NULL);
#define RDOC_CATCH_END   renderer->getContext()->m_renderDoc_api->EndFrameCapture(NULL, NULL);

const std::string TextureResource::k_type = "texture_resource";
const std::string BufferResource::k_type = "buffer_resource";
const std::string SamplerResource::k_type = "sampler_resource";

u32 CalcuMipMap(const int& width, const int& height) {
  return std::floor(std::log2(std::max(width, height))) + 1;
}

void ResourceCache::destroy(Renderer* renderer) {
  for (auto it = textures.begin(); it != textures.end(); ++it) {
    if (!it->second) {
      DG_WARN("An invalid TextureResource pointer");
      return;
    }
    renderer->getContext()->DestroyTexture(it->second->handle);
    renderer->getTextures().release(it->second);
  }
  textures.clear();

  for (auto it = samplers.begin(); it != samplers.end(); ++it) {
    if (!it->second) {
      DG_WARN("An invalid SamplerResource pointer");
      return;
    }
    renderer->getContext()->DestroySampler(it->second->handle);
    renderer->getSamplers().release(it->second);
  }
  samplers.clear();

  for (auto it = buffers.begin(); it != buffers.end(); ++it) {
    if (!it->second) {
      DG_WARN("An invalid bufferResource pointer");
      return;
    }
    renderer->getContext()->DestroyBuffer(it->second->handle);
    renderer->getBuffers().release(it->second);
  }
  buffers.clear();

  for (auto it = materials.begin(); it != materials.end(); ++it) {
    if (!it->second) {
      DG_WARN("An invalid Material pointer");
      return;
    }
    renderer->getMaterials().release(it->second);
  }
  materials.clear();
}

TextureResource* ResourceCache::queryTexture(const std::string& name) {
  auto find = this->textures.find(name);
  if (find != this->textures.end()) {
    return find->second;
  }
  return nullptr;
}

SamplerResource* ResourceCache::querySampler(const std::string& name) {
  auto find = this->samplers.find(name);
  if (find != this->samplers.end()) {
    return find->second;
  }
  return nullptr;
}

BufferResource* ResourceCache::QueryBuffer(const std::string& name) {
  auto find = this->buffers.find(name);
  if (find != this->buffers.end()) {
    return find->second;
  }
  return nullptr;
}

Material* ResourceCache::QueryMaterial(const std::string& name) {
  auto find = this->materials.find(name);
  if (find != this->materials.end()) {
    return find->second;
  }
  return nullptr;
}

void Renderer::makeDefaultMaterial() {
  MaterialCreateInfo matCI{};
  matCI.setName("defaultMat");
  m_defaultMaterial = createMaterial(matCI);
  DescriptorSetLayoutCreateInfo descInfo = m_context->m_defaultSetLayoutCI;
  descInfo.addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, "MaterialUniformBuffer"});
  descInfo.setName("defaultDescLayout");
  DescriptorSetLayoutHandle descLayout = m_context->createDescriptorSetLayout(descInfo);
  PipelineCreateInfo        pipelineInfo{};

  //pipelineInfo.m_BlendState.add_blend_state().setColor(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD);
  pipelineInfo.m_renderPassHandle = m_context->m_gameViewPass;
  pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineInfo.m_vertexInput.Attrib = Vertex::getAttributeDescriptions();
  pipelineInfo.m_vertexInput.Binding = Vertex::getBindingDescription();
  pipelineInfo.m_shaderState.reset();
  pipelineInfo.m_shaderState.addStage("./shaders/vspbr.vert", VK_SHADER_STAGE_VERTEX_BIT);
  pipelineInfo.m_shaderState.addStage("./shaders/fspbr.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineInfo.m_shaderState.setName("defaultPipeline");
  pipelineInfo.name = pipelineInfo.m_shaderState.name;
  pipelineInfo.addDescriptorSetlayout(descLayout);
  pipelineInfo.addDescriptorSetlayout(m_context->m_bindlessDescriptorSetLayout);
  m_defaultMaterial->setProgram(createProgram("defaultPbrProgram", {pipelineInfo}));
  m_defaultMaterial->setIblMap(this, "./models/skybox/graveyard_pathways_2k.hdr");
}

void Renderer::init(std::shared_ptr<DeviceContext> context) {
  m_context = context;
  m_camera = std::make_shared<Camera>();
  m_camera->getAspect() = (float) m_context->m_swapChainWidth / (float) m_context->m_swapChainHeight;
  ShaderCompiler::init();
  m_buffers.init(40960);
  m_textures.init(4096);
  m_samplers.init(4096);
  m_materials.init(4096);
  // m_objLoader = std::make_shared<objLoader>(this);
  // m_gltfLoader = std::make_shared<gltfLoader>(this);
  m_resourceLoader = std::make_shared<ResourceLoader>(this);
  GUI::getInstance().init(this);
  makeDefaultMaterial();
}

void Renderer::destroy() {
  vkDeviceWaitIdle(m_context->m_logicDevice);
  GUI::getInstance().Destroy();
  ShaderCompiler::destroy();
  m_resourceCache.destroy(this);
  m_textures.destroy();
  m_buffers.destroy();
  m_materials.destroy();
  m_samplers.destroy();
  destroySkyBox();
  m_context->Destroy();
}

TextureResource* Renderer::createTexture(TextureCreateInfo& textureInfo) {
  if (textureInfo.name.empty()) {
    DG_ERROR("Sampled texture object must have an unique name");
    exit(-1);
  }
  auto findRes = m_resourceCache.textures.find(textureInfo.name);
  if (m_resourceCache.textures.find(textureInfo.name) != m_resourceCache.textures.end()) {
    DG_INFO("There is a Texture with the same name:{} in resourceCache, renderer will reuse it", textureInfo.name);
    return findRes->second;
  }
  textureInfo.setMipmapLevel(std::min(textureInfo.m_mipmapLevel, CalcuMipMap(textureInfo.m_textureExtent.width, textureInfo.m_textureExtent.height)));
  auto textureRes = m_textures.obtain();
  textureRes->name = textureInfo.name;
  textureRes->handle = m_context->createTexture(textureInfo);
  if (textureInfo.m_mipmapLevel > 1) {
    SamplerCreateInfo sampleInfo{};
    sampleInfo.set_min_mag_mip(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR).set_LodMinMaxBias(0, (float) textureInfo.m_mipmapLevel, 0).set_address_uvw(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT).set_name(textureInfo.name.c_str());
    auto*    sampleResource = this->createSampler(sampleInfo);
    Texture* tex = m_context->accessTexture(textureRes->handle);
    tex->setSampler(this->m_context, sampleResource->handle);
  }
  m_resourceCache.textures[textureInfo.name] = textureRes;
  return textureRes;
}

BufferResource* Renderer::createBuffer(BufferCreateInfo& bufferInfo) {
  if (bufferInfo.name.empty()) {
    DG_ERROR("Buffer created by renderer must have an unique name");
    exit(-1);
  }
  auto find = m_resourceCache.buffers.find(bufferInfo.name);
  if (find != m_resourceCache.buffers.end()) {
    DG_WARN("You are trying to create a buffer which has a same name{} with an existing buffer, renderer will reuse it", bufferInfo.name);
    return find->second;
  }
  auto bufferRes = m_buffers.obtain();
  bufferRes->name = bufferInfo.name;
  bufferRes->handle = m_context->createBuffer(bufferInfo);
  m_resourceCache.buffers[bufferInfo.name] = bufferRes;
  return bufferRes;
}

SamplerResource* Renderer::createSampler(SamplerCreateInfo& samplerInfo) {
  if (samplerInfo.name.empty()) {
    DG_ERROR("Sampler created by renderer must have an unique name")
    exit(-1);
  }
  auto findRes = m_resourceCache.samplers.find(samplerInfo.name);
  if (findRes != m_resourceCache.samplers.end()) {
    DG_INFO("There is a Sampler with the same name:{} in resourceCache, renderer will reuse it", samplerInfo.name);
    return findRes->second;
  }
  auto samplerRes = m_samplers.obtain();
  samplerRes->name = samplerInfo.name;
  samplerRes->handle = m_context->createSampler(samplerInfo);
  m_resourceCache.samplers[samplerInfo.name] = samplerRes;
  return samplerRes;
}

Material* Renderer::createMaterial(MaterialCreateInfo& matInfo) {
  if (matInfo.name.empty()) {
    DG_ERROR("Material created by renderer must have an unique name");
    exit(-1);
  }
  auto findRes = m_resourceCache.materials.find(matInfo.name);
  if (findRes != m_resourceCache.materials.end()) {
    DG_INFO("There is a material with the same name in resourceCache, renderer will reuse it")
    return findRes->second;
  }
  auto material = m_materials.obtain();
  material->name = matInfo.name;
  material->renderOrder = matInfo.renderOrder;
  material->renderer = this;
  m_resourceCache.materials[material->name] = material;
  return material;
}

std::shared_ptr<Program> Renderer::createProgram(const std::string& name, ProgramPassCreateInfo progPassCI) {
  std::shared_ptr<Program> pg = std::make_shared<Program>(m_context, name);
  u32                      numPasses = 1;
  pg->passes.resize(numPasses);
  for (int i = 0; i < numPasses; ++i) {
    ProgramPass& pass = pg->passes[i];
    pass.pipeline = m_context->createPipeline(progPassCI.pipelineInfo);
    for (auto& layoutHandle : progPassCI.pipelineInfo.m_descLayout) {
      pass.descriptorSetLayout.emplace_back(layoutHandle);
    }
  }
  return pg;
}

void Renderer::destroyBuffer(BufferResource* bufRes) {
  if (!bufRes) {
    DG_WARN("An invalid bufferResource pointer");
    return;
  }
  auto it = m_resourceCache.buffers.find(bufRes->name);
  if (it != m_resourceCache.buffers.end()) {
    m_resourceCache.buffers.erase(it);
  }
  m_context->DestroyBuffer(bufRes->handle);
  m_buffers.release(bufRes);
}

void Renderer::destroyTexture(TextureResource* texRes) {
  if (!texRes) {
    DG_WARN("An invalid TextureResource pointer");
    return;
  }
  auto it = m_resourceCache.textures.find(texRes->name);
  if (it != m_resourceCache.textures.end()) {
    m_resourceCache.textures.erase(it);
  }
  m_context->DestroyTexture(texRes->handle);
  m_textures.release(texRes);
}

void Renderer::destroySampler(SamplerResource* samplerRes) {
  if (!samplerRes) {
    DG_WARN("An invalid SamplerResource pointer");
    return;
  }
  auto it = m_resourceCache.samplers.find(samplerRes->name);
  if (it != m_resourceCache.samplers.end()) {
    m_resourceCache.samplers.erase(it);
  }
  m_context->DestroySampler(samplerRes->handle);
  m_samplers.release(samplerRes);
}

void Renderer::destroyMaterial(Material* material) {
  if (!material) {
    DG_WARN("An invalid Material pointer");
    return;
  }
  auto it = m_resourceCache.materials.find(material->name);
  if (it != m_resourceCache.materials.end()) {
    m_resourceCache.materials.erase(it);
  }
  m_materials.release(material);
}

double oldTimeStamp = 0.0f;
void   Renderer::newFrame() {
  double newTimeStamp = glfwGetTime();
  deltaTime = newTimeStamp - oldTimeStamp;
  oldTimeStamp = newTimeStamp;
  GUI::getInstance().eventListen();
  m_context->newFrame();
  GUI::getInstance().newGUIFrame();
  m_resourceLoader->getSceneGraph()->recalculateAllTransforms();
}

void Renderer::present() {
  m_context->present();
}

void Renderer::loadModel(const std::string& path) {
  if (path.empty()) {
    DG_WARN("Invalid resource path");
    return;
  }
  m_resourceLoader->loadModel(path);
}

void Renderer::executeScene() {
  if (have_skybox) executeSkyBox();
  m_resourceLoader->executeScene(nullptr);
  m_renderObjects.insert(m_renderObjects.end(), m_resourceLoader->getRenderObjects().begin(), m_resourceLoader->getRenderObjects().end());
}

void Renderer::drawScene() {
  CommandBuffer* cmd = m_context->getCommandBuffer(QueueType::Enum::Graphics, true);
  cmd->pushMark("DrawScene");
  Rect2DInt scissor{0, 0, (u16) m_context->m_gameViewWidth, (u16) m_context->m_gameViewHeight};
  cmd->setScissor(&scissor);
  ViewPort viewPort{
      .rect{
          .x = 0.0f,
          .y = 0.0f,
          .width = (float) scissor.width,
          .height = (float) scissor.height},
      .min_depth = 0.0f,
      .max_depth = 1.0f};
  cmd->setViewport(&viewPort);
  for (int i = 0; i < m_renderObjects.size(); ++i) {
    auto& currRenderObject = m_renderObjects[i];
    cmd->bindPass(currRenderObject.renderpass);
    cmd->bindPipeline(currRenderObject.materialPtr->program->passes[0].pipeline);
    cmd->bindVertexBuffer(currRenderObject.vertexBuffer, 0, 0);
    cmd->bindIndexBuffer(currRenderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdSetDepthTestEnable(cmd->m_commandBuffer, true);

    Buffer* globalUniformBuffer = m_context->accessBuffer(currRenderObject.globalUniform);
    if (!globalUniformBuffer) {
      DG_WARN("Invalid camera view Buffer");
      continue;
    }
    void* data;
    vmaMapMemory(m_context->m_vma, globalUniformBuffer->m_allocation, &data);
    UniformData udata{};
    udata.cameraPos = m_camera->getPosition();
    udata.cameraDirectory = m_camera->getDirectVector();
    udata.projectMatrix = m_camera->getProjectMatrix();
    udata.viewMatrix = m_camera->getViewMatrix();
    memcpy(data, &udata, sizeof(UniformData));
    vmaUnmapMemory(m_context->m_vma, globalUniformBuffer->m_allocation);
    Material::UniformMaterial um{};
    um = currRenderObject.materialPtr->uniformMaterial;
    um.modelMatrix = m_resourceLoader->getSceneGraph()->getGlobalTransformsFromIdx(currRenderObject.transformIndex);
    for (auto& [first, second] : currRenderObject.materialPtr->textureMap) {
      um.textureIndices[second.bindIdx] = {(int) second.texture.index};
    }
    Buffer* materialBuffer = m_context->accessBuffer(currRenderObject.materialUniform);
    vmaMapMemory(m_context->m_vma, materialBuffer->m_allocation, &data);
    memcpy(data, &um, sizeof(Material::UniformMaterial));
    vmaUnmapMemory(m_context->m_vma, materialBuffer->m_allocation);

    cmd->bindDescriptorSet(currRenderObject.descriptors, 0, nullptr, 0);
    cmd->bindDescriptorSet({m_context->m_bindlessDescriptorSet}, 1, nullptr, 0);
    cmd->drawIndexed(TopologyType::Enum::Triangle, currRenderObject.vertexIndicesCount, 1, 0, 0, 0);
  }
  cmd->popMark();
  this->m_context->queueCommandBuffer(cmd);
}

void Renderer::setDefaultMaterial(Material* mat) {
  if (!mat) {
    DG_WARN("You are implementing a invalid material as current material, check again");
    return;
  }
  m_defaultMaterial = mat;
}

Material* Renderer::getDefaultMaterial() {
  return m_defaultMaterial;
}

void Renderer::drawUI() {
  //之后可以在这里补充一些其他的界面逻辑
  //这里UI绘制和主场景绘制的commandbuffer共用,效果不好,回头单独给GUI创建一个commandbufferpool,分开录制。
  GUI::getInstance().OnGUI();
  GUI::getInstance().endGUIFrame();
  auto cmd = GUI::getInstance().getCurrentFramCb();
  cmd->begin();
  cmd->bindPass(this->m_context->m_swapChainPass);
  auto drawData = GUI::getInstance().GetDrawData();
  ImGui_ImplVulkan_RenderDrawData(drawData, cmd->m_commandBuffer);
  //m_context->queueCommandBuffer(cmd);
  cmd->endpass();
  cmd->end();
}

TextureHandle Renderer::upLoadTextureToGPU(std::string& texPath, TextureCreateInfo& texInfo) {
  if (texPath.empty()) return {k_invalid_index};
  auto found = texPath.find_last_of('.');
  if (texPath.substr(found + 1) == "ktx") {
    ktxTexture* ktxTexture;
    ktxResult   res = ktxTexture_CreateFromNamedFile(texPath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
    DGASSERT(res == KTX_SUCCESS);
    TextureCreateInfo texCI{};
    texCI.setData(ktxTexture).setName("skyTexture").setFormat(VK_FORMAT_R16G16B16A16_SFLOAT).setFlag(TextureFlags::Mask::Default_mask).setMipmapLevel(ktxTexture->numLevels).setExtent({ktxTexture->baseWidth, ktxTexture->baseHeight, 1}).setBindLess(true);
    texCI.m_imageType = TextureType::Enum::TextureCube;
    TextureHandle handle = createTexture(texCI)->handle;
    ktxTexture_Destroy(ktxTexture);
    ktxTexture = nullptr;
    return handle;
  }
  u32         foundHead = texPath.find_last_of("/\\");
  u32         foundEnd = texPath.find_last_not_of(".");
  std::string texName = texPath.substr(foundHead + 1, foundEnd);
  auto*       resource = this->m_resourceCache.queryTexture(texName);
  if (resource) {
    return resource->handle;
  }
  int      imgWidth, imgHeight, imgChannel;
  stbi_uc* ptr = stbi_load(texPath.c_str(), &imgWidth, &imgHeight, &imgChannel, 4);
  texInfo.setName(texName.c_str()).setData(ptr).setExtent({(u32) imgWidth, (u32) imgHeight, 1}).setMipmapLevel(std::min(texInfo.m_mipmapLevel, CalcuMipMap(imgWidth, imgHeight)));
  TextureHandle texHandle = createTexture(texInfo)->handle;
  if (texInfo.m_mipmapLevel > 1) {
    SamplerCreateInfo samplerCI{};
    samplerCI.set_name(texName.c_str()).set_min_mag_mip(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR).set_LodMinMaxBias(0.0f, (float) (texInfo.m_mipmapLevel), 0.0f);
    auto*    samplerPtr = createSampler(samplerCI);
    Texture* tex = this->m_context->accessTexture(texHandle);
    tex->setSampler(this->m_context, samplerPtr->handle);
  }
  delete ptr;
  ptr = nullptr;
  return texHandle;
}

void Renderer::addImageBarrier(VkCommandBuffer cmdBuffer, Texture* texture, ResourceState oldState, ResourceState newState, u32 baseMipLevel, u32 mipCount, bool isDepth) {
  this->m_context->addImageBarrier(cmdBuffer, texture, oldState, newState, baseMipLevel, mipCount, isDepth);
}

void Renderer::executeSkyBox() {
  if (m_skyTexture.index == k_invalid_index) {
    DG_INFO("You have not set skybox in this renderer");
    return;
  }
  //create render object
  m_renderObjects.emplace_back();
  auto& skybox = m_renderObjects.back();
  //render data prepare
  skybox.renderpass = m_context->m_gameViewPass;
  skybox.vertexBuffer = upLoadVertexDataToGPU(cubeVertexData, "skyMesh");
  skybox.indexBuffer = upLoadVertexDataToGPU(cubeIndexData, "skyMesh");
  skybox.globalUniform = m_context->m_viewProjectUniformBuffer;
  BufferCreateInfo bcinfo{};
  bcinfo.reset().setName("skyBoxMaterialUniformBuffer").setDeviceOnly(false).setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::UniformMaterial));
  skybox.materialUniform = createBuffer(bcinfo)->handle;
  //material stuff
  DescriptorSetLayoutCreateInfo skyLayoutInfo{};
  skyLayoutInfo.reset().addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "globalUniform"}).addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, "MaterialUniform"});
  skyLayoutInfo.setName("skyDescLayout");
  PipelineCreateInfo pipelineInfo;
  pipelineInfo.m_renderPassHandle = m_context->m_gameViewPass;
  pipelineInfo.m_depthStencil.setDepth(false, false, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineInfo.m_vertexInput.Attrib = Vertex::getAttributeDescriptions();
  pipelineInfo.m_vertexInput.Binding = Vertex::getBindingDescription();
  pipelineInfo.m_shaderState.reset();
  RasterizationCreation resCI{};
  resCI.m_cullMode = VK_CULL_MODE_FRONT_BIT;
  pipelineInfo.m_rasterization = resCI;
  pipelineInfo.m_shaderState.addStage("./shaders/vsskybox.vert", VK_SHADER_STAGE_VERTEX_BIT);
  pipelineInfo.m_shaderState.addStage("./shaders/fsskybox.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineInfo.m_shaderState.setName("skyPipeline");
  pipelineInfo.addDescriptorSetlayout(m_context->createDescriptorSetLayout(skyLayoutInfo));
  pipelineInfo.addDescriptorSetlayout(m_context->m_bindlessDescriptorSetLayout);
  MaterialCreateInfo matInfo{};
  matInfo.setName("skyMaterial");
  skybox.materialPtr = createMaterial(matInfo);
  std::shared_ptr<Program> skyProgram = createProgram("skyPipe", {pipelineInfo});
  skybox.materialPtr->setProgram(skyProgram);
  skybox.materialPtr->setDiffuseTexture(m_skyTexture);
  DescriptorSetCreateInfo descInfo{};
  descInfo.reset().setName("skyDesc").setLayout(skybox.materialPtr->program->passes[0].descriptorSetLayout[0]).buffer(skybox.globalUniform, 0).buffer(skybox.materialUniform, 1);
  skybox.descriptors.push_back(m_context->createDescriptorSet(descInfo));
  skybox.vertexIndicesCount = 36;
}

void Renderer::addSkyBox(std::string skyTexPath) {
  TextureCreateInfo texInfo{};
  texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setMipmapLevel(k_max_mipmap_level).setBindLess(true);
  m_skyTexture = upLoadTextureToGPU(skyTexPath, texInfo);
  have_skybox = true;
}

void Renderer::destroySkyBox() {
  auto& rj = m_renderObjects.front();
  for (auto& i : rj.descriptors) {
    m_context->DestroyDescriptorSet(i);
  }
}

}// namespace dg
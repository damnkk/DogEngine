#include "dgMaterial.h"
#include "CommandBuffer.h"
#include "DeviceContext.h"
#include "Renderer.h"
#include "dgShaderCompiler.h"
namespace dg {

MaterialCreateInfo& MaterialCreateInfo::setRenderOrder(int renderOrder) {
  this->renderOrder = renderOrder;
  return *this;
}

FrameBufferCreateInfo& FrameBufferCreateInfo::setDepthStencilTexture(dg::TextureHandle depthTexture) {
  if (depthTexture.index == k_invalid_index) {
    DG_ERROR("Can not set an invalid depth texture for frame buffer")
    exit(-1);
  }
  m_depthStencilTexture = depthTexture;
  return *this;
}

MaterialCreateInfo& MaterialCreateInfo::setName(std::string name) {
  this->name = name;
  return *this;
}

Program::~Program() {
  for (auto& passe : passes) {
    context->DestroyPipeline(passe.pipeline);
  }
}

Program::Program(std::shared_ptr<DeviceContext> context, std::string name) {
  this->context = context;
  this->name = name;
}

Material::Material() : textureMap({{"DiffuseTexture", {k_invalid_index, 0}}, {"MetallicRoughnessTexture", {k_invalid_index, 1}}, {"AOTexture", {k_invalid_index, 2}}, {"NormalTexture", {k_invalid_index, 3}}, {"EmissiveTexture", {k_invalid_index, 4}}}) {
}

void Material::addTexture(Renderer* renderer, std::string name, std::string path) {
  TextureCreateInfo texInfo{};
  texInfo.setFormat(VK_FORMAT_R8G8B8A8_UNORM).setFlag(TextureFlags::Mask::Default_mask).setBindLess(true).setMipmapLevel(1);
  TextureHandle handle = renderer->upLoadTextureToGPU(path, texInfo);
  textureMap[name] = {handle, (u32) textureMap.size()};
}

void Material::updateProgram() {
}

void generateLUTTexture(Renderer* renderer, TextureHandle LUTTexture) {
  RenderPassCreateInfo lutRenderPassCI{};
  lutRenderPassCI.setName("LutRenderPass").addRenderTexture(LUTTexture).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear);
  RenderPassHandle      lutRenderPass = renderer->getContext()->createRenderPass(lutRenderPassCI);
  FrameBufferCreateInfo lutFrameBuffer{};
  lutFrameBuffer.reset().addRenderTarget(LUTTexture).setRenderPass(lutRenderPass).setExtent({512, 512});
  FrameBufferHandle lutFBO = renderer->getContext()->createFrameBuffer(lutFrameBuffer);

  PipelineCreateInfo pipelineInfo;
  pipelineInfo.m_renderPassHandle = lutRenderPass;
  pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
  pipelineInfo.m_shaderState.reset();
  pipelineInfo.m_shaderState.addStage("./shaders/lut.vert", VK_SHADER_STAGE_VERTEX_BIT);
  pipelineInfo.m_shaderState.addStage("./shaders/lut.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineInfo.m_shaderState.setName("lutPipeline");
  pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
  PipelineHandle           lutPipeline = renderer->getContext()->createPipeline(pipelineInfo);
  CommandBuffer*           cmd = renderer->getContext()->getInstantCommandBuffer();
  VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
  cmd->bindPass(lutRenderPass);

  cmd->bindPipeline(lutPipeline);
  Rect2DInt scissor;
  scissor = {0, 0, 512, 512};
  cmd->setScissor(&scissor);
  ViewPort viewport;
  viewport.max_depth = 1.0f;
  viewport.min_depth = 0.0f;
  viewport.rect.width = scissor.width;
  viewport.rect.height = scissor.height;
  cmd->setViewport(&viewport);
  cmd->setDepthStencilState(VK_TRUE);
  cmd->draw(TopologyType::Triangle, 0, 3, 0, 1);
  cmd->endpass();
  cmd->flush(renderer->getContext()->m_graphicsQueue);
  renderer->getContext()->DestroyPipeline(lutPipeline);
  renderer->getContext()->DestroyFrameBuffer(lutFBO, false);
  renderer->getContext()->DestroyRenderPass(lutRenderPass);
  cmd->destroy();
}

void Material::addLutTexture(Renderer* renderer) {
  auto LUTTextureFind = renderer->getResourceCache().textures.find("LUTTexture");
  if (LUTTextureFind != renderer->getResourceCache().textures.end()) {
    if (textureMap.find("LUTTexture") != textureMap.end()) {
      textureMap["LUTTexture"].texture = LUTTextureFind->second->handle;
      return;
    }
    textureMap["LUTTexture"] = {LUTTextureFind->second->handle, (u32) this->textureMap.size()};
    return;
  }
  TextureCreateInfo lutCI{};
  lutCI.setExtent({512, 512, 1}).setName("LUTTexture").setBindLess(true).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask).setTextureType(TextureType::Enum::Texture2D).setFormat(VK_FORMAT_R16G16_SFLOAT);
  TextureHandle LutTex = renderer->createTexture(lutCI)->handle;
  generateLUTTexture(renderer, LutTex);
  textureMap["LUTTexture"] = {LutTex, (u32) textureMap.size()};
}

void generateDiffuseEnvTexture(Renderer* renderer, TextureHandle handle, TextureHandle envMap) {
  Material::aliInt um = {(int) envMap.index};
  BufferCreateInfo bufferCI{};
  bufferCI.reset().setName("ENVMapIdx").setUsageSize(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material::aliInt)).setDeviceOnly(false).setData(&um);
  BufferHandle bufHandle = renderer->getContext()->createBuffer(bufferCI);

  DescriptorSetLayoutCreateInfo descLayoutInfo{};
  descLayoutInfo.reset().setName("ENVMapIdx").addBinding({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, "Idx"});
  DescriptorSetLayoutHandle descLayoutHandle = renderer->getContext()->createDescriptorSetLayout(descLayoutInfo);

  DescriptorSetCreateInfo descCI{};
  descCI.reset().buffer(bufHandle, 0).setName("ENVMapIdx").setLayout(descLayoutHandle);
  DescriptorSetHandle descHandle = renderer->getContext()->createDescriptorSet(descCI);

  RenderPassCreateInfo irraRenderPassCI{};
  irraRenderPassCI.setName("DiffRenderPass").addRenderTexture(handle).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear);
  RenderPassHandle      irraRenderPass = renderer->getContext()->createRenderPass(irraRenderPassCI);
  Texture*              irraDTexture = renderer->getContext()->accessTexture(handle);
  FrameBufferCreateInfo diffFrameBufferCI{};
  diffFrameBufferCI.reset().addRenderTarget(handle).setRenderPass(irraRenderPass).setExtent({irraDTexture->m_extent.width, irraDTexture->m_extent.height});
  FrameBufferHandle irraFBO = renderer->getContext()->createFrameBuffer(diffFrameBufferCI);
  //RDOC_CATCH_START
  PipelineCreateInfo pipelineInfo{};
  pipelineInfo.m_renderPassHandle = irraRenderPass;
  pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
  pipelineInfo.m_shaderState.reset();
  pipelineInfo.m_shaderState.addStage("./shaders/lut.vert", VK_SHADER_STAGE_VERTEX_BIT);
  pipelineInfo.m_shaderState.addStage("./shaders/irra.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineInfo.m_shaderState.setName("irraPipeline");
  pipelineInfo.addDescriptorSetlayout(descLayoutHandle);
  pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
  PipelineHandle irraPipeline = renderer->getContext()->createPipeline(pipelineInfo);
  CommandBuffer* cmd = renderer->getContext()->getInstantCommandBuffer();

  VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
  cmd->bindPass(irraRenderPass);
  cmd->bindPipeline(irraPipeline);
  Rect2DInt scissor;
  scissor = {0, 0, (u16) irraDTexture->m_extent.width, (u16) irraDTexture->m_extent.height};
  cmd->setScissor(&scissor);
  ViewPort viewport;
  viewport.max_depth = 1.0f;
  viewport.min_depth = 0.0f;
  viewport.rect.width = scissor.width;
  viewport.rect.height = scissor.height;
  cmd->setViewport(&viewport);
  cmd->setDepthStencilState(VK_FALSE);
  cmd->bindDescriptorSet({descHandle}, 0, nullptr, 0);
  cmd->bindDescriptorSet({renderer->getContext()->m_bindlessDescriptorSet}, 1, nullptr, 0);
  cmd->draw(TopologyType::Enum::Triangle, 0, 3, 0, 1);
  cmd->endpass();
  cmd->flush(renderer->getContext()->m_graphicsQueue);
  renderer->getContext()->DestroyPipeline(irraPipeline);
  renderer->getContext()->DestroyFrameBuffer(irraFBO, false);
  renderer->getContext()->DestroyRenderPass(irraRenderPass);
  renderer->getContext()->DestroyBuffer(bufHandle);
  renderer->getContext()->DestroyDescriptorSet(descHandle);
  cmd->destroy();
}

void Material::addDiffuseEnvMap(Renderer* renderer, TextureHandle HDRTexture) {
  Texture*    HDR = renderer->getContext()->accessTexture(HDRTexture);
  std::string name = "irradianceTex";
  name += HDR->m_name;
  auto Find = renderer->getResourceCache().textures.find(name);
  if (Find != renderer->getResourceCache().textures.end()) {
    if (textureMap.find("DiffuseEnvMap") != textureMap.end()) {
      textureMap["DiffuseEnvMap"].texture = Find->second->handle;
      return;
    }
    textureMap["DiffuseEnvMap"] = {Find->second->handle, (u32) textureMap.size()};
    return;
  }
  TextureCreateInfo DiffTexCI{};
  DiffTexCI.setBindLess(true).setName(name.c_str()).setExtent({HDR->m_extent.width / 2, HDR->m_extent.height / 2, 1}).setFormat(VK_FORMAT_R8G8B8A8_UNORM).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask).setMipmapLevel(1).setTextureType(TextureType::Enum::Texture2D);
  TextureHandle handle = renderer->createTexture(DiffTexCI)->handle;
  generateDiffuseEnvTexture(renderer, handle, HDRTexture);
  textureMap["DiffuseEnvMap"] = {handle, (u32) textureMap.size()};
};

void generateSpecularEnvTexture(Renderer* renderer, TextureHandle handle, TextureHandle envMap) {
  struct PushBlock {
    int   EnvIdx;
    float roughness;
  } pushBlock;

  Texture*          specTexture = renderer->getContext()->accessTexture(handle);
  TextureCreateInfo fboTex{};
  fboTex.setName("fbo").setMipmapLevel(1).setBindLess(true).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setExtent(specTexture->m_extent).setFlag(TextureFlags::Mask::Default_mask | TextureFlags::Mask::RenderTarget_mask);
  TextureHandle fboHandle = renderer->getContext()->createTexture(fboTex);
  Texture*      fboTexture = renderer->getContext()->accessTexture(fboHandle);

  RenderPassCreateInfo specRenderPassCI{};
  specRenderPassCI.setName("specRenderPass").addRenderTexture(fboHandle).setOperations(RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear, RenderPassOperation::Enum::Clear).setFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  RenderPassHandle specRenderPass = renderer->getContext()->createRenderPass(specRenderPassCI);

  FrameBufferCreateInfo specFrameBufferCI{};
  specFrameBufferCI.reset().addRenderTarget(fboHandle).setRenderPass(specRenderPass).setExtent({specTexture->m_extent.width, specTexture->m_extent.height});
  FrameBufferHandle specFBO = renderer->getContext()->createFrameBuffer(specFrameBufferCI);

  VkPushConstantRange push{};
  push.size = sizeof(PushBlock);
  push.offset = 0;
  push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  PipelineCreateInfo pipelineInfo{};
  pipelineInfo.m_renderPassHandle = specRenderPass;
  pipelineInfo.m_depthStencil.setDepth(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
  pipelineInfo.m_rasterization.m_cullMode = VK_CULL_MODE_NONE;
  pipelineInfo.m_shaderState.reset();
  pipelineInfo.m_shaderState.addStage("./shaders/lut.vert", VK_SHADER_STAGE_VERTEX_BIT);
  pipelineInfo.m_shaderState.addStage("./shaders/prefilter.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
  pipelineInfo.m_shaderState.setName("irraPipeline");
  pipelineInfo.addDescriptorSetlayout(renderer->getContext()->m_bindlessDescriptorSetLayout);
  pipelineInfo.addPushConstants(push);
  PipelineHandle specPipeline = renderer->getContext()->createPipeline(pipelineInfo);
  CommandBuffer* cmd = renderer->getContext()->getInstantCommandBuffer();
  //RDOC_CATCH_START
  VkCommandBufferBeginInfo cmdBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd->m_commandBuffer, &cmdBeginInfo);
  renderer->getContext()->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_UNDEFINED, ResourceState::RESOURCE_STATE_RENDER_TARGET, 0, 1, false);
  renderer->getContext()->addImageBarrier(cmd->m_commandBuffer, specTexture, ResourceState::RESOURCE_STATE_UNDEFINED, ResourceState::RESOURCE_STATE_COPY_DEST, 0, specTexture->m_mipLevel, false);
  for (u32 i = 0; i < specTexture->m_mipLevel; ++i) {
    pushBlock.roughness = (float) i / (float) (specTexture->m_mipLevel - 1);
    pushBlock.EnvIdx = envMap.index;
    cmd->bindPass(specRenderPass);
    cmd->bindPipeline(specPipeline);
    Rect2DInt scissor;
    scissor = {0, 0, (u16) specTexture->m_extent.width, (u16) specTexture->m_extent.height};
    cmd->setScissor(&scissor);
    ViewPort viewport;
    viewport.max_depth = 1.0f;
    viewport.min_depth = 0.0f;
    viewport.rect.width = static_cast<float>(scissor.width * std::pow(0.5f, i));
    viewport.rect.height = static_cast<float>(scissor.height * std::pow(0.5f, i));
    cmd->setViewport(&viewport);
    cmd->setDepthStencilState(VK_FALSE);
    cmd->bindPushConstants<PushBlock>(VK_SHADER_STAGE_FRAGMENT_BIT, &pushBlock);
    cmd->bindDescriptorSet({renderer->getContext()->m_bindlessDescriptorSet}, 0, nullptr, 0);
    cmd->draw(TopologyType::Enum::Triangle, 0, 3, 0, 1);
    cmd->endpass();
    renderer->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_RENDER_TARGET, ResourceState ::RESOURCE_STATE_COPY_SOURCE, 0, 1, false);
    VkImageCopy copyRegion = {};
    copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.layerCount = 1;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel = 0;
    copyRegion.srcOffset = {0, 0, 0};

    copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = 0;
    copyRegion.dstSubresource.mipLevel = i;
    copyRegion.dstSubresource.layerCount = 1;
    copyRegion.dstOffset = {0, 0, 0};

    copyRegion.extent.width = static_cast<u32>(viewport.rect.width);
    copyRegion.extent.height = static_cast<u32>(viewport.rect.height);
    copyRegion.extent.depth = 1;
    if (copyRegion.extent.width != 0 && copyRegion.extent.height != 0) {
      vkCmdCopyImage(cmd->m_commandBuffer, fboTexture->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, specTexture->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    }
    renderer->addImageBarrier(cmd->m_commandBuffer, fboTexture, ResourceState::RESOURCE_STATE_COPY_SOURCE, ResourceState::RESOURCE_STATE_RENDER_TARGET, 0, 1, false);
  }
  renderer->addImageBarrier(cmd->m_commandBuffer, specTexture, ResourceState::RESOURCE_STATE_COPY_DEST, ResourceState::RESOURCE_STATE_SHADER_RESOURCE, 0, specTexture->m_mipLevel, false);
  cmd->flush(renderer->getContext()->m_graphicsQueue);
  //RDOC_CATCH_END
  renderer->getContext()->DestroyPipeline(specPipeline);
  renderer->getContext()->DestroyFrameBuffer(specFBO, true);
  renderer->getContext()->DestroyRenderPass(specRenderPass);
  cmd->destroy();
}

void Material::addSpecularEnvMap(Renderer* renderer, TextureHandle HDRTexture) {
  Texture*    HDR = renderer->getContext()->accessTexture(HDRTexture);
  std::string name = "SpecularTex";
  name += HDR->m_name;
  auto Find = renderer->getResourceCache().textures.find(name);
  if (Find != renderer->getResourceCache().textures.end()) {
    if (textureMap.find("SpecularEnvMap") != textureMap.end()) {
      textureMap["SpecularEnvMap"].texture = Find->second->handle;
    }
    textureMap["SpecularEnvMap"] = {Find->second->handle, (u32) textureMap.size()};
    return;
  }
  TextureCreateInfo SpecTexCI{};
  SpecTexCI.setBindLess(true).setName(name.c_str()).setExtent({HDR->m_extent.width, HDR->m_extent.height, 1}).setFormat(VK_FORMAT_R8G8B8A8_UNORM).setFlag(TextureFlags::Mask::Default_mask).setMipmapLevel(k_max_mipmap_level).setTextureType(TextureType::Enum::Texture2D);
  TextureHandle handle = renderer->createTexture(SpecTexCI)->handle;
  generateSpecularEnvTexture(renderer, handle, HDRTexture);
  textureMap["SpecularEnvMap"] = {handle, (u32) textureMap.size()};
}

void Material::setIblMap(Renderer* renderer, std::string path) {
  TextureCreateInfo texInfo{};
  texInfo.setFlag(TextureFlags::Mask::Default_mask).setFormat(VK_FORMAT_R8G8B8A8_SRGB).setMipmapLevel(16).setBindLess(true);
  TextureHandle hdrTex = this->renderer->upLoadTextureToGPU(path, texInfo);
  Texture*      hdr = renderer->getContext()->accessTexture(hdrTex);
  //renderer->getContext()->m_descriptorSetUpdateQueue.pop_back();
  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.descriptorCount = 1;
  write.dstArrayElement = hdrTex.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstSet = renderer->getContext()->m_VulkanBindlessDescriptorSet;
  write.dstBinding = k_bindless_sampled_texture_bind_index;
  write.pImageInfo = &hdr->m_imageInfo;
  vkUpdateDescriptorSets(renderer->getContext()->m_logicDevice, 1, &write, 0, nullptr);
  addLutTexture(renderer);
  addDiffuseEnvMap(this->renderer, hdrTex);
  addSpecularEnvMap(this->renderer, hdrTex);
  bool isSameWithSkyTex = (hdrTex.index == renderer->getSkyTexture().index);
  if (!isSameWithSkyTex) {
    auto hdrResource = renderer->getResourceCache().textures[hdr->m_name];
    renderer->destroyTexture(hdrResource);
  }
}

void Material::setIblMap(Renderer* renderer, TextureHandle handle) {
  Texture* hdr = renderer->getContext()->accessTexture(handle);
  renderer->getContext()->m_descriptorSetUpdateQueue.pop_back();

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.descriptorCount = 1;
  write.dstArrayElement = handle.index;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.dstSet = renderer->getContext()->m_VulkanBindlessDescriptorSet;
  write.dstBinding = k_bindless_sampled_texture_bind_index;
  write.pImageInfo = &hdr->m_imageInfo;
  vkUpdateDescriptorSets(renderer->getContext()->m_logicDevice, 1, &write, 0, nullptr);
  addDiffuseEnvMap(this->renderer, handle);
  bool isSameWithSkyTex = (handle.index == renderer->getSkyTexture().index);
  if (!isSameWithSkyTex) {
    auto hdrResource = renderer->getResourceCache().textures[hdr->m_name];
    renderer->destroyTexture(hdrResource);
  }
}

void Material::setProgram(const std::shared_ptr<Program>& program) {
  Material::program = program;
};
}// namespace dg
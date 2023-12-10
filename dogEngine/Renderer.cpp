#include "Renderer.h"
#include "ModelLoader.h"
#include "CommandBuffer.h"
namespace dg{

const std::string                       TextureResource::k_type = "texture_resource";
const std::string                       BufferResource::k_type = "buffer_resource";
const std::string                       SamplerResource::k_type = "sampler_resource";
const std::string                       ProgramResource::k_type = "program_resource";

void keycallback(GLFWwindow *window)
{
  float sensitivity = 0.07;
  
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    sensitivity = 1.0;
  }
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    DeviceContext::m_camera->pos += sensitivity * DeviceContext::m_camera->direction;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    DeviceContext::m_camera->pos -= sensitivity * DeviceContext::m_camera->direction;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    DeviceContext::m_camera->pos -=
        sensitivity * glm::normalize(glm::cross(DeviceContext::m_camera->direction, DeviceContext::m_camera->up));
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    DeviceContext::m_camera->pos +=
        sensitivity * glm::normalize(glm::cross(DeviceContext::m_camera->direction, DeviceContext::m_camera->up));
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    DeviceContext::m_camera->pos += sensitivity * glm::vec3(0, 1, 0);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
    DeviceContext::m_camera->pos -= sensitivity * glm::vec3(0, 1, 0);
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
    sensitivity= 0.7;
  }
  if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE){
    sensitivity = 0.007;
  }

}

ProgramCreateInfo&  ProgramCreateInfo::setName(const char* name){
  this->name = name;
  return *this;
}

void ResourceCache::destroy(Renderer* renderer){
  for(auto& i:textures){
    renderer->destroyTexture(i.second);
  }
  textures.clear();

  for(auto& i:samplers){
    renderer->destroySampler(i.second);
  }
  samplers.clear();

  for(auto& i : buffers){
    renderer->destroyBuffer(i.second);
  }
  buffers.clear();

  for(auto& i:materials){
    renderer->destroyMaterial(i.second);
  }
  materials.clear();
  for(auto& i:programs){
    renderer->destroyProgram(i.second);
  }
  programs.clear();

}

MaterialCreateInfo& MaterialCreateInfo::setProgram(ProgramResource* progInfo){
  program = progInfo;
  name = program->name;
  return *this;
}

MaterialCreateInfo& MaterialCreateInfo::setRenderOrder(u32 renderOrder){
  this->renderOrder = renderOrder;
  return *this;
}

MaterialCreateInfo& MaterialCreateInfo::setName(const char* name){
  this->name = name;
  return *this;
}

void Renderer::init(std::shared_ptr<DeviceContext> context){
    m_context = context;
    GUI::getInstance().init(context);
    m_buffers.init(4096);
    m_textures.init(256);
    m_samplers.init(256);
    m_programs.init(32);
    m_materials.init(32);
    m_objLoader = std::make_shared<objLoader>(this);
    m_gltfLoader = std::make_shared<gltfLoader>(this);
}

void Renderer::destroy(){
    vkDeviceWaitIdle(m_context->m_logicDevice);
    GUI::getInstance().Destroy();
    m_resourceCache.destroy(this);
    m_gltfLoader->destroy();
    m_objLoader->destroy();
    m_context->Destroy();
}

TextureResource* Renderer::createTexture(const TextureCreateInfo& textureInfo){
    if(textureInfo.name.empty()){
      DG_ERROR("Sampled texture object must have an unique name");
      exit(-1);
    }
    auto findRes = m_resourceCache.textures.find(textureInfo.name);
    if(m_resourceCache.textures.find(textureInfo.name)!=m_resourceCache.textures.end()){
        DG_INFO("There is a Texture with the same name in resourceCache, renderer will reuse it");
        return findRes->second;        
    }
    auto textureRes = m_textures.obtain();
    textureRes->name = textureInfo.name;
    textureRes->handle = m_context->createTexture(textureInfo);
    m_resourceCache.textures[textureInfo.name] = textureRes;
    return textureRes;
}

BufferResource* Renderer::createBuffer(const BufferCreateInfo& bufferInfo){
    if(bufferInfo.name.empty()){
      DG_ERROR("Buffer created by renderer must have an unique name");
      exit(-1);
    }
    if(m_resourceCache.buffers.find(bufferInfo.name)!=m_resourceCache.buffers.end()){
      DG_ERROR("You are trying to create a buffer which has a same name with an existing buffer, through renderer");
      exit(-1);
    }
    auto bufferRes = m_buffers.obtain();
    bufferRes->name = bufferInfo.name;
    bufferRes->handle = m_context->createBuffer(bufferInfo);
    m_resourceCache.buffers[bufferInfo.name] = bufferRes;
    return bufferRes;
}

SamplerResource* Renderer::createSampler(const SamplerCreateInfo& samplerInfo){
    if(samplerInfo.name.empty()){
      DG_ERROR("Sampler created by renderer must have an unique name")
      exit(-1);
    }
    if(m_resourceCache.samplers.find(samplerInfo.name)!=m_resourceCache.samplers.end()){
      DG_ERROR("You are trying to create a sampler which has a same name with an existing sampler, through renderer");
      exit(-1);
    }
    auto samplerRes = m_samplers.obtain();
    samplerRes->name = samplerInfo.name;
    samplerRes->handle = m_context->createSampler(samplerInfo);
    m_resourceCache.samplers[samplerInfo.name] = samplerRes;
    return samplerRes;
}

Material* Renderer::createMaterial(const MaterialCreateInfo& matInfo){
  if(matInfo.name.empty()){
    DG_ERROR("Material created by renderer must have an unique name");
    exit(-1);
  }
  auto findRes = m_resourceCache.materials.find(matInfo.name);
  if(findRes != m_resourceCache.materials.end()){
    DG_INFO("There is a material with the same name in resourceCache, renderer will reuse it")
    return findRes->second;
  }

  auto material = m_materials.obtain();
  material->program = matInfo.program;
  material->name = matInfo.name;
  material->renderOrder = matInfo.renderOrder;
  m_resourceCache.materials[material->name] = material;
  return material;
}

ProgramResource* Renderer::createProgram(const ProgramCreateInfo& programInfo){
  if(programInfo.name.empty()){
    DG_ERROR("Program created by renderer must have an unique name");
    exit(-1);
  }
  auto findRes = m_resourceCache.programs.find(programInfo.name);
  if(findRes != m_resourceCache.programs.end()){
    DG_INFO("There is a program with the same name in resourceCache, renderer will reuse it")
    return findRes->second;
  }
  auto programRes = m_programs.obtain();
  u32 numPasses = 1;
  programRes->passes.resize(numPasses);
  programRes->name = programInfo.name;
  for(int i = 0;i<numPasses;++i){
    ProgramPass& pass = programRes->passes[i];
    pass.descriptorSetLayout = m_context->createDescriptorSetLayout(programInfo.dslyaoutInfo);
    pipelineCreateInfo pipelineinfo = programInfo.pipelineInfo;
    pipelineinfo.addDescriptorSetlayout(pass.descriptorSetLayout);
    pass.pipeline = m_context->createPipeline(pipelineinfo);
  }
  m_resourceCache.programs[programRes->name] = programRes;
  return programRes;
}


void Renderer::destroyBuffer(BufferResource* bufRes){
  if(!bufRes){
    DG_WARN("An invalid bufferResource pointer");
    return;
  }
  m_context->DestroyBuffer(bufRes->handle);
  m_buffers.release(bufRes);

}

void Renderer::destroyTexture(TextureResource* texRes){
  if(!texRes){
    DG_WARN("An invalid TextureResource pointer");
    return;
  }
  m_context->DestroyTexture(texRes->handle);
  m_textures.release(texRes);
}

void Renderer::destroySampler(SamplerResource* samplerRes){
  if(!samplerRes){
    DG_WARN("An invalid SamplerResource pointer");
    return;
  }
  m_context->DestroySampler(samplerRes->handle);
  m_samplers.release(samplerRes);
}

void Renderer::destroyMaterial(Material* material){
  if(!material){
    DG_WARN("An invalid Material pointer");
    return;
  }
  m_materials.release(material);
  
}

void Renderer::destroyProgram(ProgramResource* progRes){
  if(!progRes){
    DG_WARN("An invalid ProgramResource pointer");
    return;
  }
  m_context->DestroyPipeline(progRes->passes[0].pipeline);
  m_programs.release(progRes);
}

void Renderer::newFrame(){
    m_context->newFrame();
    GUI::getInstance().newGUIFrame();
}

void Renderer::present(){
    m_context->present();

}

void Renderer::loadFromPath(const std::string& path){
    if(path.empty()){
        DG_WARN("Invalid resource path");
        return ;
    }
    auto found = path.find_last_of(".");
    std::string fileExtension = path.substr(found+1);
    if(fileExtension=="obj"){
        m_objLoader->loadFromPath(path);
    }else if(fileExtension == "gltf"){
        m_gltfLoader->loadFromPath(path);
    }
}

void Renderer::executeScene(){
    if(m_objLoader->haveContent()){
        m_objLoader->Execute(m_objLoader->getSceneNode());
    }
    if(m_gltfLoader->haveContent()){
        m_gltfLoader->Execute(m_gltfLoader->getSceneNode());
    }
}

void Renderer::drawScene(){
    CommandBuffer* cmd = m_context->getCommandBuffer(QueueType::Enum::Graphics, true);
    std::vector<RenderObject> renderObjects;
    if(m_objLoader->haveContent()){
       renderObjects = m_objLoader->getRenderObject();
    }
    if(m_gltfLoader->haveContent()){
        renderObjects.insert(renderObjects.end(),m_gltfLoader->getRenderObject().begin(),m_gltfLoader->getRenderObject().end());
    }
    for(int i = 0;i<renderObjects.size();++i){
      auto& currRenderObject = renderObjects[i];
      cmd->bindPass(currRenderObject.m_renderPass);
      cmd->bindPipeline(currRenderObject.m_material->program->passes[0].pipeline);
      Rect2DInt scissor;
      scissor = {0,0,(u16)m_context->m_swapChainWidth,(u16)m_context->m_swapChainHeight};
      cmd->setScissor(&scissor);
      ViewPort viewPort;
      viewPort.max_depth = 1.0f;
      viewPort.min_depth = 0.0f;
      viewPort.rect = scissor;
      cmd->setViewport(&viewPort);
      cmd->bindVertexBuffer(currRenderObject.m_vertexBuffer, 0, 0);
      cmd->bindIndexBuffer(currRenderObject.m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdSetDepthTestEnable(cmd->m_commandBuffer, VK_TRUE);
      Buffer* uniformBuffer = m_context->accessBuffer(currRenderObject.m_uniformBuffer);
      if(!uniformBuffer){DG_WARN("Invalid uniform Buffer");}
      void* data;
      vmaMapMemory(m_context->m_vma, uniformBuffer->m_allocation, &data);
      UniformData udata{};
      udata.cameraPos = m_context->m_camera->pos;
      udata.cameraDirectory = m_context->m_camera->direction;
      static auto startTime = std::chrono::high_resolution_clock::now();
      auto endTime = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float, std::chrono::seconds::period>(endTime-startTime).count();
      udata.modelMatrix = currRenderObject.m_modelMatrix;
      udata.modelMatrix = glm::scale(udata.modelMatrix,glm::vec3(0.5f,0.5f,0.5f));
      //udata.modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f,0.0f,0.0f));
      udata.projectMatrix = m_context->m_camera->getProjectMatrix();
      udata.viewMatrix = m_context->m_camera->getViewMatrix();
      u32 test = sizeof(UniformData);
      memcpy(data,&udata,sizeof(UniformData));
      vmaUnmapMemory(m_context->m_vma, uniformBuffer->m_allocation);
      // std::vector<DescriptorSetHandle> descriptors;
      // descriptors.push_back(currRenderObject.m_descriptor);
      // if(currRenderObject.m_descriptors.size()>k_max_swapchain_images){
      //     descriptors.insert(descriptors.end(),currRenderObject.m_descriptors.begin()+k_max_swapchain_images,currRenderObject.m_descriptors.end());
      // }
      cmd->bindDescriptorSet(currRenderObject.m_descriptors, nullptr, 0);
      //cmd->draw(TopologyType::Enum::Triangle, 0, currRenderObject.m_vertexCount, 0, 0);
      cmd->drawIndexed(TopologyType::Enum::Triangle, currRenderObject.m_indexCount, 1, 0, 0, 0);
    }
}

void Renderer::setCurrentMaterial(Material* mat){
  if(!mat){
    DG_WARN("You are implementing a invalid material as current material, check again");
    return ;
  }
  m_currentMaterial = mat;
}

Material* Renderer::getCurrentMaterial(){
  return m_currentMaterial;
}

void Renderer::drawUI(){
    //之后可以在这里补充一些其他的界面逻辑
    //这里UI绘制和主场景绘制的commandbuffer共用,效果不好,回头单独给GUI创建一个commandbufferpool,分开录制。
    GUI::getInstance().OnGUI();
    GUI::getInstance().endGUIFrame();
    auto cmd = m_context->getCommandBuffer(QueueType::Enum::Graphics, false);
    auto drawData = GUI::getInstance().GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawData, cmd->m_commandBuffer);
    m_context->queueCommandBuffer(cmd);
}

}
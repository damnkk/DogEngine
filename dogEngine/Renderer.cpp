#include "Renderer.h"
#include "ModelLoader.h"
#include "CommandBuffer.h"
namespace dg{

void keycallback(GLFWwindow *window)
{
  float sensitivity = 0.7;
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

void Renderer::init(std::shared_ptr<DeviceContext> context){
    m_context = context;
    GUI::getInstance().init(context);
    m_objLoader = std::make_shared<objLoader>(context);
    m_gltfLoader = std::make_shared<gltfLoader>(context);
}

void Renderer::destroy(){
    vkDeviceWaitIdle(m_context->m_logicDevice);
    GUI::getInstance().Destroy();
    m_context->Destroy();
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
        cmd->bindPipeline(currRenderObject.m_pipeline);
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
        Buffer* uniformBuffer = m_context->accessBuffer(currRenderObject.m_uniformBuffers[m_context->m_currentFrame]);
        if(!uniformBuffer){DG_WARN("Invalid uniform Buffer");}
        void* data;
        vmaMapMemory(m_context->m_vma, uniformBuffer->m_allocation, &data);
        UniformData udata{};
        udata.cameraPos = m_context->m_camera->pos;
        udata.modelMatrix = currRenderObject.m_modelMatrix;
        udata.projectMatrix = m_context->m_camera->getProjectMatrix();
        udata.viewMatrix = m_context->m_camera->getViewMatrix();
        u32 test = sizeof(UniformData);
        memcpy(data,&udata,sizeof(UniformData));
        vmaUnmapMemory(m_context->m_vma, uniformBuffer->m_allocation);
        std::vector<DescriptorSetHandle> descriptors;
        descriptors.push_back(currRenderObject.m_descriptors[m_context->m_currentFrame]);
        if(currRenderObject.m_descriptors.size()>k_max_swapchain_images){
            descriptors.insert(descriptors.end(),currRenderObject.m_descriptors.begin()+k_max_swapchain_images,currRenderObject.m_descriptors.end());
        }
        cmd->bindDescriptorSet(descriptors, nullptr, 0);
        //cmd->draw(TopologyType::Enum::Triangle, 0, currRenderObject.m_vertexCount, 0, 0);
        cmd->drawIndexed(TopologyType::Enum::Triangle, currRenderObject.m_indexCount, 1, 0, 0, 0);
    }
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
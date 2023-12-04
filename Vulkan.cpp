#include "dogEngine/DeviceContext.h"
#include "dogEngine/Renderer.h"

using namespace dg;

GLFWwindow* createGLTFWindow(u32 width,u32 height) {
  DG_INFO("Window Initiation")
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(width, height, "DogEngine", nullptr, nullptr);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);
  return window;
}

int main() {
  auto window = createGLTFWindow(1280,720);
  ContextCreateInfo contextInfo;
  contextInfo.set_window(1280, 720, window);
  contextInfo.m_applicatonName = "God Engine";
  contextInfo.m_debug = true;
  std::shared_ptr<DeviceContext> context = std::make_shared<DeviceContext>();
  context->init(contextInfo);
  Renderer renderer;
  renderer.init(context);
  //renderer.loadFromPath("./models/nanosuit/nanosuit.obj");
  renderer.loadFromPath("./models/duck/12248_Bird_v1_L2.obj");
  renderer.executeScene();

  while(!glfwWindowShouldClose(context->m_window)){
    glfwPollEvents();
    keycallback(context->m_window);
    renderer.newFrame();
    renderer.drawScene();
    renderer.drawUI();
    renderer.present();
    //std::cout<<"11";
  }

// context的销毁还没有实现完。
  renderer.destroy();
  return 0;
}

#include "dogEngine/DeviceContext.h"
#include "dogEngine/Renderer.h"
#include "Vertex.h"
#include "ModelLoader.h"

using namespace dg;

GLFWwindow* createGLTFWindow(u32 width,u32 height) {
  DG_INFO("Window Initiation")
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(width, height, "DogEngine", nullptr, nullptr);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR);
  return window;
}

bool isMinimized = false;
void windowIconifyCallback(GLFWwindow* window, int iconified){
  if(iconified){
    isMinimized = true;
  }else {
    isMinimized = false;
  }
}

int main() {
  auto window = createGLTFWindow(1380,720);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);
  ContextCreateInfo contextInfo;
  int width,height;
  glfwGetWindowSize(window, &width, &height);
  contextInfo.set_window(width, height, window);
  contextInfo.m_applicatonName = "God Engine";
  contextInfo.m_debug = true;
  std::shared_ptr<DeviceContext> context = std::make_shared<DeviceContext>();
  context->init(contextInfo);
  Renderer renderer;
  renderer.init(context);

  //renderer.loadFromPath("./models/nanosuit/nanosuit.obj");
  //renderer.loadFromPath("./models/Sponza/sponza.obj");
  //renderer.loadFromPath("./models/duck/12248_Bird_v1_L2.obj");
  //renderer.loadFromPath("./models/FlightHelmet/FlightHelmet.gltf");
  renderer.loadFromPath("./models/DamagedHelmet/DamagedHelmet.gltf");
  //renderer.loadFromPath("./models/glTFSponza/Sponza.gltf");

  renderer.addSkyBox("./models/skybox/small_empty_room_4_2k.hdr");
  //renderer.addSkyBox("./models/skybox/resting_place_2k.hdr");
  //renderer.addSkyBox("./models/skybox/farm_sunset_2k.hdr");
  renderer.executeScene();

  while(!glfwWindowShouldClose(context->m_window)){
    glfwPollEvents();
    keycallback(context->m_window);
    if(isMinimized) continue;
    renderer.newFrame();
    renderer.drawScene();
    renderer.drawUI();
    renderer.present();
  }

// context的销毁还没有实现完。
  renderer.destroy();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

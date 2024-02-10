#include "ModelLoader.h"
#include "Vertex.h"
#include "dogEngine/DeviceContext.h"
#include "dogEngine/Renderer.h"
#include "glslang_c_interface.h"

using namespace dg;

bool isMinimized = false;
void windowIconifyCallback(GLFWwindow* window, int iconified) {
  if (iconified) {
    isMinimized = true;
  } else {
    isMinimized = false;
  }
}

int main() {
  auto window = createGLFWWindow(1380, 720);
  glfwSetWindowIconifyCallback(window, windowIconifyCallback);
  ContextCreateInfo contextInfo;
  int               width, height;
  glfwGetWindowSize(window, &width, &height);
  contextInfo.set_window(width, height, window);
  contextInfo.m_applicatonName = "God Engine";
  contextInfo.m_debug = true;
  std::shared_ptr<DeviceContext> context = std::make_shared<DeviceContext>();
  context->init(contextInfo);
  Renderer renderer;
  renderer.init(context);

  //renderer.loadModel("./models/BoomBoxWithAxes/BoomBoxWithAxes.gltf");
  //renderer.loadModel("./models/Sponza/Sponza.gltf");
  renderer.loadModel("./models/orrery/scene.gltf");
  //renderer.loadModel("./models/Camera_01_2k/Camera_01_2k.gltf");
  //renderer.loadModel("./models/DamagedHelmet/DamagedHelmet.gltf");
  //renderer.loadModel("./models/MetalRoughSpheres/MetalRoughSpheres.gltf");
  //renderer.loadModel("./models/scene/scene.gltf");

  //renderer.addSkyBox("./models/skybox/small_empty_room_4_2k.hdr");
  //renderer.addSkyBox("./models/skybox/small_empty_room_1_2k.hdr");
  renderer.addSkyBox("./models/skybox/graveyard_pathways_2k.hdr");
  renderer.executeScene();

  while (!glfwWindowShouldClose(context->m_window)) {
    glfwPollEvents();
    if (isMinimized) continue;
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

  //------------------test-------------------
  // ResourceLoader loader;
  // loader.m_renderer = &renderer;
  // loader.loadModel("./models/orrery/scene.gltf");
  // loader.loadModel("./models/DamagedHelmet/DamagedHelmet.gltf");
  // loader.loadModel("./models/BoomBoxWithAxes/BoomBoxWithAxes.gltf");
  // std::cout << "test" << std::endl;
}

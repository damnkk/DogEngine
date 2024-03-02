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
  contextInfo.setWindow(width, height, window);
  contextInfo.m_applicatonName = "God Engine";
  contextInfo.m_debug = true;
  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rtPipelineFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
  VkPhysicalDeviceDescriptorIndexingFeaturesEXT    descIndexingFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
  VkPhysicalDeviceBufferAddressFeaturesEXT         bufferAddressFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
  VkPhysicalDeviceHostQueryResetFeatures           hostQueryResetFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES};

  contextInfo.addInstanceLayer("VK_LAYER_KHRONOS_validation")
      .addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
      .addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
      .addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &accelFeatures)
      .addDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, false, &rtPipelineFeatures)
      .addDeviceExtension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, false, &bufferAddressFeatures)
      .addDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, false, &descIndexingFeatures)
      .addDeviceExtension(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME, false, &hostQueryResetFeatures);
  std::shared_ptr<DeviceContext> context = std::make_shared<DeviceContext>();
  context->init(contextInfo);
  Renderer renderer;
  renderer.init(context);

  renderer.loadModel("./models/BoomBoxWithAxes/BoomBoxWithAxes.gltf");
  //renderer.loadModel("./models/Sponza/Sponza.gltf");
  //renderer.loadModel("./models/orrery/scene.gltf");
  //renderer.loadModel("./models/Camera_01_2k/Camera_01_2k.gltf");
  //renderer.loadModel("./models/DamagedHelmet/DamagedHelmet.gltf");
  //renderer.loadModel("./models/MetalRoughSpheres/MetalRoughSpheres.gltf");
  //renderer.loadModel("./models/scene/scene.gltf");
  //renderer.loadModel("./models/Bistro_v5_2/Interiorgltf/bistro1.gltf");
  //renderer.loadModel("./models/duck/12248_Bird_v1_L2.obj");

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

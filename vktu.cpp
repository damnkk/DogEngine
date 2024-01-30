#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <iostream>
struct Node{
  glm::vec4 color;
  float depth;
  uint32_t next;
};
int main() {
    Node nd;
    std::cout<<sizeof(nd)<<std::endl;

    return EXIT_SUCCESS;
}
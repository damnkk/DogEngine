#ifndef VERTEX_H
#define VERTEX_H
#include<iostream>
#include "dgpch.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>

namespace dg{
struct Vertex{
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec4 tangent = glm::vec4(0.0f);
    glm::vec2 texCoord = glm::vec2(0.0f);

    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

}// namespace dg

#endif //VERTEX_H
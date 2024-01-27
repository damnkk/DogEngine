#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "GLFW/glfw3.h"

namespace dg {

class Camera {
 public:
  Camera(){};
  glm::mat4 getViewMatrix(bool useEularAngle = true);
  glm::mat4 getProjectMatrix(bool ortho = false);
  void updateDirection(float deltaTime, glm::vec2 currMousePos);
  void updatePosition(float deltaTime, GLFWwindow *window);
  bool &rightButtonPressState() { return rightButtonClick; }

 public:
  glm::vec3 &getPosition() { return pos; }
  glm::vec3 &getUpVector() { return up; }
  glm::vec3 &getDirectVector() { return direction; }
  float &getFov() { return fov; }
  float &getAspect() { return aspect; }
  float &getZNear() { return zNear; }
  float &getZFar() { return zFar; }

 protected:
  glm::vec3 pos = glm::vec3(0.0f, 0.0f, 5.0f), up = glm::vec3(0.0f, 1.0f, 0.0f), direction = glm::vec3(-1.0f, 0.0f, 0.0f);
  float fov = 70.0f, aspect = 1.0f, zNear = 0.1f, zFar = 1000000.0f;
  float pitch = 0.0f, yaw = 0.0f, row = 0.0f;
  float left = -1.0f, right = 1.0f, top = 1.0f, down = -1.0f;
  //camera view Control
  glm::vec2 oldMousePos = glm::vec2(0.0, 0.0);
  float mouseSpeed = 4.0f;
  bool rightButtonClick = false;
  //Move Control
  glm::vec3 moveSpeed = glm::vec3(0);
  float acceleration = 150.0f;
  float damping = 0.2f;
  float maxSpeed = 10.0f;
  float fastCoef = 10.0f;
};

}// namespace dg

#endif//CAMERA_H
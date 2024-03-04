#include "Camera.h"
#include "imguiBase/imgui.h"
namespace dg {

void Camera::updateDirection(float deltaTime, glm::vec2 currMousePos) {
  if (!rightButtonClick) {
    oldMousePos.x = currMousePos.x;
    oldMousePos.y = currMousePos.y;
  }
  float sensitive = 0.35;
  float xDif = currMousePos.x - oldMousePos.x;
  float yDif = currMousePos.y - oldMousePos.y;
  pitch += sensitive * yDif;
  pitch = glm::clamp(pitch, -89.0f, 89.0f);
  yaw += sensitive * xDif;
  oldMousePos.x = currMousePos.x;
  oldMousePos.y = currMousePos.y;
}

void Camera::updatePosition(float deltaTime) {
  const glm::vec3 forward = direction;
  const glm::vec3 right = glm::normalize(glm::cross(forward, up));
  const glm::vec3 up = glm::normalize(glm::cross(right, forward));
  glm::vec3       accel(0.0f);
  if (ImGui::IsKeyDown(ImGuiKey_W)) accel += forward;
  if (ImGui::IsKeyDown(ImGuiKey_S)) accel -= forward;
  if (ImGui::IsKeyDown(ImGuiKey_A)) accel -= right;
  if (ImGui::IsKeyDown(ImGuiKey_D)) accel += right;
  if (ImGui::IsKeyDown(ImGuiKey_Space)) accel += up;
  if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) accel -= up;
  if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) accel *= fastCoef;

  if (accel == glm::vec3(0)) {
    moveSpeed -= moveSpeed * glm::min((1.0f / damping) * static_cast<float>(deltaTime), 1.0f);
  } else {
    moveSpeed += accel * acceleration * static_cast<float>(deltaTime);
    const float maxSpeed =
        ImGui::IsKeyDown(ImGuiKey_LeftShift) ? this->maxSpeed * fastCoef : this->maxSpeed;
    if (glm::length(moveSpeed) > maxSpeed) moveSpeed = glm::normalize(moveSpeed) * maxSpeed;
  }
  pos += moveSpeed * static_cast<float>(deltaTime);
}

glm::mat4 Camera::getViewMatrix(bool useEularAngle) {
  if (useEularAngle) {
    direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    direction.y = -sin(glm::radians(pitch));
    direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw));
  }
  return glm::lookAt(pos, pos + direction, up);
}

glm::mat4 Camera::getProjectMatrix(bool ortho) {
  if (ortho) { return glm::ortho(left, right, down, top, zNear, zFar); }
  auto proj = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
  proj[1][1] *= -1;
  return proj;
}

}// namespace dg
#include "Camera.h"
namespace dg{
glm::mat4 Camera::getViewMatrix(bool useEularAngle){
    if(useEularAngle){
        direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
        direction.y = -sin(glm::radians(pitch));
        direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    }
    return glm::lookAt(pos, pos + direction, up);
}


glm::mat4 Camera::getProjectMatrix(bool ortho){
    if(ortho){
        return glm::ortho(left, right, down, top, zNear, zFar);
    }
    return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}

}
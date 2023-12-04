#pragma once


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace dg{

class Camera {
public:
    glm::vec3 pos = glm::vec3(0.0f,0.0f,5.0f),up = glm::vec3(0.0f,1.0f,0.0f),direction = glm::vec3(-1.0f,0.0f,0.0f);
    float fov = 70.0f,aspect = 1.0f,zNear = 0.1f,zFar = 1000000.0f;
    float pitch = 0.0f,yaw = 0.0f,row = 0.0f;
    float left = -1.0f,right = 1.0f,top = 1.0f,down= -1.0f;
    Camera(){};
    
    glm::mat4 getViewMatrix(bool useEularAngle = true);
    glm::mat4 getProjectMatrix(bool ortho = false);
};

}
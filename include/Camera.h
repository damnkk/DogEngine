#pragma once
#include "Common.h"

class Camera {
public:
    glm::vec3 pos = glm::vec3(2.0f,2.0f,2.0f),up = glm::vec3(0.0f,1.0f,0.0f),direction = glm::vec3(-1.0f,0.0f,0.0f);
    float fov = 70.0f,aspect = 1.0f,zNear = 0.01f,zFar = 500.0f;
    float pitch = 0.0f,yaw = 0.0f,row = 0.0f;
    float left = -1.0f,right = 1.0f,top = 1.0f,down= -1.0f;
    Camera(){};
    
    glm::mat4 getViewMatrix(bool useEularAngle = true);
    glm::mat4 getProjectMatrix(bool ortho = false);
};
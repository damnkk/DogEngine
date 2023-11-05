#pragma once

#include "common.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace dg{

struct LightData{
    glm::vec3 m_Color = glm::vec3(1.0f);
    float m_AmbientIntensity = 0.0f;
    glm::vec3 m_LightPosition = glm::vec3(1.0f);
    float m_Radius = 1.0f;
};

class Light{
public:
    Light();
    Light(const float red, const float green, const float blue, const float ambient_intensity);
    ~Light();

    void SetLightPosition(glm::vec3 lightPosition) noexcept;
    void SetLightRadius(float radius) noexcept;
    LightData GetUBOData() const{ return m_LightData;}

private: 
    LightData m_LightData;
};

}
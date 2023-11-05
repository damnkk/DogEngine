#include "Light.h"

namespace dg{

Light::Light(){
    m_LightData.m_Color = glm::vec3(1.0f,1.0f,1.0f);
    m_LightData.m_AmbientIntensity = 1.0f;
    m_LightData.m_LightPosition = glm::vec3(0.0f,0.0f,-1.0f);
    m_LightData.m_Radius = 1.0f;
}

Light::Light(const float red, const float green, const float blue, const float ambient_intensity){
    m_LightData.m_Color.x = red;
    m_LightData.m_Color.y = green;
    m_LightData.m_Color.z = blue;

    m_LightData.m_AmbientIntensity = ambient_intensity;
    m_LightData.m_LightPosition = glm::vec3(0.0f,0.0f,-1.0f);
    m_LightData.m_Radius = 1.0f;
}

Light::~Light(){

}

void Light::SetLightPosition(glm::vec3 lightPosition) noexcept{m_LightData.m_LightPosition = lightPosition;}
void Light::SetLightRadius(float lightRadius) noexcept{m_LightData.m_Radius = lightRadius;}

} //namespace dg
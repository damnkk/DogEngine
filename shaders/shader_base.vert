#version 450

layout(binding=5)uniform UniformBufferObject{
    vec3 cameraPos;
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(location=0)in vec3 inPosition;
layout(location=1)in vec3 inNormal;
layout(location=2)in vec2 inTexCoord;

layout(location=0)out vec3 fragNormal;
layout(location=1)out vec2 fragTexCoord;
layout(location=2)out vec3 worldPos;

void main(){
    gl_Position=ubo.proj*ubo.view*ubo.model*vec4(inPosition,1.);
    worldPos=vec3(ubo.model*vec4(inPosition,1.));
    fragNormal=inNormal;
    fragTexCoord=inTexCoord;
}
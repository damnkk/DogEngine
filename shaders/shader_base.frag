#version 450
layout(binding=0)uniform UniformBufferObject{
    vec3 cameraPos;
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

layout(set=0,binding=0)uniform sampler2D diffuseTex;
layout(set=0,binding=1)uniform sampler2D mrtTex;
layout(set=0,binding=2)uniform sampler2D normTex;
layout(set=0,binding=3)uniform sampler2D emissTex;
layout(set=0,binding=4)uniform sampler2D occuTex;

layout(location=0)in vec3 fragNormal;
layout(location=1)in vec2 fragTexCoord;
layout(location=2)in vec3 worldPos;

layout(location=0)out vec4 outColor;

vec3 lightPos=vec3(2.f,6.f,0.f);

void main(){
    outColor=texture(diffuseTex,fragTexCoord);
}
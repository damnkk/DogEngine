#version 450

layout(binding=0)uniform samplerCube texSampler;

layout(location=0)in vec3 fragColor;
layout(location=1)in vec2 fragTexCoord;

layout(location=0)out vec4 outColor;

void main(){
    outColor=texture(texSampler,vec3(fragTexCoord,1.f));
}
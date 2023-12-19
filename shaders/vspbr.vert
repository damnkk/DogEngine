#version 450

layout(binding=30)uniform UniformBufferObject{
    vec3 cameraPos;
    vec3 cameraDirectory;
    mat4 view;
    mat4 proj;
}ubo;

layout(binding=31)uniform UniformMaterialObject{
    mat4 modelMatrix;
    vec4 baseColorFactor;
    vec3 emissiveFactor;
    vec3 tueFactor;
    vec3 mrFactor;
}umat;

layout(location=0)in vec3 inPosition;
layout(location=1)in vec3 inNormal;
layout(location=2)in vec2 inTexCoord;

layout(location=0)out vec3 fragNormal;
layout(location=1)out vec2 fragTexCoord;
layout(location=2)out vec3 worldPos;

void main(){
    gl_Position=ubo.proj*ubo.view*umat.modelMatrix*vec4(inPosition,1.);
    worldPos=vec3(umat.modelMatrix*vec4(inPosition,1.));
    fragNormal=normalize(mat3(transpose(inverse(umat.modelMatrix)))*inNormal);
    fragTexCoord=inTexCoord;
}
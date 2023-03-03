#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 worldPos;

layout(push_constant) uniform Model{
    mat4 Model;
}model;

void main() {
    gl_Position = ubo.proj * ubo.view * model.Model * vec4(inPosition, 1.0);
    worldPos = vec3(model.Model*vec4(inPosition,1.0));
    fragNormal = inNormal;
    fragTexCoord = inTexCoord;
}
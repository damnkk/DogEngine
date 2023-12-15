#version 450

layout(binding=5)uniform UniformBufferObject{
	vec3 cameraPos;
	vec3 cameraDirectory;
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout(location=0)in vec3 inPosition;
layout(location=1)in vec3 inNormal;
layout(location=2)in vec2 inTexCoord;

layout(location=0)out vec3 fragTexCoord;

mat4 skyboxMat={vec4(1.f,0.,0.,0.f),
	vec4(0.f,1.f,0.f,0.f),
	vec4(0.,0.,1.,0.f),
vec4(ubo.cameraPos.x,ubo.cameraPos.y,ubo.cameraPos.z,1.f)};

void main(){
	fragTexCoord=inPosition/5.;
	fragTexCoord.y*=-1;
	gl_Position=ubo.proj*ubo.view*skyboxMat*vec4(inPosition,1.);
}
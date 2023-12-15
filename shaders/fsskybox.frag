#version 450
layout(binding=5)uniform UniformBufferObject{
	vec3 cameraPos;
	vec3 cameraDirectory;
	mat4 model;
	mat4 view;
	mat4 proj;
}ubo;

layout(set=0,binding=0)uniform samplerCube skybox;
layout(set=0,binding=1)uniform sampler2D mrtTex;
layout(set=0,binding=2)uniform sampler2D normTex;
layout(set=0,binding=3)uniform sampler2D emissTex;
layout(set=0,binding=4)uniform sampler2D occuTex;
layout(location=0)in vec3 fragTexCoord;

layout(location=0)out vec4 outColor;
#define PI 3.14159265359

void main(){
	outColor=texture(skybox,fragTexCoord,11);
	//outColor=vec4(vec3(.2471,.7765,.9882),1.f);
}
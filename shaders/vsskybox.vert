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

layout(location=0)out vec3 worldPos;

mat4 skyboxMat={vec4(1.f,0.,0.,0.f),
	vec4(0.f,1.f,0.f,0.f),
	vec4(0.,0.,1.,0.f),
vec4(ubo.cameraPos.x,ubo.cameraPos.y,ubo.cameraPos.z,1.f)};

void main(){
	//worldPos=vec3(umat.modelMatrix*vec4(inPosition,1.));
	worldPos=inPosition;
	gl_Position=ubo.proj*ubo.view*skyboxMat*vec4(inPosition,1.);
}
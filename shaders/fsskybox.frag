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

layout(set=0,binding=0)uniform sampler2D skybox;
layout(set=0,binding=1)uniform sampler2D mrtTex;
layout(set=0,binding=2)uniform sampler2D normTex;
layout(set=0,binding=3)uniform sampler2D emissTex;
layout(set=0,binding=4)uniform sampler2D occuTex;
layout(location=0)in vec3 worldPos;

layout(location=0)out vec4 outColor;
#define PI 3.14159265359

vec3 ACESToneMapping(vec3 color,float adapted_lum)
{
	const float A=2.51f;
	const float B=.03f;
	const float C=2.43f;
	const float D=.59f;
	const float E=.14f;
	
	color*=adapted_lum;
	return(color*(A*color+B))/(color*(C*color+D)+E);
}

vec2 SampleSphericalMap(vec3 v){
	vec2 uv=vec2(atan(v.z,v.x),asin(v.y));
	uv/=vec2(2.*PI,PI);
	uv+=.5;
	uv.y=1.-uv.y;
	return uv;
}

void main(){
	vec3 sampleVector=worldPos-ubo.cameraPos;
	vec2 uv=SampleSphericalMap(normalize(worldPos));
	vec3 envColor=texture(skybox,uv).xyz;
	envColor=ACESToneMapping(envColor,1.5f);
	outColor=vec4(envColor,1.f);
	
	//outColor=vec4(vec3(.2471,.7765,.9882),1.f);
}
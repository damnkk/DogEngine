#version 450

#extension GL_EXT_nonuniform_qualifier:enable
#extension GL_EXT_scalar_block_layout:enable
layout(set=0,binding=0)uniform textureIndex{
    int idx;
}TXIdx;

layout(set=1,binding=0)uniform sampler2D textureArray[];

layout(location=0) in vec2 inUV;
layout(location=0)out vec4 outColor;

const float PI = 3.1415926535897932384626433832795;
const uint sampleCount = 4096;

float random(vec2 co){
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy,vec2(a,b));
    float sn = mod(dt,3.14);
    return fract(sin(sn)*c);
}

vec2 hammersley2d(uint i, uint N) 
{
	// Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) /float(N), rdi);
}

vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal) 
{
	// Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha*alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

vec3 prefilterEnvMap(vec3 R, float roughness){
    vec3 N = R;
    vec3 V = R;
    vec3 color = vec3(0.0);
    float totalWeight = 0.0;
    //float envMapDim = float()
    for(uint i = 0;i<sampleCount;++i){
        vec2 xi = hammersley2d(i,sampleCount);
        vec3 H = importanceSample_GGX(xi, roughness,N);
        vec3 L = 2.0*dot(V,H)*H-V;
        float dotNL = clamp(dot(N,L),0.0,1.0);
        if(dotNL>0.0){
            float dotNH = clamp(dot(N,H),0.0,1.0);
            float dotVH = clamp(dot(V,H),0.0,1.0);

            float pdf = D_GGX(dotNH,roughness)*dotNH/(4.0*dotVH)+0.0001;
            float omegaS = 1.0/(float(sampleCount)*pdf);
            //float omegaP = 4.0*PI/(6.0*);
        }
    }
    return vec3(1.0f);
}


void main(){
    vec3 color = texture(textureArray[nonuniformEXT(TXIdx.idx)],inUV).xyz;
    outColor=vec4(color,1.f);
}
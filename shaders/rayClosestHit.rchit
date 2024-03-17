#version 460

#extension GL_EXT_ray_tracing:require
#extension GL_EXT_nonuniform_qualifier :enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_GOOGLE_include_directive :enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64  :require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference :require

#include "shaderDataStructure.h"
#include "shaderUtil.glsl"

hitAttributeEXT vec2 attribs;
layout(location = 0) rayPayloadInEXT hitPayLoad prd;

layout(buffer_reference, scalar) buffer Vertices{Vertex v[];};
layout(buffer_reference, std430,buffer_reference_align = 4) buffer Indices{uint i[];};
//忘记了storage在着色器当中默认的布局就是std430,所以你啥也不写肯定是错位的
layout(buffer_reference, std140) buffer Materials{Material m[];};
layout(buffer_reference, scalar) buffer MatIndices{uint i[];};
layout(set = 0,binding= 0) uniform accelerationStructureEXT topLevelAS;
//这个objDescs没有任何问题
layout(set = 1,binding=1,scalar) buffer ObjectDescs{ObjDesc i[];}objDescs;
layout(set = 2,binding = 0) uniform sampler2D bindlessTextures[];
layout(set = 1,binding = 0) uniform cameraTransform{
    vec3 cameraPos;
    vec3 cameraDirection;
    mat4 viewMatrix;
    mat4 projMatrix;
} cameraTrans;

layout(push_constant,scalar) uniform RtPushConstant{
    int frameCount;
    int maxBound;
    vec4 clearColor;
}rtConst;

 const float PI = 3.14159265358979;
 const int k_invalid_index = -1;

/*
Diffuse  0
MRT      1
AO       2
Nomral   3
emissive 4
*/

struct MaterialParam{
    vec3 baseColor;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
};

vec3 toHamis(vec3 dir, vec3 N){
    vec3 helper = vec3(1.0f,0.0f,0.0f);
    vec3 tagent = normalize(cross(helper,N));
    vec3 bitTagent = normalize(cross(N,tagent));
    return normalize(dir.x*tagent+dir.y*bitTagent+dir.z*N);
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

vec3 random_pcg3d(uvec3 v) {
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return vec3(v) * (1.0/float(0xffffffffu));
}


vec3 diffuseImportanceSample( vec3 N){
    vec3 dir = random_pcg3d(uvec3(rtConst.frameCount+prd.recursiveDepth,gl_LaunchIDEXT.xy));
    // vec3 help = vec3(1.0f,0.0,0.0);
    // vec3 tagent = normalize(cross(N,help));
    // vec3 bitTagent = normalize(cross(tagent,N));

    return dir;
}

vec3 specularImportanceSample(){
    return vec3(0.0f,0.0f,1.0f);
}

float sqr(float x){
    return x*x;
}

float SchlickFresnel(float u){
    float m = clamp(1-u,0,1);
    float m2 = m*m;
    return m2*m2*m;
}

float GTR1(float NdotH,float a)
{//两种法线分布项
    if(a>=1) return 1/PI;
    float a2 = a*a;
    float t = 1+(a2-1)*NdotH*NdotH;
    return (a2-1)/(PI*log(a2)*t);
}

float GTR2(float NdotH,float a)
{//两种法线分布项
    float a2 = a*a;
    float t = 1+(a2-1)*NdotH*NdotH;
    return a2/(PI*t*t);
}

float smithG_GGX(float NdotV,float alphaG)
{
    float a = alphaG*alphaG;
    float b = NdotV*NdotV;
    return 1/(NdotV+sqrt(a+b-a*b));
}



vec3 brdfEvaluate(vec3 V, vec3 N, vec3 L, MaterialParam mat){
    float NdotL = dot(N,L);
    float NdotV = dot(N,V);
    if(NdotL<0||NdotV<0) return vec3(0);
    vec3 H = normalize(L+V);
    float NdotH = dot(N,H);
    float LdotH = dot(L,H);
    //diffuse 
    float Fd90 = 0.5+2.0*mat.roughness*LdotH*LdotH;
    float FL = SchlickFresnel(NdotL);
    float FV = SchlickFresnel(NdotV);
    float Fd = mix(1.0,Fd90,FL)*mix(1.0,Fd90,FV);
    //subface refrect
    float Fss90 = LdotH*LdotH*mat.roughness;
    float Fss = mix(1.0,Fss90,FL)*mix(1.0,Fss90,FV);
    float ss = 1.25*(Fss*(1.0/(NdotL+NdotV)-0.5)+0.5);
    //specular
    float alpha = max(0.001,sqr(mat.roughness));
    float Ds = GTR2(NdotH,alpha);
    vec3 F0 = mix(vec3(0.04),mat.baseColor,mat.metallic);
    float  FH = SchlickFresnel(LdotH);
    vec3 Fs = mix(vec3(0.0),F0,FH);
    float Gs = smithG_GGX(NdotL,mat.roughness);
    Gs*=smithG_GGX(NdotL,mat.roughness);

    vec3 diffuse = (1.0/PI)*Fd*mat.baseColor;
    vec3 specular = Gs*Fs*Ds;
    return diffuse*(1.0-mat.metallic)+specular;

}

float pdfEvaluate(vec3 V, vec3 N, vec3 L, MaterialParam mat){
    float NdotL = dot(N,L);
    float NdotV = dot(N,V);
    if(NdotL<0||NdotV<0) return 0;
    vec3 H = normalize(L+V);
    float NdotH = dot(N,H);
    float LdotH = dot(L,H);
    float pdfDiffuse = NdotL/PI;
    return max(0.001,pdfDiffuse);
}

void main(){
    ObjDesc objResource = objDescs.i[gl_InstanceCustomIndexEXT];
    MatIndices matIndices = MatIndices(objResource.primitiveMatIdxAddress);
    Indices indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);
    Materials materials = Materials(objResource.materialAddress);

    uint idx0 = indices.i[gl_PrimitiveID*3];
    uint idx1 = indices.i[gl_PrimitiveID*3+1];
    uint idx2 = indices.i[gl_PrimitiveID*3+2];

    Vertex v0 = vertices.v[idx0];
    Vertex v1 = vertices.v[idx1];
    Vertex v2 = vertices.v[idx2];

    const vec3 barycentrics = vec3(1.0-attribs.x-attribs.y,attribs.x,attribs.y);
    const vec3 hitPos = v0.vPos*barycentrics.x+v1.vPos*barycentrics.y+v2.vPos*barycentrics.z;
    const vec3 hitWorldPos = vec3(gl_ObjectToWorldEXT* vec4(hitPos,1.0f));
    const vec3 hitNormal = v0.vNormal*barycentrics.x+v1.vNormal*barycentrics.y+v2.vNormal*barycentrics.z;
    const vec3 hitWorldNormal = normalize(vec3(hitNormal*gl_WorldToObjectEXT));
    const vec2 hitTexcoord = v0.vTexcoord*barycentrics.x+v1.vTexcoord*barycentrics.y+v2.vTexcoord*barycentrics.z;

    uint matIdx = matIndices.i[gl_PrimitiveID];
    Material mat = materials.m[matIdx];
    //diffuse
    vec3 diffuse = mat.baseColorFactor.xyz;
    if (mat.textureIndices[0] != k_invalid_index && mat.textureUseSetting[0] > 0) {
        diffuse = texture(bindlessTextures[nonuniformEXT(mat.textureIndices[0])],hitTexcoord).xyz;
    }
    //MRT
    float metallic = mat.mrFactor.x;
    float roughness = mat.mrFactor.y;
    if (mat.textureIndices[1] != k_invalid_index && mat.textureUseSetting[2] > 0) {
        metallic*= texture(bindlessTextures[nonuniformEXT(mat.textureIndices[1])],hitTexcoord).z;
        roughness *= texture(bindlessTextures[nonuniformEXT(mat.textureIndices[1])],hitTexcoord).y;
    }
    //AO
    float ao = 1.0f;
    if (mat.textureIndices[2] != k_invalid_index) {
        ao = texture(bindlessTextures[nonuniformEXT(mat.textureIndices[2])],hitTexcoord).r;
    }
    //Normal
    vec3 texNormal  = vec3(0,0,1);
    if (mat.textureIndices[3] != k_invalid_index && mat.textureUseSetting[1] > 0) {
        texNormal = normalize(texture(bindlessTextures[nonuniformEXT(mat.textureIndices[3])],hitTexcoord).xyz*2.0-1.0f);
    }
    //emissive
    vec3 emissive = mat.emissiveFactor;
    if (mat.textureIndices[4] != k_invalid_index && mat.textureUseSetting[3] > 0) {
        emissive *= texture(bindlessTextures[nonuniformEXT(mat.textureIndices[4])],hitTexcoord).xyz;
    }

    MaterialParam matParam;
    matParam.baseColor = diffuse;
    matParam.metallic = metallic;
    matParam.roughness = roughness;
    matParam.ao = ao;
    matParam.emissive = emissive;

    vec3 V = normalize(prd.direction);
    vec3 N = hitWorldNormal;
    if(mat.textureIndices[3] != k_invalid_index && mat.textureUseSetting[1] > 0){
        N = toHamis(texNormal,hitWorldNormal);
    }

    vec3 tangent, bitangent;
    createCoordinateSystem(N, tangent, bitangent);

    vec3 L = samplingHemisphere(prd.seed, tangent, bitangent, N);
    float consineTheta = dot(N,L);
    vec3 brdf = matParam.baseColor/PI;
    float pdf  = consineTheta /  PI;
    prd.hitValue = emissive;
    prd.history =consineTheta*brdf/max(0.001,pdf);
    prd.origin = hitWorldPos;
    prd.direction = L;
    prd.lastNormal = N;
    //prd.hitValue =N;
    //prd.hitValue =emissive;
    // prd.recursiveDepth = 100;
    return;
}


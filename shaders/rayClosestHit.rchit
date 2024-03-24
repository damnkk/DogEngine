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
layout(location = 1) rayPayloadEXT hitPayLoad envSamplePrd;

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
    int skyTextureBindlessIdx;
    vec4 clearColor;
}rtConst;

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

 vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy)+vec2(0.5);
 vec2 pix = pixelCenter/vec2(gl_LaunchSizeEXT.xy);

uint seed = uint(
    uint((pix.x * 0.5 + 0.5) * gl_LaunchSizeEXT.x)  * uint(1973) + 
    uint((pix.y * 0.5 + 0.5) * gl_LaunchSizeEXT.y) * uint(9277) + 
    uint(rtConst.frameCount) * uint(26699)) | uint(1);

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}
 
float rand() {
    return float(wang_hash(seed)) / 4294967296.0;
}

int grayCode(int i)
{
    return i^(i>>1);
}
// 生成第 d 维度的第 i 个 sobol 数
float sobol(int d,int i)
{
    uint result = 0u;
    int offset = 32*d;
    for(int j = 0;i!=0;i>>=1,j++)
    {
        if((i&1)!=0)
        {
            result^=sobolMatrix[offset+j];
        }
    }
    return float(result)*(1.0f/float(0xFFFFFFFFU));
}

vec2 sobolVec2(int i,int b)
{
    float u = sobol(b*2,grayCode(i));
    float v = sobol(b*2+1,grayCode(i));
    return vec2(u,v);
}

vec2 CranleyPattersonRotation(vec2 p)
{
    uint pseed = uint(
    uint((pix.x*0.5+0.5)*gl_LaunchSizeEXT.x) *uint(1973)+
    uint((pix.y*0.5+0.5)*gl_LaunchSizeEXT.y)*uint(9277)+
    uint(114514/1919)*uint(26699))|uint(1);
    float u  =float(wang_hash(pseed))/4294967296.0;
    float v = float(wang_hash(pseed))/4294967296.0;
    p.x +=u ;
    if(p.x>1) p.x-=1;
    if(p.x<0) p.x+=1;
    p.y +=v;
    if(p.y>1) p.y-=1;
    if(p.y<0) p.y+=1;

    return p;
}

vec3 toHamis(vec3 dir, vec3 N){
    vec3 helper = vec3(1.0f,0.0f,0.0f);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tagent = normalize(cross(helper,N));
    vec3 bitTagent = normalize(cross(tagent,N));
    return dir.x*tagent+dir.y*bitTagent+dir.z*N;
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


vec3 diffuseImportanceSample(float xi_1,float xi_2,vec3 N){
    float r = sqrt(xi_1);
    float theta = xi_2*2.0*PI;
    float x = r*cos(theta);
    float y = r*sin(theta);
    float z = sqrt(1.0-x*x-y*y);

    //从z半球投影到法相半球
    vec3 L = toHamis(vec3(x,y,z),N);
    return L;
}

vec3 specularImportanceSample(float xi_i,float xi_2,vec3 V,vec3 N,float alpha){
    float phi_h = 2.0*PI*xi_i;
    float sin_phi_h = sin(phi_h);
    float cos_phi_h = cos(phi_h);

    float cos_theta_h = sqrt((1.0-xi_2)/(1.0+(alpha*alpha-1.0)*xi_2));
    float sin_theta_h = sqrt(max(0.0,1.0-cos_theta_h*cos_theta_h));

    vec3 H = normalize(vec3(sin_theta_h*cos_phi_h,sin_theta_h*sin_phi_h,cos_theta_h));
    H = toHamis(H,N);
    vec3 L = reflect(-V,H);
    return L;
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
    vec3 Fs = mix(F0,vec3(1),FH);
    float Gs = smithG_GGX(NdotL,mat.roughness);
    Gs*=smithG_GGX(NdotL,mat.roughness);

    vec3 diffuse = (1.0/PI)*Fd*mat.baseColor;
    vec3 specular = Gs*Fs*Ds;
    return diffuse*(1.0-mat.metallic)+specular;
}

vec3 SampleBrdf(float xi_1,float xi_2,float xi_3,vec3 V,vec3 N, MaterialParam mat){
    float alpha_GTR2 = max(0.001,sqr(mat.roughness));
    float r_diffuse = (1.0-mat.metallic);
    float r_specular = 1.0;
    float r_sum = r_diffuse+r_specular;

    float p_diffuse = r_diffuse/r_sum;
    float p_specular = r_specular/r_sum;
    float rd = xi_3;
    if(rd<=p_diffuse){
        return diffuseImportanceSample(xi_1,xi_2,N);
    }else if(rd>p_diffuse&&rd<=p_diffuse+p_specular){
        return specularImportanceSample(xi_1,xi_2,V,N,alpha_GTR2);
    }
    //return specularImportanceSample(xi_1,xi_2,V,N,alpha_GTR2);
    return vec3(0,1,0);
}

float pdfEvaluate(vec3 V, vec3 N, vec3 L, MaterialParam mat){
    float NdotL = dot(N,L);
    float NdotV = dot(N,V);
    if(NdotL<0||NdotV<0) return 100000;
    vec3 H = normalize(L+V);
    float NdotH = dot(N,H);
    float LdotH = dot(L,H);

    float alpha_Ds = max(0.001,sqr(mat.roughness));
    float Ds = GTR2(NdotH,alpha_Ds);
    
    float pdfDiffuse = NdotL/PI;
    float pdfSpecular = Ds*NdotH/(4.0*dot(L,H));

    float rDiffuse = (1.0-mat.metallic);
    float rSpecular = 1.0;
    float rSum = rDiffuse+rSpecular;

    float pDiffuse = rDiffuse/rSum;
    float pSpecular = rSpecular/rSum;

    float pdf = pDiffuse*pdfDiffuse+pSpecular*pdfSpecular;
     //pdf = pdfSpecular;
    return max(0.00001,pdf);
}

vec3 SampleHdr(float x1,float x2,int HDRTexIdx){
    vec2 xy = texture(bindlessTextures[nonuniformEXT(HDRTexIdx)],vec2(x1,x2)).xy;
    xy.y = 1.0-xy.y;
    float phi = 2.0*PI*(xy.x-0.5);
    float theta = PI*(xy.y-0.5);
    vec3 L = vec3(cos(theta)*cos(phi),sin(theta),cos(theta)*sin(phi));
    return L;
}

float hdrPdf(vec3 L,int hdrResolution,int HDRTexIdx){
    vec2 uv = toSphericalCoord(normalize(L));
    float pdf =texture(bindlessTextures[nonuniformEXT(HDRTexIdx)],uv).z;
    float theta = PI*(0.5-uv.y);
    float sin_theta = max(sin(theta),1e-10);
    float p_convert = float(hdrResolution*hdrResolution/2)/(2.0*PI*PI*sin_theta);
    return pdf*p_convert;
}

float misMixWeight(float a ,float b){
    float t = a*a;
    return t/(b*b+t);
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

    vec3 V = -normalize(prd.direction);
    vec3 N = hitWorldNormal;
    if(mat.textureIndices[3] != k_invalid_index && mat.textureUseSetting[1] > 0){
        N = toHamis(texNormal,hitWorldNormal);
    }

     N *=(1.-2.*step(0.0f,dot(prd.direction,N)));
    vec2 uv = sobolVec2(rtConst.frameCount+1,prd.recursiveDepth);
    uv = CranleyPattersonRotation(uv);
    vec3 L = SampleBrdf(uv.x,uv.y,rnd(prd.seed),V,N,matParam);
    float consineTheta = dot(N,L);
    vec3 brdf = brdfEvaluate(V,N,L,matParam);
    float pdf = pdfEvaluate(V,N,L,matParam);

    // env sample
    vec3 envL = SampleHdr(rand(),rand(),rtConst.skyTextureBindlessIdx);
    vec3 test = vec3(1.0f,0.0f,0.0f);
    if(dot(N,envL)>0.0){
        test = vec3(0.0f,1.0f,0.0f);
        envSamplePrd.recursiveDepth = 0;
        envSamplePrd.envSample = 1;
        envSamplePrd.origin = hitWorldPos;
        envSamplePrd.direction = envL;
        envSamplePrd.hitValue = vec3(0.0f,0.0f,0.0);
        float tMin = 0.0001;
        float tMax = 10000000.0;
        uint rayFlags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;

        traceRayEXT(topLevelAS,
            rayFlags,
            0xFF,
            0,0,1,envSamplePrd.origin,tMin,envSamplePrd.direction, tMax,1);
        float envPdf = hdrPdf(envL,2048,rtConst.skyTextureBindlessIdx);
        vec3 envBrdf = brdfEvaluate(V,N,envL,matParam);

        float misWeight = misMixWeight(envPdf,pdf);
        prd.hitValue = misWeight*envSamplePrd.hitValue*envBrdf*dot(N,envL)/envPdf;
    }

//material sample
    prd.hitValue += emissive;
    prd.hitValue = emissive;
    prd.weight =consineTheta*brdf/max(0.0000001,pdf);
    prd.origin = hitWorldPos;
    prd.direction = L;
    prd.lastNormal = N;
    prd.brdf = brdf;
    prd.pdf = pdf;

    // prd.hitValue = vec3(roughness);
    // prd.recursiveDepth = 100;
    return;
}


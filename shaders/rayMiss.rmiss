#version 460
#extension GL_EXT_ray_tracing:require
#extension GL_EXT_nonuniform_qualifier:enable
#extension GL_EXT_scalar_block_layout:enable
#extension GL_GOOGLE_include_directive:enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64:require
#extension GL_EXT_buffer_reference2:require
#extension GL_EXT_buffer_reference:require
#include "shaderDataStructure.h"
#include "shaderUtil.glsl"
layout(location=0)rayPayloadInEXT hitPayLoad res;
layout(location=1)rayPayloadInEXT hitPayLoad envRes;

layout(buffer_reference,scalar)buffer Vertices{Vertex v[];};
layout(buffer_reference,std430,buffer_reference_align=4)buffer Indices{uint i[];};
//忘记了storage在着色器当中默认的布局就是std430,所以你啥也不写肯定是错位的
layout(buffer_reference,std140)buffer Materials{Material m[];};
layout(buffer_reference,scalar)buffer MatIndices{uint i[];};
layout(set=0,binding=0)uniform accelerationStructureEXT topLevelAS;
//这个objDescs没有任何问题
layout(set=1,binding=1,scalar)buffer ObjectDescs{ObjDesc i[];}objDescs;
layout(set=2,binding=0)uniform sampler2D bindlessTextures[];
layout(set=1,binding=0)uniform cameraTransform{
    vec3 cameraPos;
    vec3 cameraDirection;
    mat4 viewMatrix;
    mat4 projMatrix;
}cameraTrans;

layout(push_constant,scalar)uniform RtPushConstant{
    int frameCount;
    int maxBound;
    int skyTextureBindlessIdx;
    vec4 clearColor;
}rtConst;

//const float PI = 3.14159265358979;

vec2 directoryToUV(in vec3 cameraDirect){
    vec3 direct=normalize(cameraDirect);
    vec2 uv=vec2(atan(direct.z,direct.x),asin(direct.y));
    uv/=vec2(2.*PI,PI);
    uv+=.5;
    uv.y=1.-uv.y;
    return uv;
}

vec3 SampleHdr(float x1,float x2,int HDRTexIdx){
    vec2 xy=texture(bindlessTextures[nonuniformEXT(HDRTexIdx)],vec2(x1,x2)).xy;
    xy.y=1.-xy.y;
    float phi=2.*PI*(xy.x-.5);
    float theta=PI*(xy.y-.5);
    vec3 L=vec3(cos(theta)*cos(phi),sin(theta),cos(theta)*sin(phi));
    return L;
}

float hdrPdf(vec3 L,int hdrResolution,int HDRTexIdx){
    vec2 uv=toSphericalCoord(normalize(L));
    float pdf=texture(bindlessTextures[nonuniformEXT(HDRTexIdx)],uv).z;
    float theta=PI*(.5-uv.y);
    float sin_theta=max(sin(theta),1e-10);
    float p_convert=float(hdrResolution*hdrResolution/2)/(2.*PI*PI*sin_theta);
    return pdf*p_convert;
}

float misMixWeight(float a,float b){
    float t=a*a;
    return t/(b*b+t);
}

vec3 lightDir=vec3(1.f,1.f,0.f);
vec3 lightColor=vec3(0.,0.,0.);

void main(){
    vec2 uv=directoryToUV(res.direction);
    vec3 gamma=vec3(1./2.2);
    //envRes.hitValue = texture(bindlessTextures[nonuniformEXT(rtConst.skyTextureBindlessIdx)],uv).xyz;
    
    if(res.recursiveDepth<=0){
        res.hitValue=pow(texture(bindlessTextures[nonuniformEXT(rtConst.skyTextureBindlessIdx)],uv).xyz,vec3(1./1.6));
    }else{
        vec3 color=min(vec3(1000.),pow(texture(bindlessTextures[nonuniformEXT(rtConst.skyTextureBindlessIdx)],uv).xyz,gamma));
        float pdfLight=hdrPdf(res.direction,2048,rtConst.skyTextureBindlessIdx);
        float mis_weight=misMixWeight(pdfLight,res.pdf);
        res.hitValue=vec3(0);
        res.hitValue=min(vec3(1000.),pow(texture(bindlessTextures[nonuniformEXT(rtConst.skyTextureBindlessIdx)],uv).xyz,gamma));
        res.hitValue+=max(0.f,dot(res.lastNormal,lightDir))*lightColor;
        
    }
    res.recursiveDepth=1000;
}
#version 450

#extension GL_EXT_nonuniform_qualifier:enable
#extension GL_EXT_scalar_block_layout:enable

layout(set=0,binding=0)uniform ENVMapIdx{
    int index;
}ubo;

layout(location=0)in vec2 inUV;
layout(location=0)out vec4 outColor;
layout(set=1,binding=0)uniform sampler2D textureArray[];

const float PI=3.1415926535897932384626433832795;

vec3 TwoDimVectorTo3D(vec2 uv){
    vec3 outVec=vec3(1.f);
    outVec.y=cos(uv.y*PI);
    outVec.x=sin(uv.y*PI)*cos(uv.x*2.f*PI);
    outVec.z=sin(uv.y*PI)*sin(uv.x*2.f*PI);
    return normalize(outVec);
}

//the range is [0,1]^2
vec2 hammersley2d(uint i,uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits=(i<<16u)|(i>>16u);
    bits=((bits&0x55555555u)<<1u)|((bits&0xAAAAAAAAu)>>1u);
    bits=((bits&0x33333333u)<<2u)|((bits&0xCCCCCCCCu)>>2u);
    bits=((bits&0x0F0F0F0Fu)<<4u)|((bits&0xF0F0F0F0u)>>4u);
    bits=((bits&0x00FF00FFu)<<8u)|((bits&0xFF00FF00u)>>8u);
    float rdi=float(bits)*2.3283064365386963e-10;
    return vec2(float(i)/float(N),rdi);
}

vec2 SampleSphericalMap(vec3 v){
    vec2 uv=vec2(atan(v.z,v.x),asin(v.y));
    uv/=vec2(2.*PI,PI);
    uv+=.5;
    uv.y=1.-uv.y;
    return uv;
}

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

void main()
{
    vec3 N=TwoDimVectorTo3D(inUV);
    vec3 up=vec3(0.,1.,0.);
    vec3 right=normalize(cross(up,N));
    up=normalize(cross(N,right));
    
    const float TWO_PI=PI*2.;
    const float HALF_PI=PI*.5;
    vec3 color=vec3(0.);
    uint sampleCount=2048u;
    for(uint i=0;i<sampleCount;++i){
        vec2 vdc=hammersley2d(int(i),sampleCount);
        float phi=vdc.y*2.*PI;
        float cosTheta=sqrt(1.-vdc.x);
        float sinTheta=sqrt(1.-cosTheta*cosTheta);
        vec3 sampleVectorInNormalSpace=vec3(cos(phi)*sinTheta,sin(phi)*sinTheta,cosTheta);
        vec3 sampleVector=sampleVectorInNormalSpace.x*right+sampleVectorInNormalSpace.y*up+sampleVectorInNormalSpace.z*N;
        vec2 UV=SampleSphericalMap(sampleVector);
        color+=texture(textureArray[nonuniformEXT(ubo.index)],UV).xyz*cosTheta*sinTheta;
    }
    
    outColor=vec4(PI*color/float(sampleCount),1.);
}
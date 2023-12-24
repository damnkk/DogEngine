#version 450

layout(location=0)in vec2 inUV;
layout(location=0)out vec4 outColor;
layout(constant_id=0)const uint NUM_SAMPLES=1024u;

const float PI=3.1415926535897932384626433832795;

float random(vec2 co){
    float a=12.9898;
    float b=78.233;
    float c=43758.5453;
    float dt=dot(co.xy,vec2(a,b));
    float sn=mod(dt,3.14);
    return fract(sin(sn)*c);
}

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

vec3 importanceSample_GGX(vec2 Xi,float roughness,vec3 normal){
    float alpha=roughness*roughness;
    float phi=2.*PI*Xi.x+random(normal.xz)*.1;
    float cosTheta=sqrt((1.-Xi.y)/(1.+(alpha*alpha-1.)*Xi.y));
    float sinTheta=sqrt(1.-cosTheta*cosTheta);
    vec3 H=vec3(sinTheta*cos(phi),sinTheta*sin(phi),cosTheta);
    
    vec3 up=abs(normal.z)<.999?vec3(0.,0.,1.):vec3(1.,0.,0.);
    vec3 tangentX=normalize(cross(up,normal));
    vec3 tangentY=normalize(cross(normal,tangentX));
    return normalize(tangentX*H.x+tangentY*H.y+normal*H.z);
}

float G_SchlicksmithGGX(float dotNL,float dotNV,float roughness){
    float k=(roughness*roughness)/2.;
    float GL=dotNL/(dotNL*(1.-k)+k);
    float GV=dotNV/(dotNV*(1.-k)+k);
    return GL*GV;
}

vec2 BRDF(float NoV,float roughness){
    const vec3 N=vec3(0.,0.,1.f);
    vec3 V=vec3(sqrt(1.f-NoV*NoV),0.,NoV);
    vec2 LUT=vec2(0.f);
    for(uint i=0u;i<NUM_SAMPLES;++i){
        //得到一个低差异序列采样点
        vec2 Xi=hammersley2d(i,NUM_SAMPLES);
        //得到三维采样向量
        vec3 H=importanceSample_GGX(Xi,roughness,N);
        vec3 L=2.*dot(V,H)*H-V;
        
        float dotNL=max(dot(N,L),0.f);
        float dotNV=max(dot(N,V),0.f);
        float dotVH=max(dot(V,H),0.f);
        float dotNH=max(dot(H,N),0.f);
        
        if(dotNL>0.f){
            float G=G_SchlicksmithGGX(dotNL,dotNV,roughness);
            float G_Vis=(G*dotVH)/(dotNH*dotNV);
            float Fc=pow(1.-dotVH,5.);
            LUT+=vec2((1.-Fc)*G_Vis,Fc*G_Vis);
        }
    }
    return LUT/float(NUM_SAMPLES);
}

void main()
{
    outColor=vec4(BRDF(inUV.s,inUV.t),0.,1.);
}
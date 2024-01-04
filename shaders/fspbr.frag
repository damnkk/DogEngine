#version 450

#extension GL_EXT_nonuniform_qualifier:enable
#extension GL_EXT_scalar_block_layout:enable
layout(binding=0)uniform UniformBufferObject{
    vec3 cameraPos;
    vec3 cameraDirectory;
    mat4 view;
    mat4 proj;
}ubo;

layout(binding=1)uniform UniformMaterialObject{
    mat4 modelMatrix;
    vec4 baseColorFactor;
    vec3 emissiveFactor;
    vec3 tueFactor;
    vec3 mrFactor;
    int textureIndices[];
}umat;

/*
Diffuse  0
MRT      1
AO       2
Nomral   3
emissive 4
*/

layout(set=1,binding=0)uniform sampler2D globalTextures[];

layout(location=0)in vec3 fragNormal;
layout(location=1)in vec2 fragTexCoord;
layout(location=2)in vec3 worldPos;

layout(location=0)out vec4 outColor;
#define PI 3.14159265359

vec3 lightdirection=vec3(0.f,200.f,00.f);

vec2 directoryToUV(in vec3 cameraDirect){
    vec3 direct=normalize(cameraDirect);
    vec2 uv=vec2(atan(direct.z,direct.x),asin(direct.y));
    uv/=vec2(2.*PI,PI);
    uv+=.5;
    uv.y=1.-uv.y;
    return uv;
}

float heaviside(float v){
    if(v>0.)return 1.;
    else return 0.;
}

vec3 addNormTex(vec3 initNormal,vec2 fragTexCoord){
    mat3 TBN=mat3(1.);
    
    vec3 T=normalize(cross(initNormal,vec3(1.,0,0)));
    vec3 B=normalize(cross(initNormal,T));
    
    // the transpose of texture-to-eye space matrix
    TBN=mat3(
        T,
        B,
        normalize(initNormal)
    );
    
    vec3 N=normalize(texture(globalTextures[nonuniformEXT(umat.textureIndices[3])],fragTexCoord).rgb*2.-1.);
    N=normalize(TBN*N);
    return N;
    
}

vec3 decode_srgb(vec3 c){
    vec3 result;
    if(c.r<=.04045){
        result.r=c.r/12.92;
    }else{
        result.r=pow((c.r+.055)/1.055,2.4);
    }
    
    if(c.g<=.04045){
        result.g=c.g/12.92;
    }else{
        result.g=pow((c.g+.055)/1.055,2.4);
    }
    
    if(c.b<=.04045){
        result.b=c.b/12.92;
    }else{
        result.b=pow((c.b+.055)/1.055,2.4);
    }
    
    return clamp(result,0.,1.);
}

vec3 encode_srgb(vec3 c){
    vec3 result;
    if(c.r<=.0031308){
        result.r=c.r*12.92;
    }else{
        result.r=1.055*pow(c.r,1./2.4)-.055;
    }
    
    if(c.g<=.0031308){
        result.g=c.g*12.92;
    }else{
        result.g=1.055*pow(c.g,1./2.4)-.055;
    }
    
    if(c.b<=.0031308){
        result.b=c.b*12.92;
    }else{
        result.b=1.055*pow(c.b,1./2.4)-.055;
    }
    
    return clamp(result,0.,1.);
}

// D
float Trowbridge_Reitz_GGX(float NdotH,float a)
{
    float a2=a*a;
    float NdotH2=NdotH*NdotH;
    
    float nom=a2;
    float denom=(NdotH2*(a2-1.)+1.);
    denom=PI*denom*denom;
    
    return nom/denom;
}

// F
vec3 SchlickFresnel(float HdotV,vec3 F0)
{
    float m=clamp(1-HdotV,0,1);
    float m2=m*m;
    float m5=m2*m2*m;// pow(m,5)
    return F0+(1.-F0)*m5;
}

// G
float SchlickGGX(float NdotV,float k)
{
    float nom=NdotV;
    float denom=NdotV*(1.-k)+k;
    
    return nom/denom;
}

vec3 PBR(vec3 N,vec3 V,vec3 L,vec3 albedo,vec3 radiance,float roughness,float metallic)
{
    roughness=max(roughness,.05);// 保证光滑物体也有高光
    
    vec3 H=normalize(L+V);
    float NdotL=max(dot(N,L),0);
    float NdotV=max(dot(N,V),0);
    float NdotH=max(dot(N,H),0);
    float HdotV=max(dot(H,V),0);
    float alpha=roughness*roughness;
    float k=alpha/2.;
    vec3 F0=mix(vec3(.04,.04,.04),albedo,metallic);
    
    float D=Trowbridge_Reitz_GGX(NdotH,alpha);
    vec3 F=SchlickFresnel(HdotV,F0);
    float G=SchlickGGX(NdotV,k)*SchlickGGX(NdotL,k);
    
    vec3 k_s=F;
    vec3 k_d=(1.-k_s)*(1.-metallic);
    vec3 f_diffuse=albedo/PI;
    vec3 f_specular=(D*F*G)/(4.*NdotV*NdotL+.0001);
    f_diffuse*=PI;
    //f_specular*=PI;
    
    vec3 color=(k_d*f_diffuse+f_specular)*radiance*NdotL;
    
    return color;
}

void main(){
    
    vec3 test=texture(globalTextures[nonuniformEXT(umat.textureIndices[5])],fragTexCoord).xyz;
    vec3 test2=texture(globalTextures[nonuniformEXT(umat.textureIndices[6])],fragTexCoord).xyz;
    vec3 test3=texture(globalTextures[nonuniformEXT(umat.textureIndices[7])],fragTexCoord).xyz;
    vec3 V=normalize(ubo.cameraPos-worldPos);
    vec3 L=normalize(lightdirection-worldPos);
    vec3 N=normalize(fragNormal);
    N=addNormTex(N,fragTexCoord);
    vec3 H=normalize(L+V);
    float roughness=texture(globalTextures[nonuniformEXT(umat.textureIndices[1])],fragTexCoord).y;
    float metallic=texture(globalTextures[nonuniformEXT(umat.textureIndices[1])],fragTexCoord).z;
    vec4 baseColor=vec4(decode_srgb(texture(globalTextures[nonuniformEXT(umat.textureIndices[0])],fragTexCoord).xyz),texture(globalTextures[nonuniformEXT(umat.textureIndices[0])],fragTexCoord).a);
    vec3 emission=vec3(decode_srgb(texture(globalTextures[nonuniformEXT(umat.textureIndices[4])],fragTexCoord).xyz));
    vec3 color=PBR(N,V,L,baseColor.xyz,vec3(2.f),roughness,metallic)+emission;
    //color=vec3(texture(globalTextures[nonuniformEXT(umat.textureIndices[2])],fragTexCoord));
    outColor=vec4(color,1.);
}
#version 450

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

layout(location=0)in vec3 inPosition;
layout(location=1)in vec3 inNormal;
layout(location=2)in vec4 inTangent;
layout(location=3)in vec2 inTexCoord;

layout(location=0)out vec3 fragNormal;
layout(location=1)out vec2 fragTexCoord;
layout(location=2)out vec4 outTangent;
layout(location=3)out vec3 worldPos;

void main(){
    gl_Position=ubo.proj*ubo.view*umat.modelMatrix*vec4(inPosition,1.);
    worldPos=vec3(umat.modelMatrix*vec4(inPosition,1.))/vec4(umat.modelMatrix*vec4(inPosition,1.)).w;
    //vec4 calcuTangent = length(inTangent)>0.001?inTangent:vec4(normalize(vec3(cross(vec3(0, 1, 0), inNormal))),1.0);
    vec3 helper = normalize(vec3(0.3,-0.5,0.7));
    if(dot(helper,inNormal)>0.999){
        helper = normalize(vec3(0,0,1));
        // if(dot(helper,inNormal)>0.99){
        //     helper = vec3(0,1,0);
        // }
    }
    vec4 calcuTangent = length(inTangent)>0.001?inTangent:vec4(normalize(vec3(cross(inNormal,helper))),1.0);
    calcuTangent = vec4(normalize(vec3(cross(inNormal,helper))),1.0);

    outTangent = vec4(mat3(transpose(inverse(umat.modelMatrix)))*calcuTangent.xyz,calcuTangent.w);
    fragNormal=normalize(mat3(transpose(inverse(umat.modelMatrix)))*inNormal);
    fragTexCoord=inTexCoord;
}
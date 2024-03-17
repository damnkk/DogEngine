#version 460
#extension GL_EXT_ray_tracing:require
#extension GL_EXT_nonuniform_qualifier :enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_GOOGLE_include_directive :enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64  :require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference :require
#include "shaderDataStructure.h"
layout(location = 0) rayPayloadInEXT hitPayLoad res;


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


vec3 lightDir = vec3(1.0f,1.0f,0.0f);
vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main(){
    if(res.recursiveDepth<=0){
        res.hitValue = vec3(1.0,0.0,1.0);
    }else{
        res.hitValue += res.history*dot(res.lastNormal,lightDir)*lightColor;
    }
res.recursiveDepth = 1000;
}
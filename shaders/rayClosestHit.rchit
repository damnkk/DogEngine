#version 460

#extension GL_EXT_ray_tracing:require
#extension GL_EXT_nonuniform_qualifier :enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_GOOGLE_include_directive :enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64  :require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference :require

#include "shaderDataStructure.h"

hitAttributeEXT vec2 attribs;
layout(location = 0) rayPayloadInEXT hitPayLoad prd;

layout(buffer_reference, scalar) buffer Vertices{Vertex v[];};
layout(buffer_reference, std430,buffer_reference_align = 4) buffer Indices{uint i[];};
layout(buffer_reference) buffer Materials{Material m[];};
layout(buffer_reference, scalar) buffer MatIndices{int i[];};
layout(set = 0,binding= 0) uniform accelerationStructureEXT topLevelAS;
//这个objDescs没有任何问题
layout(set = 1,binding=1,scalar) buffer ObjectDescs{ObjDesc i[];}objDescs;
layout(set = 2,binding = 0) uniform sampler2D bindlessTextures[];

layout(push_constant) uniform PushConstant{
    int frmaeCount;
    vec4 clearColor;
};

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
    const vec3 pos = v0.vPos*barycentrics.x+v1.vPos*barycentrics.y+v2.vPos*barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT* vec4(pos,1.0f));
    const vec2 hitTexcoord = v0.vTexcoord*barycentrics.x+v1.vTexcoord*barycentrics.y+v2.vTexcoord*barycentrics.z;

    int matIdx = matIndices.i[gl_PrimitiveID];
    Material mat = materials.m[matIdx];
     //diffuse
     vec3 diffuse = texture(bindlessTextures[nonuniformEXT(9)],hitTexcoord).xyz;

    prd.hitValue = vec3(diffuse);
    if(gl_PrimitiveID%2==0){
        prd.hitValue = vec3(1.0,0.0,0.0);
    }
    
}


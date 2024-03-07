#version 460

#extension GL_EXT_ray_tracing:require
#extension GL_EXT_nonuniform_qualifier :enable
#extension GL_EXT_scalar_block_layout :enable
#extension GL_GOOGLE_include_directive :enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64  :require
#extension GL_EXT_buffer_reference2 : require

#include "shaderDataStructure.h"

layout(buffer_reference, scalar) buffer Vertices{Vertex v[];};
layout(buffer_reference, scalar) buffer Indices{ivec3 i[];};
layout(buffer_reference, scalar) buffer Materials{Material m[];};
layout(buffer_reference, scalar) buffer MatIndices{int i[];};
layout(set = 0,binding= 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1,binding=1,scalar) buffer ObjectDescs{ObjDesc i[];};
layout(set = 2,binding = 0) uniform sampler2D bindlessTextures[];

void main(){

}


#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#include "shaderDataStructure.h"
layout(location = 0) rayPayloadInEXT vec3 res;

void main(){
    int a = 23;
}
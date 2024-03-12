#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#include "shaderDataStructure.h"
layout(location = 0) rayPayloadInEXT hitPayLoad res;

void main(){
    res.hitValue= vec3(0.6549, 0.0863, 0.9216);
}
#version 450

layout(location=0)out vec2 outUV;

void main()
{
    outUV=vec2((gl_VertexIndex<<1)&2,gl_VertexIndex&2);
    gl_Position=vec4(outUV*2.f-1.f,0.f,1.f);
}
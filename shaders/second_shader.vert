#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec2 outUV;

void main()
{
	//this is an odd and beautiful way to draw a full screen quard,we set vertex count as 3,but don't load any vertices from buffer, we just use
	//the gl_VertexIndex, cuz gl_Vertexindex is [0,1,2] at this time, we can get 3 vertex position like (0,0),(2,0),(0,2), then we think that there is
	//a quad in this triangle,which vertex position is (0,0)(1,0)(0,1)(1,1),we just convert this area to the range of [-1,1],then the part out of this quad
	//would be culled off.
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);

	gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);
}
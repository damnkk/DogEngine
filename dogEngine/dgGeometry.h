//
// Created by Administrator on 2023/12/17.
//

#ifndef VULKAN_LEARNING_DGGEOMETRY_H
#define VULKAN_LEARNING_DGGEOMETRY_H
#include <vector>
#include <Vertex.h>
namespace dg{
    std::vector<Vertex> planeVertexData = {
            {{-1.0f, 1.0f, 0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{-1.0f,-1.0f, 0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{ 1.0f,-1.0f, 0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{ 1.0f, 1.0f, 0.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},

    };

    std::vector<u32> planeIndexData = {
        0,1,2,0,2,3
    };

    std::vector<Vertex> cubeVertexData = {
            {{-1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{-1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{ 1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{ 1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{-1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{ 1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{ 1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{-1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{-1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{ 1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{ 1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{-1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{ 1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{ 1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{ 1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{ 1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{ 1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{-1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{-1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{ 1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},
            {{-1.0f,-1.0f,-1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,1.0f}},
            {{-1.0f,-1.0f, 1.0f},{0.0f,-1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,1.0f}},
            {{-1.0f, 1.0f, 1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{1.0f,0.0f}},
            {{-1.0f, 1.0f,-1.0f},{0.0f, 1.0f,0.0f},{0.0f,0.0f,0.0f,0.0f},{0.0f,0.0f}},

    };

    std::vector<u32> cubeIndexData = {
            0,1,2,2,3,0,4,5,6,6,7,4,8,9,10,10,11,8,12,13,14,14,15,12,16,17,18,18,19,16,20,21,22,22,23,20
    };

}// namespace dg;
#endif //VULKAN_LEARNING_DGGEOMETRY_H
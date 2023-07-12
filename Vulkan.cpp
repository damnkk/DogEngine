
//#include "Renderer.h"

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "vkRenderer.h"



int main(){

    VulkanRenderer vkRenderer;
    try{
        vkRenderer.initVulkan();
    }catch(std::exception e){
        std::cerr<<e.what()<<std::endl;
    }
    return 0;
}
#include "common.h"

class VulkanRenderer{
public:
    void fun();
    void initWindow();
    void initVulkan();
    void rendering();
    void clear();
    void createInstance();
    vk::Instance m_instance;
    vk::ApplicationInfo info;
    
    vk::Pipeline m_pipeline;
};
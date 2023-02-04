#include"Utilities.h"

//static member properties must be initialized, before static member func use them. And their 
//initialization should be done in cpp file.
MainDevice*		Utility::m_MainDevice		= nullptr;
VkSurfaceKHR*	Utility::m_Surface			= nullptr;
VkCommandPool*	Utility::m_CommandPool		= nullptr;
VkQueue*		Utility::m_GraphicsQueue	= nullptr;
VmaAllocator* Utility::m_allocator = nullptr;

void Utility::Setup(MainDevice* main_device, VkSurfaceKHR* surface, VkCommandPool* command_pool, VkQueue* queue, VmaAllocator* allocator){
    m_MainDevice	= main_device;
    m_Surface		= surface;
    m_CommandPool	= command_pool;
    m_GraphicsQueue = queue;
    m_allocator = allocator;
}

std::vector<char> Utility::ReadFile(const std::string& filename){
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
      throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

VkFormat Utility::findSupportedFormat(const std::vector<VkFormat>& candidate, VkImageTiling tiling, VkFormatFeatureFlags features){
    for (auto format : candidate)
    {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(m_MainDevice->physicalDevice, format, &props);
      if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
      {
        return format;
      }
      else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
      {
        return format;
      }
    }
    throw std::runtime_error("failed to find supported format!");
}

QueueFamilyIndices Utility::findQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies)
    {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphicsFamily = i;
      }

      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, *m_Surface, &presentSupport);

      if (presentSupport)
      {
        indices.presentFamily = i;
      }

      if (indices.isComplete())
      {
        break;
      }

      i++;
    }
    return indices;
}

bool Utility::checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
      requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void Utility::createImage(uint32_t width, uint32_t height, uint32_t miplevels, 
VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, Texture& image){
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.mipLevels = miplevels;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocationCreateInfo{};
    allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (vmaCreateImage(*m_allocator, &imageInfo, &allocationCreateInfo, &image.textureImage, &image.allocation, nullptr)!=VK_SUCCESS){
      throw std::runtime_error("failed to create image");
    }
}

VkImageView Utility::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t miplevels){
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = miplevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    

    VkImageView imageView;
    if (vkCreateImageView(m_MainDevice->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

VkSampler Utility::CreateSampler(const VkSamplerCreateInfo& sampler_create_info){
    VkSampler sampler;
	VkResult result = vkCreateSampler(m_MainDevice->logicalDevice, &sampler_create_info, nullptr, &sampler);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create a Texture Sampler!");

	return sampler;
}

void Utility::CreateTextureSampler(Texture& texture){
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_MainDevice->physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(texture.miplevels);
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_MainDevice->logicalDevice, &samplerInfo, nullptr, &texture.sampler) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create texture sampler!");
    }
}

void Utility::CreateDepthBufferImage(Texture& image, const VkExtent2D &image_extent){
    const std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
	const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	const VkFormatFeatureFlags format_flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	VkFormat Format = Utility::findSupportedFormat(formats, tiling, format_flags);
	Utility::createImage(image_extent.width, image_extent.height, image.miplevels, Format, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image);
	image.textureImageView = Utility::createImageView(image.textureImage, Format, VK_IMAGE_ASPECT_DEPTH_BIT, image.miplevels);
}

void Utility::CreatePositionBufferImage(Texture& image, const VkExtent2D& image_extent){
    const std::vector<VkFormat> formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
	const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	const VkFormatFeatureFlags format_flags = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image.Format = Utility::findSupportedFormat(formats, tiling, format_flags);
	Utility::createImage(image_extent.width, image_extent.height, image.miplevels, image.Format, tiling,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image);
	image.textureImageView = Utility::createImageView(image.textureImage, image.Format, VK_IMAGE_ASPECT_COLOR_BIT, image.miplevels);
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType						= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter					= VK_FILTER_NEAREST;		// linear interpolation between the texels
		samplerCreateInfo.minFilter					= VK_FILTER_NEAREST;		// quando viene miniaturizzata come renderizzarla (lerp)
		samplerCreateInfo.addressModeU				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.borderColor				= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerCreateInfo.unnormalizedCoordinates	= VK_FALSE;			// è normalizzata
		samplerCreateInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCreateInfo.mipLodBias				= 0.0f;
		samplerCreateInfo.minLod					= 0.0f;
		samplerCreateInfo.maxLod					= 1.0f;
		samplerCreateInfo.anisotropyEnable			= VK_TRUE;
		samplerCreateInfo.maxAnisotropy				= 16;
		samplerCreateInfo.compareEnable				= VK_FALSE;
		samplerCreateInfo.compareOp					= VK_COMPARE_OP_NEVER;
		image.sampler = Utility::CreateSampler(samplerCreateInfo);
	}
}

void Utility::CreateColorBufferImage(Texture& image, const VkExtent2D& image_extent){
    const std::vector<VkFormat> formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
	const VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	const VkFormatFeatureFlags format_flags = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	image.Format = Utility::findSupportedFormat(formats, tiling, format_flags);
	Utility::createImage(image_extent.width, image_extent.height, image.miplevels, image.Format, tiling,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image);

	image.textureImageView = Utility::createImageView(image.textureImage, image.Format, VK_IMAGE_ASPECT_COLOR_BIT, image.miplevels);

	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType						= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter					= VK_FILTER_NEAREST;		// linear interpolation between the texels
		samplerCreateInfo.minFilter					= VK_FILTER_NEAREST;		// quando viene miniaturizzata come renderizzarla (lerp)
		samplerCreateInfo.addressModeU				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW				= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.borderColor				= VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerCreateInfo.unnormalizedCoordinates	= VK_FALSE;			// è normalizzata
		samplerCreateInfo.mipmapMode				= VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCreateInfo.mipLodBias				= 0.0f;
		samplerCreateInfo.minLod					= 0.0f;
		samplerCreateInfo.maxLod					= 1.0f;
		samplerCreateInfo.anisotropyEnable			= VK_TRUE;
		samplerCreateInfo.maxAnisotropy				= 16;
		samplerCreateInfo.compareEnable				= VK_FALSE;
		samplerCreateInfo.compareOp					= VK_COMPARE_OP_NEVER;

		image.sampler = Utility::CreateSampler(samplerCreateInfo);
	}
}

void Utility::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, Buffer& buffer){
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    buffer.allocation = VK_NULL_HANDLE;
    vmaCreateBuffer(*m_allocator, &bufferInfo, &allocInfo, &buffer.buffer,&buffer.allocation, nullptr);
}

void Utility::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
    VkCommandBuffer transfer_command_buffer = beginSingleTimeCommands();

	VkBufferCopy buffer_copy_region = {};
	buffer_copy_region.size		 = size;
	vkCmdCopyBuffer(transfer_command_buffer, srcBuffer, dstBuffer, 1, &buffer_copy_region);

    endSingleTimeCommands(transfer_command_buffer);
}

void Utility::CopyBufferToImage(const VkBuffer& src, const VkImage& image, const uint32_t width, const uint32_t height){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1};
    vkCmdCopyBufferToImage(commandBuffer, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Utility::beginSingleTimeCommands(){
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = *m_CommandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_MainDevice->logicalDevice, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void Utility::endSingleTimeCommands(VkCommandBuffer commandBuffer){
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(*m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(*m_GraphicsQueue);

    vkFreeCommandBuffers(m_MainDevice->logicalDevice, *m_CommandPool, 1, &commandBuffer);
}

VkShaderModule Utility::createShaderModule(const std::vector<char>& code){
    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.codeSize = code.size();
    shader_module_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule module;
    if(vkCreateShaderModule(m_MainDevice->logicalDevice,&shader_module_info, nullptr, &module)!=VK_SUCCESS){
        throw std::runtime_error("failed to create shader module!");
    }
    return module;
}



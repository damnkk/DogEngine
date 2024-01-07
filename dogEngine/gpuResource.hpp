#ifndef GPURESOURCE_H
#define GPURESOURCE_H
#include "dgpch.h"
#include "dgplatform.h"
#define GLFW_INCLUDE_VULKAN
#include "dgLog.hpp"
#include "glm/glm.hpp"
#include <GLFW/glfw3.h>
//#include "vma/vk_mem_alloc.h"
#include "dgAssert.h"
#include "vk_mem_alloc.h"
//#include "vk_mem_alloc.h"

namespace dg {
    struct DeviceContext;
    static const u32 k_max_swapchain_images = 3;
    static const u32 k_invalid_index = 0xffffffff;
    static const u32 k_max_mipmap_level = 100000000;
    static const u32 k_max_discriptor_nums_per_set = 32;
    static const u32 k_max_bindless_resource = 1024;
    static const u32 k_bindless_sampled_texture_bind_index = 0;
    static const u8 k_max_image_outputs = 8;
    static const u8 k_max_vertex_streams = 16;
    static const u8 k_max_vertex_attributes = 16;
    static const u8 k_max_shader_stages = 5;
    static const u8 k_max_descriptor_set_layouts = 8;

    using ResourceHandle = u32;

    struct BufferHandle {
        ResourceHandle index;
    };

    struct TextureHandle {
        ResourceHandle index;
    };

    struct ShaderStateHandle {
        ResourceHandle index;
    };

    struct SamplerHandle {
        ResourceHandle index;
    };

    struct DescriptorSetLayoutHandle {
        ResourceHandle index;
    };

    struct DescriptorSetHandle {
        ResourceHandle index;
    };

    struct PipelineHandle {
        ResourceHandle index;
    };

    struct RenderPassHandle {
        ResourceHandle index;
    };

    struct FrameBufferHandle {
        ResourceHandle index;
    };

    struct ImageBarrier {
        TextureHandle texture;
    };

    struct MemBarrier {
        BufferHandle buffer;
    };

    struct Resource {
        void addReference() { ++references; }
        void removeReference() {
            DGASSERT(references == 0);
            --references;
        }
        u64 references = 0;
        std::string name;
    };

    typedef enum ResourceState {
        RESOURCE_STATE_UNDEFINED = 0,
        RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER = 0x1,
        RESOURCE_STATE_INDEX_BUFFER = 0x2,
        RESOURCE_STATE_RENDER_TARGET = 0x4,
        RESOURCE_STATE_UNORDERED_ACCESS = 0x8,
        RESOURCE_STATE_DEPTH_WRITE = 0x10,
        RESOURCE_STATE_DEPTH_READ = 0x20,
        RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE = 0x40,
        RESOURCE_STATE_PIXEL_SHADER_RESOURCE = 0x80,
        RESOURCE_STATE_SHADER_RESOURCE = 0x40 | 0x80,
        RESOURCE_STATE_STREAM_OUT = 0x100,
        RESOURCE_STATE_INDIRECT_ARGUMENT = 0x200,
        RESOURCE_STATE_COPY_DEST = 0x400,
        RESOURCE_STATE_COPY_SOURCE = 0x800,
        RESOURCE_STATE_GENERIC_READ = (((((0x1 | 0x2) | 0x40) | 0x80) | 0x200) | 0x800),
        RESOURCE_STATE_PRESENT = 0x1000,
        RESOURCE_STATE_COMMON = 0x2000,
        RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
        RESOURCE_STATE_SHADING_RATE_SOURCE = 0x8000,
    };

    struct Texture {
        VkImage m_image;
        VkDescriptorImageInfo m_imageInfo;
        VkFormat m_format;
        VmaAllocation m_vma;
        TextureHandle m_handle;
        bool bindless = false;
        TextureType::Enum m_type;
        std::string m_name;
        VkExtent3D m_extent = {1, 1, 1};
        u16 m_mipLevel;
        VkImageUsageFlags m_usage;
        ResourceState m_resourceState = RESOURCE_STATE_UNDEFINED;
        u8 m_flags = 0;

        void setSampler(std::shared_ptr<DeviceContext> context, SamplerHandle handle);
    };

    struct Buffer {
        VkBuffer m_buffer;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VkDeviceMemory m_deviceMemory;
        VkDeviceSize m_bufferSize;
        VkBufferUsageFlags m_bufferUsage;
        u32 m_size = 0;
        u32 m_globalOffset = 0;
        std::string name;
        BufferHandle m_handle;
        BufferHandle m_parentHandle;
        void *m_mappedMemory;
        void destroy(VkDevice &devcie, VmaAllocator &allocator);
    };

    struct Sampler {
        VkSampler m_sampler;
        VkFilter m_minFilter = VK_FILTER_LINEAR;
        VkFilter m_maxFilter = VK_FILTER_LINEAR;
        VkSamplerMipmapMode m_mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VkSamplerAddressMode m_addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        float m_minLod = 0.0f;
        float m_maxLod = 0.0f;
        float m_LodBias = 0.0f;
        std::string name;
    };

    struct DescriptorBinding {

        VkDescriptorType type;
        u16 start = 0;
        u16 count = 0;
        u16 set = 0;
        VkShaderStageFlagBits stageFlags;

        std::string name;
    };// struct ResourceBindingVulkan

    struct DescriptorSetLayout {
        VkDescriptorSetLayout m_descriptorSetLayout;

        std::vector<VkDescriptorSetLayoutBinding> m_vkBindings;
        std::vector<DescriptorBinding> m_bindings;
        u32 m_numBindings = 0;
        u32 m_setIndex = 0;
        bool bindless = false;
        DescriptorSetLayoutHandle m_handle = {k_invalid_index};
    };

    struct DescriptorSet {
        VkDescriptorSet m_vkdescriptorSet;
        std::vector<ResourceHandle> m_resources;
        std::vector<SamplerHandle> m_samples;
        std::vector<u32> m_bindings;

        const DescriptorSetLayout *m_layout;
        u32 m_resourceNums;
    };

    struct RenderPass {
        VkRenderPass m_renderPass;
        VkFramebuffer m_frameBuffer;

        std::vector<TextureHandle> m_outputTextures;
        TextureHandle m_depthTexture;
        bool operator==(RenderPass &pass);
        bool operator!=(RenderPass &pass);

        RenderPassType::Enum m_type;
        float m_scalex = 1.0f;
        float m_scaley = 1.0f;
        u32 m_width;
        u32 m_height;
        u32 m_dispatchx = 0;
        u32 m_dispatchy = 0;
        u32 m_dispatchz = 0;

        u32 m_resize = 0;
        u32 m_numRenderTarget = 0;
        std::string name;
    };

    struct FrameBuffer {
        VkFramebuffer m_frameBuffer;
        RenderPassHandle m_renderPassHandle;
        u32 m_width;
        u32 m_height;
        float m_scalex = 1.0f;
        float m_scaley = 1.0f;
        TextureHandle m_colorAttachments[k_max_image_outputs];
        u32 m_numRenderTargets = 0;
        TextureHandle m_depthStencilAttachment = {k_invalid_index};
        bool m_resize = 0;
        std::string name;
    };

    struct ShaderState {
        VkPipelineShaderStageCreateInfo m_shaderStateInfo[k_max_shader_stages];
        std::string m_name;
        u32 m_activeShaders = 0;
        bool m_isInGraphicsPipelie = true;
    };

    struct TextureCreateInfo {
        void *m_sourceData = nullptr;
        VkExtent3D m_textureExtent;
        u32 m_mipmapLevel = 1;
        VkFormat m_imageFormat;
        TextureType::Enum m_imageType = TextureType::Enum::Texture2D;
        std::string name;
        u8 m_flags = 0;
        bool bindless = false;

        TextureCreateInfo &setName(const char *neme);
        TextureCreateInfo &setFormat(VkFormat format);
        TextureCreateInfo &setExtent(VkExtent3D extent);
        TextureCreateInfo &setData(void *data);
        TextureCreateInfo &setMipmapLevel(u32 miplevel);
        TextureCreateInfo &setFlag(u8 flag);
        TextureCreateInfo &setTextureType(TextureType::Enum type);
        TextureCreateInfo &setBindLess(bool isBindLess);
    };

    struct BufferCreateInfo {
        VkDeviceSize m_bufferSize;
        VkBufferUsageFlags m_bufferUsage;
        void *m_sourceData = nullptr;
        std::string name;
        bool m_deviceOnly = true;
        bool m_presistent = false;

        BufferCreateInfo &reset();
        BufferCreateInfo &setUsageSize(VkBufferUsageFlags usage, VkDeviceSize size);
        BufferCreateInfo &setData(void *data);
        BufferCreateInfo &setName(const char *name);
        BufferCreateInfo &setDeviceOnly(bool deviceOnly);
    };

    struct SamplerCreateInfo {
        VkFilter m_minFilter = VK_FILTER_LINEAR;
        VkFilter m_maxFilter = VK_FILTER_LINEAR;
        VkSamplerMipmapMode m_mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VkSamplerAddressMode m_addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        float m_maxLod = 0.0f;
        float m_minLod = 0.0f;
        float m_LodBias = 0.0f;

        std::string name;

        SamplerCreateInfo &set_min_mag_mip(VkFilter min, VkFilter max, VkSamplerMipmapMode mipMode);
        SamplerCreateInfo &set_address_u(VkSamplerAddressMode u);
        SamplerCreateInfo &set_address_uv(VkSamplerAddressMode u, VkSamplerAddressMode v);
        SamplerCreateInfo &set_address_uvw(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
        SamplerCreateInfo &set_name(const char *name);
        SamplerCreateInfo &set_LodMinMaxBias(float min, float max, float bias);
    };

    struct RasterizationCreation {
        VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace m_front = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        FillMode::Enum m_fillMode = FillMode::Enum::Solid;
    };
    struct StencilOperationState {
        VkStencilOp m_fail = VK_STENCIL_OP_KEEP;
        VkStencilOp m_pass = VK_STENCIL_OP_KEEP;
        VkStencilOp m_depthFail = VK_STENCIL_OP_KEEP;
        VkCompareOp m_compare = VK_COMPARE_OP_ALWAYS;
        u32 m_compareMask = 0xff;
        u32 m_writeMask = 0xff;
        u32 m_reference = 0xff;
    };

    struct DepthStencilCreation {
        StencilOperationState m_front;
        StencilOperationState m_back;
        VkCompareOp m_depthCompare = VK_COMPARE_OP_ALWAYS;

        bool m_depthEnable = true;
        bool m_depthWriteEnable = true;
        bool m_stencilEnable = true;
        u32 m_pad = 5;
        DepthStencilCreation() : m_depthEnable(true), m_depthWriteEnable(true), m_stencilEnable(false) {
        }
        DepthStencilCreation &setDepth(bool enable, bool write, VkCompareOp compareTest);
    };

    struct BlendState {
        VkBlendFactor m_sourceColor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor m_destinationColor = VK_BLEND_FACTOR_ONE;
        VkBlendOp m_colorBlendOp = VK_BLEND_OP_ADD;

        VkBlendFactor m_sourceAlpha = VK_BLEND_FACTOR_ONE;
        VkBlendFactor m_destinationAlpha = VK_BLEND_FACTOR_ONE;
        VkBlendOp m_alphaOperation = VK_BLEND_OP_ADD;

        ColorWriteEnabled::Mask m_colorWriteMask = ColorWriteEnabled::All_mask;

        bool m_blendEnabled = false;
        bool m_separateBlend = true;
        u32 m_pad = 6;

        BlendState() : m_blendEnabled(false), m_separateBlend(false) {
        }
        BlendState &setColor(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp blendop);
        BlendState &setAlpha(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp blendop);
        BlendState &setColorWriteMask(ColorWriteEnabled::Mask value);
    };

    struct BlendStateCreation {
        BlendState m_blendStates[k_max_image_outputs];
        u32 m_activeStates = 0;
        BlendStateCreation &reset();
        BlendState &add_blend_state();
    };

    struct VertexStream {
        u16 m_binding = 0;
        u16 m_stride = 0;
        VertexInputRate::Enum m_inputRate = VertexInputRate::Enum::Count;
    };

    struct VertexAttribute {
        u16 m_location = 0;
        u16 m_binding = 0;
        u16 m_offset = 0;
        VertexComponentFormat ::Enum m_format = VertexComponentFormat::Enum::Count;
    };
    struct VertexInputCreation {
        std::vector<VkVertexInputAttributeDescription> Attrib;
        VkVertexInputBindingDescription Binding;
    };

    struct ShaderStage {
        const char *m_code;
        u32 m_codeSize;
        VkShaderStageFlagBits m_type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    };

    struct ShaderStateCreation {
        ShaderStage m_stages[k_max_shader_stages];
        std::string name;
        u32 m_stageCount;
        u32 m_spvInput;

        ShaderStateCreation &reset();
        ShaderStateCreation &setName(const char *name);
        ShaderStateCreation &addStage(const char *code, u32 codeSize, VkShaderStageFlagBits type);
        ShaderStateCreation &setSpvInput(bool value);
    };

    struct Rect2DInt {
        int16_t x = 0;
        int16_t y = 0;
        u16 width = 0;
        u16 height = 0;
    };

    struct Rect2DFloat {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    struct ViewPort {
        Rect2DFloat rect;
        float min_depth = 0.0f;
        float max_depth = 0.0f;
    };

    struct ViewPortState {
        u32 m_numViewports = 0;
        u32 m_numScissors = 0;
        ViewPort *m_viewport = nullptr;
        Rect2DInt *m_scissors = nullptr;
    };

    struct RenderPassOutput {
        VkFormat m_colorFormats[k_max_image_outputs];
        VkFormat m_depthStencilFormat;
        u32 m_numColorFormats;

        RenderPassOperation::Enum m_colorOperation = RenderPassOperation::Enum::DontCare;
        RenderPassOperation::Enum m_depthOperation = RenderPassOperation::Enum::DontCare;
        RenderPassOperation::Enum m_stencilOperation = RenderPassOperation::Enum::DontCare;

        RenderPassOutput &reset();
        RenderPassOutput &color(VkFormat format);
        RenderPassOutput &depth(VkFormat format);
        RenderPassOutput &setOperations(RenderPassOperation::Enum color,
                                        RenderPassOperation::Enum depth,
                                        RenderPassOperation::Enum stencil);
    };


    struct DescriptorSetCreateInfo {
        std::vector<ResourceHandle> m_resources;
        std::vector<SamplerHandle> m_samplers;
        std::vector<u32> m_bindings;

        DescriptorSetLayoutHandle m_layout;
        u32 m_resourceNums = 0;
        std::string name;

        DescriptorSetCreateInfo &reset();
        DescriptorSetCreateInfo &setName(const char *name);
        DescriptorSetCreateInfo &texture(TextureHandle handle, u32 binding);
        DescriptorSetCreateInfo &buffer(BufferHandle handle, u32 binding);
        DescriptorSetCreateInfo &textureSampler(TextureHandle tex, SamplerHandle sampler, u32 binding);
        DescriptorSetCreateInfo &setLayout(DescriptorSetLayoutHandle handle);
    };
    struct DescriptorSetLayoutCreateInfo {
        struct Binding {
            VkDescriptorType m_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            int m_start = -1;
            u32 m_count;
            std::string name;
            VkShaderStageFlagBits stageFlags = VK_SHADER_STAGE_ALL;
        };
        Binding m_bindings[k_max_discriptor_nums_per_set];
        u32 m_bindingNum = 0;
        u32 m_setIndex = 0;
        bool bindless = false;
        std::string name;

        DescriptorSetLayoutCreateInfo &reset();
        DescriptorSetLayoutCreateInfo &setName(const char *name);
        DescriptorSetLayoutCreateInfo &addBinding(const Binding &binding);
        DescriptorSetLayoutCreateInfo &setBindingAtIndex(const Binding &binding, u32 index);
        DescriptorSetLayoutCreateInfo &setSetIndex(u32 index);
    };

    struct PipelineCreateInfo {

        RasterizationCreation m_rasterization;
        DepthStencilCreation m_depthStencil;
        BlendStateCreation m_BlendState;
        VertexInputCreation m_vertexInput;
        ShaderStateCreation m_shaderState;

        RenderPassOutput m_renderPassOutput;
        RenderPassHandle m_renderPassHandle;
        DescriptorSetLayoutHandle m_descLayout[k_max_descriptor_set_layouts];
        std::vector<VkPushConstantRange> m_pushConstants;
        const ViewPortState *m_viewport = nullptr;

        u32 m_numActivateLayouts = 0;
        std::string name;
        PipelineCreateInfo &addDescriptorSetlayout(const DescriptorSetLayoutHandle &handle);
        PipelineCreateInfo &addPushConstants(VkPushConstantRange push);
        RenderPassOutput &renderPassOutput();
    };

    struct RenderPassCreateInfo {
        u32 m_rtNums;
        RenderPassType::Enum m_renderPassType = RenderPassType::Geometry;

        std::vector<TextureHandle> m_outputTextures;
        TextureHandle m_depthTexture = {k_invalid_index};
        VkImageLayout m_finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        float m_scalex = 1.0f;
        float m_scaley = 1.0f;
        u32 resize = 1;

        RenderPassOperation::Enum m_color = RenderPassOperation::Enum::DontCare;
        RenderPassOperation::Enum m_depth = RenderPassOperation::Enum::DontCare;
        RenderPassOperation::Enum m_stencil = RenderPassOperation::Enum::DontCare;

        std::string name;
        RenderPassCreateInfo &setName(const char *name);
        RenderPassCreateInfo &setFinalLayout(VkImageLayout finalLayout);
        RenderPassCreateInfo &addRenderTexture(TextureHandle handle);
        RenderPassCreateInfo &setScale(float x, float y, u32 resize);
        RenderPassCreateInfo &setDepthStencilTexture(TextureHandle handle);
        RenderPassCreateInfo &setType(RenderPassType::Enum type);
        RenderPassCreateInfo &setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);
    };

    struct FrameBufferCreateInfo {
        RenderPassHandle m_renderPassHandle;
        u32 m_numRenderTargets = 0;
        TextureHandle m_outputTextures[k_max_image_outputs];
        TextureHandle m_depthStencilTexture;

        u32 m_width;
        u32 m_height;
        float m_scaleX = 1.0f;
        float m_scaleY = 1.0f;
        u8 resize = 1;
        std::string name;

        FrameBufferCreateInfo &reset();
        FrameBufferCreateInfo &addRenderTarget(TextureHandle texture);
        FrameBufferCreateInfo &setDepthStencilTexture(TextureHandle depthTexture);
        FrameBufferCreateInfo &setName(const char *name);
        FrameBufferCreateInfo &setScaling(float scalex, float scaley, u8 resize);
        FrameBufferCreateInfo &setRenderPass(RenderPassHandle rp);
        FrameBufferCreateInfo &setExtent(VkExtent2D extent);
    };

    struct Pipeline {
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipelineLayout;
        VkPipelineBindPoint m_bindPoint;
        ShaderStateHandle m_shaderState;
        DescriptorSetLayout *m_DescriptorSetLayout[k_max_descriptor_set_layouts];
        DescriptorSetLayoutHandle m_DescriptorSetLayoutHandle[k_max_descriptor_set_layouts];
        u32 m_activeLayouts = 0;

        RasterizationCreation m_rasterizationCrt;
        DepthStencilCreation m_depthStencilCrt;
        BlendStateCreation m_blendStateCrt;
        VertexInputCreation m_vertexInputCrt;
        ShaderStateCreation m_shaderStateCrt;
        PipelineHandle m_pipelineHandle;
        bool m_isGraphicsPipeline;
    };

    struct ExecutionBarrier {
        static const u32 maxBarrierNum = 8;
        PipelineStage::Enum srcStage;
        PipelineStage::Enum dstStage;
        VkImageLayout newLayout;
        u32 oldQueueIndex = VK_QUEUE_FAMILY_IGNORED;
        u32 newQueueIndex = VK_QUEUE_FAMILY_IGNORED;
        VkAccessFlagBits srcImageAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
        VkAccessFlagBits dstImageAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
        VkAccessFlagBits srcBufferAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
        VkAccessFlagBits dstBufferAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;


        u32 numImageBarriers;
        u32 numBufferBarriers;
        u32 newBarrierExperimental = UINT32_MAX;

        ImageBarrier imageBarriers[maxBarrierNum];
        MemBarrier memoryBarriers[maxBarrierNum];

        ExecutionBarrier &reset();
        ExecutionBarrier &set(PipelineStage::Enum srcStage, PipelineStage::Enum dstStage);
        ExecutionBarrier &addImageBarrier(const ImageBarrier &imageBarrier);
        ExecutionBarrier &addmemoryBarrier(const MemBarrier &bufferBarrier);
    };

    struct ResourceUpdate {
        ResourceUpdateType::Enum type;
        ResourceHandle handle;
        u32 currentFrame;
        bool deleting = true;
    };

    struct DescriptorSetUpdate {
        DescriptorSetHandle handle;
        u32 currFrame = 0;
    };

    struct ConstentData {
        int tempData;
    };


    static VkImageType to_vk_image_type(TextureType::Enum e) {
        static VkImageType imageTypes[TextureType::UNDEFINED] = {VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D};
        return imageTypes[e];
    }

    static VkImageViewType to_vk_image_view_type(TextureType::Enum e) {
        static VkImageViewType imageViewTypes[TextureType::UNDEFINED] = {VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY};
        return imageViewTypes[e];
    }

    static VkFormat to_vk_vertex_format(VertexComponentFormat::Enum value) {
        // Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2, Uint4, Count
        static VkFormat s_vk_vertex_formats[VertexComponentFormat::Count] = {VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, /*MAT4 TODO*/ VK_FORMAT_R32G32B32A32_SFLOAT,
                                                                             VK_FORMAT_R8_SINT, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SNORM,
                                                                             VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT};

        return s_vk_vertex_formats[value];
    }


    static VkAccessFlags util_to_vk_access_flags(ResourceState state) {
        VkAccessFlags ret = 0;
        if (state & RESOURCE_STATE_COPY_SOURCE) {
            ret |= VK_ACCESS_TRANSFER_READ_BIT;
        }
        if (state & RESOURCE_STATE_COPY_DEST) {
            ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        if (state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {
            ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        if (state & RESOURCE_STATE_INDEX_BUFFER) {
            ret |= VK_ACCESS_INDEX_READ_BIT;
        }
        if (state & RESOURCE_STATE_UNORDERED_ACCESS) {
            ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        if (state & RESOURCE_STATE_INDIRECT_ARGUMENT) {
            ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        if (state & RESOURCE_STATE_RENDER_TARGET) {
            ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        if (state & RESOURCE_STATE_DEPTH_WRITE) {
            ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        }
        if (state & RESOURCE_STATE_SHADER_RESOURCE) {
            ret |= VK_ACCESS_SHADER_READ_BIT;
        }
        if (state & RESOURCE_STATE_PRESENT) {
            ret |= VK_ACCESS_MEMORY_READ_BIT;
        }
        if (state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE) {
            ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
        }
        return ret;
    }

    static VkPipelineStageFlags to_vk_pipeline_stage(PipelineStage::Enum value) {
        static VkPipelineStageFlags s_vk_values[] = {VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
        return s_vk_values[value];
    }


    static VkImageLayout util_to_vk_image_layout(ResourceState usage) {
        if (usage & RESOURCE_STATE_COPY_SOURCE)
            return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        if (usage & RESOURCE_STATE_COPY_DEST)
            return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        if (usage & RESOURCE_STATE_RENDER_TARGET)
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        if (usage & RESOURCE_STATE_DEPTH_WRITE)
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        if (usage & RESOURCE_STATE_DEPTH_READ)
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        if (usage & RESOURCE_STATE_UNORDERED_ACCESS)
            return VK_IMAGE_LAYOUT_GENERAL;

        if (usage & RESOURCE_STATE_SHADER_RESOURCE)
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if (usage & RESOURCE_STATE_PRESENT)
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        if (usage == RESOURCE_STATE_COMMON)
            return VK_IMAGE_LAYOUT_GENERAL;

        return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    static VkPipelineStageFlags util_determine_pipeline_stage_flags(VkAccessFlags accessFlags, QueueType::Enum queueType) {
        VkPipelineStageFlags flags = 0;

        switch (queueType) {
            case QueueType::Graphics: {
                if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
                    flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0) {
                    flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                    flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    /*if ( pRenderer->pActiveGpuSettings->mGeometryShaderSupported ) {
                        flags |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                    }
                    if ( pRenderer->pActiveGpuSettings->mTessellationSupported ) {
                        flags |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                        flags |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                    }*/
                    flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
#ifdef ENABLE_RAYTRACING
                    if (pRenderer->mVulkan.mRaytracingExtension) {
                        flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV;
                    }
#endif
                }

                if ((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
                    flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
                    flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                if ((accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                    flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

                break;
            }
            case QueueType::Compute: {
                if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0 ||
                    (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0 ||
                    (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0 ||
                    (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) != 0)
                    return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

                if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
                    flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                break;
            }
            case QueueType::CopyTransfer:
                return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            default:
                break;
        }

        // Compatible with both compute and graphics queues
        if ((accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT) != 0)
            flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

        if ((accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

        if ((accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_HOST_BIT;

        if (flags == 0)
            flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        return flags;
    }

}//namespace dg

#endif//GPURESOURCE_H
#ifndef GPURESOURCE_H
#define GPURESOURCE_H
#include "dgplatform.h"
#include "dgpch.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "dgLog.hpp"
#include "glm/glm.hpp"
//#include "vma/vk_mem_alloc.h"
#include "dgAssert.h"
#include "vk_mem_alloc.h"
//#include "vk_mem_alloc.h"

namespace dg{

static const u32            k_max_swapchain_images = 3;
static const u32            k_invalid_index = 0xffffffff;
static const u32            k_max_discriptor_nums_per_set = 16;
static const u8                     k_max_image_outputs = 8; 
static const u8                     k_max_vertex_streams = 16;
static const u8                     k_max_vertex_attributes = 16;
static const u8                     k_max_shader_stages = 5;  
static const u8                     k_max_descriptor_set_layouts = 8; 

using ResourceHandle = u32;

struct BufferHandle{
   ResourceHandle              index;
};

struct TextureHandle{
    ResourceHandle             index;
};

struct ShaderStateHandle{
    ResourceHandle             index;
};

struct SamplerHandle{
    ResourceHandle             index;
};

struct DescriptorSetLayoutHandle{
    ResourceHandle              index;
};

struct DescriptorSetHandle{
    ResourceHandle              index;
};

struct PipelineHandle{
    ResourceHandle              index;
};

struct RenderPassHandle{
    ResourceHandle              index;
};

struct FrameBufferHandle{
    ResourceHandle              index;
};

struct ImageBarrier{
    TextureHandle               texture;
};

struct MemBarrier{
    BufferHandle                buffer;
};

struct Resource{
    void                        addReference(){++references;}
    void                        removeReference(){DGASSERT(references==0); --references;}
    u64                         references = 0;
    const  char*                name = nullptr;
};

struct TextureResource:public Resource{
    TextureHandle                       handle;
    u32                                 poolIdx;
    static constexpr const char*        k_type = "texture_resource";
};

struct BufferResource:public Resource{
    BufferHandle                        handle;
    u32                                 poolIdx;
    static constexpr const char*        k_type = "buffer_resource";
};

struct SamplerResource:public Resource{
    SamplerHandle                       handle;
    u32                                 poolIdx;
    static constexpr const char*        k_type = "sampler_resource";
};

struct DescriptorSetLayoutResource:public Resource{
    DescriptorSetLayoutHandle           handle;
    u32                                 poolIdx;
    static constexpr const char*        k_type = "descriptorSet_layout_resource";
};

struct DescriptorSetResource:public Resource{
    DescriptorSetHandle                 handle;
    u32                                 poolIdx;
    static constexpr const char*        k_type = "descriptor_set_resource";
};

struct Texture{
    VkImage                             m_image;
    VkDescriptorImageInfo               m_imageInfo;
    VkFormat                            m_format;
    VmaAllocation                       m_vma;
    TextureHandle                       m_handle;
    
    TextureType::Enum                   m_type;
    const char*                         m_name;
    VkExtent3D                          m_extent= {1,1,1};
    u16                                 m_mipLevel;
    VkImageUsageFlags                   m_usage;

};

struct Buffer{
    VkBuffer                            m_buffer;
    VmaAllocation                       m_allocation = VK_NULL_HANDLE;
    VkDeviceMemory                      m_deviceMemory;
    VkDeviceSize                        m_bufferSize;
    VkBufferUsageFlags                  m_bufferUsage;
    u32                                 m_size = 0;
    u32                                 m_globalOffset = 0;
    const char *                        name = nullptr;
    BufferHandle                        m_handle;
    BufferHandle                        m_parentHandle;
    void*                               m_mappedMemory;
    void destroy(VkDevice& devcie,VmaAllocator& allocator);
};

struct Sampler{
    VkSampler                           m_sampler;
    VkFilter                            m_minFilter = VK_FILTER_LINEAR;
    VkFilter                            m_maxFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode                 m_mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode                m_addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode                m_addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode                m_addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    const char*                         name = nullptr;
};

struct DescriptorBinding {

    VkDescriptorType                    type;
    u16                                 start   = 0;
    u16                                 count   = 0;
    u16                                 set     = 0;

    const char*                         name    = nullptr;
}; // struct ResourceBindingVulkan

struct DescriptorSetLayout{
    VkDescriptorSetLayout               m_descriptorSetLayout;

    std::vector<VkDescriptorSetLayoutBinding>       m_vkBindings;
    DescriptorBinding*                  m_bindings = nullptr;
    u32                                 m_numBindings = 0;
    u32                                 m_setIndex = 0;

    DescriptorSetLayoutHandle           m_handle;
};

struct DescriptorSet{
    VkDescriptorSet                     m_vkdescriptorSet;
    std::vector<ResourceHandle>         m_resources;
    std::vector<SamplerHandle>          m_samples;
    std::vector<u32>                    m_bindings;

    const DescriptorSetLayout*          m_layout;
    u32                                 m_resourceNums;

};

struct RenderPass{
    VkRenderPass                        m_renderPass;
    VkFramebuffer                       m_frameBuffer;

    std::vector<TextureHandle>          m_outputTextures;
    TextureHandle                       m_depthTexture;
    bool                                operator==(RenderPass& pass);
    bool                                operator!=(RenderPass& pass);

    RenderPassType::Enum                m_type;
    float                               m_scalex = 1.0f;
    float                               m_scaley = 1.0f;
    u32                                 m_width;
    u32                                 m_height;
    u32                                 m_dispatchx = 0;
    u32                                 m_dispatchy = 0;
    u32                                 m_dispatchz = 0;

    u32                                 m_resize    = 0;
    u32                                 m_numRenderTarget = 0;
    const char*                         name;
};

struct FrameBuffer{
    VkFramebuffer                       m_frameBuffer;
    RenderPassHandle                    m_renderPassHandle;
    u32                                 m_width;
    u32                                 m_height;
    float                               m_scalex = 1.0f;
    float                               m_scaley = 1.0f;
    TextureHandle                       m_colorAttachments[k_max_image_outputs];
    u32                                 m_numRenderTargets = 0;
    TextureHandle                       m_depthStencilAttachment = {k_invalid_index};
    bool                                m_resize = 0;
    const char*                         name;
};

struct ShaderState{
    VkPipelineShaderStageCreateInfo     m_shaderStateInfo[k_max_shader_stages];
    const char*                         m_name = nullptr;
    u32                                 m_activeShaders = 0;
    bool                                m_isInGraphicsPipelie = true;
};

struct TextureCreateInfo{
    void*                               m_sourceData = nullptr;
    VkExtent3D                          m_textureExtent;
    u32                                 m_mipmapLevel;
    VkFormat                            m_imageFormat;
    TextureType::Enum                   m_imageType = TextureType::Enum::Texture2D;
    const char*                         name;
    VkImageUsageFlags                   m_imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    TextureCreateInfo&                  setName(const char* neme);
    TextureCreateInfo&                  setFormat(VkFormat format);
    TextureCreateInfo&                  setExtent(VkExtent3D extent);
    TextureCreateInfo&                  setData(void* data);
    TextureCreateInfo&                  setMipmapLevel(u32 miplevel);
    TextureCreateInfo&                  setUsage(VkImageUsageFlags usage);
};

struct BufferCreateInfo{
    VkDeviceSize                        m_bufferSize;
    VkBufferUsageFlags                  m_bufferUsage;
    void*                               m_sourceData = nullptr;
    const char*                         name= nullptr;
    bool                                m_deviceOnly= true;
    bool                                m_presistent = false;

    BufferCreateInfo&                   reset();
    BufferCreateInfo&                   setUsageSize(VkBufferUsageFlags usage,VkDeviceSize size);
    BufferCreateInfo&                   setData(void* data);
    BufferCreateInfo&                   setName(const char* name); 
    BufferCreateInfo&                   setDeviceOnly(bool deviceOnly);
};

struct SamplerCreateInfo{
    VkFilter                            m_minFilter = VK_FILTER_LINEAR;
    VkFilter                            m_maxFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode                 m_mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode                m_addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode                m_addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode                m_addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    const char*                         name = nullptr;

    SamplerCreateInfo&                  set_min_mag_mip(VkFilter min, VkFilter max, VkSamplerMipmapMode mipMode);
    SamplerCreateInfo&                  set_address_u(VkSamplerAddressMode u);
    SamplerCreateInfo&                  set_address_uv(VkSamplerAddressMode u, VkSamplerAddressMode v);
    SamplerCreateInfo&                  set_address_uvw(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
    SamplerCreateInfo&                  set_name(const char* name);
};

struct RasterizationCreation{
    VkCullModeFlags                     m_cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace                         m_front = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    FillMode::Enum                      m_fillMode = FillMode::Enum::Solid;
};
struct StencilOperationState{
    VkStencilOp                         m_fail = VK_STENCIL_OP_KEEP;
    VkStencilOp                         m_pass = VK_STENCIL_OP_KEEP;
    VkStencilOp                         m_depthFail = VK_STENCIL_OP_KEEP;
    VkCompareOp                         m_compare = VK_COMPARE_OP_ALWAYS;
    u32                                 m_compareMask = 0xff;
    u32                                 m_writeMask = 0xff;
    u32                                 m_reference = 0xff;
};

struct DepthStencilCreation{
    StencilOperationState               m_front;
    StencilOperationState               m_back;
    VkCompareOp                         m_depthCompare = VK_COMPARE_OP_ALWAYS;

    bool                                m_depthEnable = true;
    bool                                m_depthWriteEnable = true;
    bool                                m_stencilEnable = true;
    u32                                 m_pad = 5;
    DepthStencilCreation():m_depthEnable(true),m_depthWriteEnable(true),m_stencilEnable(false){

    }
    DepthStencilCreation&               setDepth(bool write, VkCompareOp compareTest);
};

struct BlendState{
    VkBlendFactor                       m_sourceColor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor                       m_destinationColor = VK_BLEND_FACTOR_ONE;
    VkBlendOp                           m_colorBlendOp = VK_BLEND_OP_ADD;

    VkBlendFactor                       m_sourceAlpha = VK_BLEND_FACTOR_ONE;
    VkBlendFactor                       m_destinationAlpha = VK_BLEND_FACTOR_ONE;
    VkBlendOp                           m_alphaOperation = VK_BLEND_OP_ADD;

    ColorWriteEnabled::Mask             m_colorWriteMask = ColorWriteEnabled::All_mask;

    bool                                m_blendEnabled = false;
    bool                                m_separateBlend = true;
    u32                                 m_pad =6;

    BlendState():m_blendEnabled(false),m_separateBlend(false){

    }
    BlendState&                         setColor(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp blendop);
    BlendState&                         setAlpha(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp blendop);
    BlendState&                         setColorWriteMask(ColorWriteEnabled::Mask value);
};

struct BlendStateCreation{
    BlendState                          m_blendStates[k_max_image_outputs];
    u32                                 m_activeStates = 0;
    BlendStateCreation&                 reset();
    BlendState&                 add_blend_state();
};

struct VertexStream{
    u16                                 m_binding = 0;
    u16                                 m_stride = 0;
    VertexInputRate::Enum               m_inputRate = VertexInputRate::Enum::Count;
};

struct VertexAttribute{
    u16                                 m_location = 0;
    u16                                 m_binding = 0;
    u16                                 m_offset = 0;
    VertexComponentFormat ::Enum        m_format = VertexComponentFormat::Enum::Count;
};
struct VertexInputCreation{
    std::vector<VkVertexInputAttributeDescription> Attrib;
    VkVertexInputBindingDescription                 Binding;
};

struct ShaderStage{
    const char*                         m_code;
    u32                                 m_codeSize;
    VkShaderStageFlagBits               m_type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
};

struct ShaderStateCreation{
    ShaderStage                         m_stages[k_max_shader_stages];
    const char*                         name;
    u32                                 m_stageCount;
    u32                                 m_spvInput;

    ShaderStateCreation&                reset();
    ShaderStateCreation&                setName(const char* name);
    ShaderStateCreation&                addStage(const char* code ,u32 codeSize, VkShaderStageFlagBits type);
    ShaderStateCreation&                setSpvInput(bool value);
};

struct Rect2DInt{
    int16_t                             x = 0;
    int16_t                             y = 0;
    u16                                 width = 0;
    u16                                 height = 0;
};

struct ViewPort{
    Rect2DInt                           rect;
    float                               min_depth = 0.0f;
    float                               max_depth = 0.0f;
};

struct ViewPortState{
    u32                                 m_numViewports = 0;
    u32                                 m_numScissors = 0;
    ViewPort*                           m_viewport = nullptr;
    Rect2DInt*                          m_scissors = nullptr;
};

struct RenderPassOutput{
    VkFormat                            m_colorFormats[k_max_image_outputs];
    VkFormat                            m_depthStencilFormat;
    u32                                 m_numColorFormats;

    RenderPassOperation::Enum           m_colorOperation = RenderPassOperation::Enum::DontCare;
    RenderPassOperation::Enum           m_depthOperation = RenderPassOperation::Enum::DontCare;
    RenderPassOperation::Enum           m_stencilOperation = RenderPassOperation::Enum::DontCare;

    RenderPassOutput&                   reset();
    RenderPassOutput&                   color(VkFormat format);
    RenderPassOutput&                   depth(VkFormat format);
    RenderPassOutput&                   setOperations(RenderPassOperation::Enum color,
                                                      RenderPassOperation::Enum depth,
                                                      RenderPassOperation::Enum stencil);
};

struct pipelineCreateInfo{
    RasterizationCreation               m_rasterization;
    DepthStencilCreation                m_depthStencil;
    BlendStateCreation                  m_BlendState;
    VertexInputCreation                 m_vertexInput;
    ShaderStateCreation                 m_shaderState;

    RenderPassOutput                    m_renderPassOutput;
    RenderPassHandle                    m_renderPassHandle;
    DescriptorSetLayoutHandle           m_DescriptroSetLayouts[k_max_discriptor_nums_per_set];
    const ViewPortState*                m_viewport = nullptr;

    u32                                 m_numActivateLayouts = 0;
    const char*                         name = 0;
    pipelineCreateInfo&                 addDescriptorSetlayout(DescriptorSetLayoutHandle handle);
    RenderPassOutput&                   renderPassOutput();

};
struct DescriptorSetCreateInfo{
    std::vector<ResourceHandle>         m_resources;
    std::vector<SamplerHandle>          m_samplers;
    std::vector<u32>                    m_bindings;

    DescriptorSetLayoutHandle           m_layout;
    u32                                 m_resourceNums = 0;
    const char*                         name;

    DescriptorSetCreateInfo&            reset();
    DescriptorSetCreateInfo&            setName(const char* name);
    DescriptorSetCreateInfo&            texture(TextureHandle handle, u32 binding);
    DescriptorSetCreateInfo&            buffer(BufferHandle handle, u32 binding);
    DescriptorSetCreateInfo&            textureSampler(TextureHandle tex, SamplerHandle sampler, u32 binding);
    DescriptorSetCreateInfo&            setLayout(DescriptorSetLayoutHandle handle);
};
struct DescriptorSetLayoutCreateInfo{
    struct Binding{
        VkDescriptorType                m_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        u32                             m_start;
        u32                             m_count;
        const char*                     name;

    };
    Binding                             m_bindings[k_max_discriptor_nums_per_set];
    u32                                 m_bindingNum;
    u32                                 m_setIndex;
    const char*                         name;

    DescriptorSetLayoutCreateInfo&      reset();
    DescriptorSetLayoutCreateInfo&      setName(const char* name);
    DescriptorSetLayoutCreateInfo&      addBinding(const Binding& binding);
    DescriptorSetLayoutCreateInfo&      setBindingAtIndex(const Binding& binding, u32 index);
    DescriptorSetLayoutCreateInfo&      setSetIndex(u32 index);
};

struct RenderPassCreateInfo{
    u32                                 m_rtNums;
    RenderPassType::Enum                m_renderPassType = RenderPassType::Geometry;

    std::vector<TextureHandle>          m_outputTextures;
    TextureHandle                       m_depthTexture;

    float                               m_scalex = 1.0f;
    float                               m_scaley = 1.0f;
    u32                                 resize   = 1;

    RenderPassOperation::Enum           m_color             = RenderPassOperation::Enum::DontCare;
    RenderPassOperation::Enum           m_depth             = RenderPassOperation::Enum::DontCare;
    RenderPassOperation::Enum           m_stencil           = RenderPassOperation::Enum::DontCare;

    const char*                         name; 
    RenderPassCreateInfo&               setName(const char* name);
    RenderPassCreateInfo&               addRenderTexture(TextureHandle handle);
    RenderPassCreateInfo&               setScale(float x,float y, u32 resize);
    RenderPassCreateInfo&               setDepthStencilTexture(TextureHandle handle);
    RenderPassCreateInfo&               setType(RenderPassType::Enum type);
    RenderPassCreateInfo&               setOperations(RenderPassOperation::Enum color,RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);
};

struct FrameBufferCreateInfo{
    RenderPassHandle                    m_renderPassHandle;
    u32                                 m_numRenderTargets = 0;
    TextureHandle                       m_outputTextures[k_max_image_outputs];
    TextureHandle                       m_depthStencilTexture;

    u32                                 m_width;
    u32                                 m_height;
    float                               m_scaleX = 1.0f;
    float                               m_scaleY = 1.0f;
    u8                                  resize = 1;
    const char*                         name = nullptr;

    FrameBufferCreateInfo&              reset();
    FrameBufferCreateInfo&              addRenderTarget(TextureHandle texture);
    FrameBufferCreateInfo&              setDepthStencilTexture(TextureHandle depthTexture);
    FrameBufferCreateInfo&              setName(const char* name);
    FrameBufferCreateInfo&              setScaling(float scalex,float scaley, u8 resize);
};

struct Pipeline{
    VkPipeline                          m_pipeline;
    VkPipelineLayout                    m_pipelineLayout;
    VkPipelineBindPoint                 m_bindPoint;
    ShaderStateHandle                   m_shaderState;
    const DescriptorSetLayout*          m_DescriptorSetLayout[k_max_descriptor_set_layouts];
    DescriptorSetLayoutHandle           m_DescriptorSetLayoutHandle[k_max_descriptor_set_layouts];   
    u32                                 m_activeLayouts = 0;
    DepthStencilCreation                m_depthStencil;
    BlendStateCreation                  m_blendState;
    RasterizationCreation               m_rasterization;
    PipelineHandle                      m_pipelineHandle;
    bool                                m_isGraphicsPipeline;                
};

struct ExecutionBarrier{
    PipelineStage::Enum                 srcStage;
    PipelineStage::Enum                 dstStage;
    VkImageLayout                       newLayout;
    u32                                 oldQueueIndex = VK_QUEUE_FAMILY_IGNORED;
    u32                                 newQueueIndex = VK_QUEUE_FAMILY_IGNORED;
    VkAccessFlagBits                    srcImageAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
    VkAccessFlagBits                    dstImageAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
    VkAccessFlagBits                    srcBufferAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;
    VkAccessFlagBits                    dstBufferAccessFlag = VK_ACCESS_FLAG_BITS_MAX_ENUM;

    

    u32                                 numImageBarriers;
    u32                                 numBufferBarriers;
    u32                                 newBarrierExperimental = UINT32_MAX;

    ImageBarrier                        imageBarriers[8];
    MemBarrier                          memoryBarriers[8];

    ExecutionBarrier&                   reset();
    ExecutionBarrier&                   set(PipelineStage::Enum srcStage, PipelineStage::Enum dstStage);
    ExecutionBarrier&                   addImageBarrier(const ImageBarrier& imageBarrier);
    ExecutionBarrier&                   addmemoryBarrier(const MemBarrier& bufferBarrier);
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
    RESOURCE_STATE_GENERIC_READ = ( ( ( ( ( 0x1 | 0x2 ) | 0x40 ) | 0x80 ) | 0x200 ) | 0x800 ),
    RESOURCE_STATE_PRESENT = 0x1000,
    RESOURCE_STATE_COMMON = 0x2000,
    RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE = 0x4000,
    RESOURCE_STATE_SHADING_RATE_SOURCE = 0x8000,
} ResourceState;

struct ResourceUpdate{
    ResourceDelectionType::Enum         type;
    ResourceHandle                      handle;
    u32                                 currentFrame;
};

struct DescriptorSetUpdate{
    DescriptorSetHandle                 handle;
    u32                                 currFrame = 0;
};

struct ConstentData{
    int    tempData;
};


static VkImageType to_vk_image_type(TextureType::Enum e){
    static VkImageType imageTypes[TextureType::UNDEFINED] = {VK_IMAGE_TYPE_1D, VK_IMAGE_TYPE_2D, VK_IMAGE_TYPE_3D, VK_IMAGE_TYPE_1D,VK_IMAGE_TYPE_2D,VK_IMAGE_TYPE_3D};
    return imageTypes[e];
}

static VkImageViewType to_vk_image_view_type(TextureType::Enum e){
    static VkImageViewType imageViewTypes[TextureType::UNDEFINED] = {VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D, VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY,VK_IMAGE_VIEW_TYPE_CUBE_ARRAY};
    return imageViewTypes[e];
}

static VkFormat to_vk_vertex_format( VertexComponentFormat::Enum value ) {
    // Float, Float2, Float3, Float4, Mat4, Byte, Byte4N, UByte, UByte4N, Short2, Short2N, Short4, Short4N, Uint, Uint2, Uint4, Count
    static VkFormat s_vk_vertex_formats[ VertexComponentFormat::Count ] = { VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT, /*MAT4 TODO*/VK_FORMAT_R32G32B32A32_SFLOAT,
                                                                          VK_FORMAT_R8_SINT, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SNORM,
                                                                          VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT };

    return s_vk_vertex_formats[ value ];
}


static VkAccessFlags util_to_vk_access_flags( ResourceState state ) {
    VkAccessFlags ret = 0;
    if ( state & RESOURCE_STATE_COPY_SOURCE ) {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if ( state & RESOURCE_STATE_COPY_DEST ) {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if ( state & RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ) {
        ret |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if ( state & RESOURCE_STATE_INDEX_BUFFER ) {
        ret |= VK_ACCESS_INDEX_READ_BIT;
    }
    if ( state & RESOURCE_STATE_UNORDERED_ACCESS ) {
        ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if ( state & RESOURCE_STATE_INDIRECT_ARGUMENT ) {
        ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if ( state & RESOURCE_STATE_RENDER_TARGET ) {
        ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if ( state & RESOURCE_STATE_DEPTH_WRITE ) {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if ( state & RESOURCE_STATE_SHADER_RESOURCE ) {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if ( state & RESOURCE_STATE_PRESENT ) {
        ret |= VK_ACCESS_MEMORY_READ_BIT;
    }
#ifdef ENABLE_RAYTRACING
    if ( state & RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE ) {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;
    }
#endif

    return ret;
}

static VkPipelineStageFlags to_vk_pipeline_stage( PipelineStage::Enum value ) {
    static VkPipelineStageFlags s_vk_values[] = { VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };
    return s_vk_values[ value ];
}


static VkImageLayout util_to_vk_image_layout( ResourceState usage ) {
    if ( usage & RESOURCE_STATE_COPY_SOURCE )
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if ( usage & RESOURCE_STATE_COPY_DEST )
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if ( usage & RESOURCE_STATE_RENDER_TARGET )
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if ( usage & RESOURCE_STATE_DEPTH_WRITE )
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if ( usage & RESOURCE_STATE_DEPTH_READ )
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    if ( usage & RESOURCE_STATE_UNORDERED_ACCESS )
        return VK_IMAGE_LAYOUT_GENERAL;

    if ( usage & RESOURCE_STATE_SHADER_RESOURCE )
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if ( usage & RESOURCE_STATE_PRESENT )
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if ( usage == RESOURCE_STATE_COMMON )
        return VK_IMAGE_LAYOUT_GENERAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

} //namespace dg

#endif //GPURESOURCE_H
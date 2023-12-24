#include "gpuResource.hpp"

namespace dg{
    BufferCreateInfo& BufferCreateInfo::reset(){
        m_bufferSize = 0;
        m_bufferUsage = 0;
        m_sourceData = nullptr;
        m_deviceOnly = true;
        m_presistent = false;
        return *this;
    }

    BufferCreateInfo& BufferCreateInfo::setData(void* data){
        m_sourceData = data;
        return *this;
    }

    BufferCreateInfo& BufferCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    }

    BufferCreateInfo& BufferCreateInfo::setUsageSize(VkBufferUsageFlags usage,VkDeviceSize size){
        m_bufferUsage = usage;
        m_bufferSize = size;
        return *this;
    }

    BufferCreateInfo& BufferCreateInfo::setDeviceOnly(bool deviceOnly){
        m_deviceOnly = deviceOnly;
        return *this;
    }

    SamplerCreateInfo& SamplerCreateInfo::set_min_mag_mip(VkFilter min, VkFilter max, VkSamplerMipmapMode mipMode){
        m_minFilter = min;
        m_maxFilter = max;
        m_mipmapMode = mipMode;
        return *this;
    }
    SamplerCreateInfo& SamplerCreateInfo::set_address_u(VkSamplerAddressMode u){
        m_addressU = u;
        return *this;
    }
    SamplerCreateInfo& SamplerCreateInfo::set_address_uv(VkSamplerAddressMode u, VkSamplerAddressMode v){
        m_addressU = u;
        m_addressV = v;
        return *this;
    }
    SamplerCreateInfo& SamplerCreateInfo::set_address_uvw(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w){
        m_addressU = u;
        m_addressV = v;
        m_addressW = w;
        return *this;
    }
    SamplerCreateInfo& SamplerCreateInfo::set_name(const char* name){
        this->name = name;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setData(void* data){
        if(!data) {
            DG_ERROR("You are attempting to set an invalid ptr as source data, check again.");
            exit(-1);
        }
        this->m_sourceData = data;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setExtent(VkExtent3D extent){
        this->m_textureExtent = extent;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setFormat(VkFormat format){
        this->m_imageFormat = format;
        return *this;
    }
    
    TextureCreateInfo& TextureCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setMipmapLevel(u32 level){
        this->m_mipmapLevel = level;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setUsage(VkImageUsageFlags usage){
        m_imageUsage |=usage;
        return *this;
    }

    TextureCreateInfo& TextureCreateInfo::setTextureType(TextureType::Enum type){
        m_imageType = type;
        return *this;
    }

    TextureCreateInfo &TextureCreateInfo::setBindLess(bool isBindLess) {
        this->bindless = isBindLess;
        return *this;
    }

    RenderPassCreateInfo& RenderPassCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    } 

    RenderPassCreateInfo& RenderPassCreateInfo::addRenderTexture(TextureHandle handle){
        this->m_outputTextures.push_back(handle);
        m_rtNums = m_outputTextures.size();
        return *this;
    }

    RenderPassCreateInfo& RenderPassCreateInfo::setDepthStencilTexture(TextureHandle handle){
        this->m_depthTexture = handle;
        return *this;
    }

    RenderPassCreateInfo& RenderPassCreateInfo::setOperations(RenderPassOperation::Enum color,RenderPassOperation::Enum depth,RenderPassOperation::Enum stencil){
        this->m_color = color;
        this->m_depth = depth;
        this->m_stencil = stencil;
        return *this;
    }

    RenderPassCreateInfo& RenderPassCreateInfo::setType(RenderPassType::Enum type){
        this->m_renderPassType = type;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::reset(){
        m_renderPassHandle = {k_invalid_index};
        m_numRenderTargets = 0;
        m_depthStencilTexture = {k_invalid_index};
        m_width = 0;
        m_height = 0;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::addRenderTarget(TextureHandle texture){
        if(m_numRenderTargets==k_max_image_outputs){
            DG_ERROR("There is already ", k_max_image_outputs," color attachments in this FrameBuffer, can add more");
            exit(-1);
        }
        m_outputTextures[m_numRenderTargets++] = texture;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::setScaling(float scalex,float scaley, u8 resize){
        m_scaleX = scalex;
        m_scaleY = scaley;
        this->resize = resize;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::setRenderPass(RenderPassHandle rp){
        this->m_renderPassHandle = rp;
        return *this;
    }

    FrameBufferCreateInfo& FrameBufferCreateInfo::setExtent(VkExtent2D extent){
        if(extent.width==0||extent.height==0||extent.height>7800||extent.width>7800){
            DG_ERROR("Invalid frameBuffer extent,check again!");
            exit(-1);
        }
        m_width = extent.width;
        m_height = extent.height;
        return *this;
    }

    DescriptorSetLayoutCreateInfo& DescriptorSetLayoutCreateInfo::reset(){
        m_bindingNum = 0;
        m_setIndex = 0;
        return *this;
    } 

    DescriptorSetLayoutCreateInfo& DescriptorSetLayoutCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    } 

    DescriptorSetLayoutCreateInfo& DescriptorSetLayoutCreateInfo::addBinding(const Binding& binding){
        if(m_bindingNum>=k_max_discriptor_nums_per_set){
            DG_ERROR("Max number of binding is ", std::to_string(k_max_discriptor_nums_per_set)," can't add more binding!")
            exit(-1);
        }
        m_bindings[m_bindingNum++] = binding;
        return *this;
    }

    DescriptorSetLayoutCreateInfo& DescriptorSetLayoutCreateInfo::setBindingAtIndex(const Binding& binding, u32 index){
        m_bindings[index] = binding;
        m_bindingNum = index+1 >m_bindingNum?(index+1):m_bindingNum;
        return *this;
    }

    DescriptorSetLayoutCreateInfo& DescriptorSetLayoutCreateInfo::setSetIndex(u32 index){
        m_setIndex = index;
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::reset(){
        m_bindings.resize(k_max_discriptor_nums_per_set);
        m_resourceNums = 0;
        m_resources.resize(k_max_discriptor_nums_per_set);
        m_layout = {k_invalid_index};
        m_samplers.resize(k_max_discriptor_nums_per_set);
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::buffer(BufferHandle handle, u32 binding){
        m_resources[m_resourceNums] = handle.index;
        m_bindings[m_resourceNums] = binding;
        m_samplers[m_resourceNums] = {k_invalid_index};
        ++m_resourceNums;
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::texture(TextureHandle handle, u32 binding){
        m_resources[m_resourceNums] = handle.index;
        m_bindings[m_resourceNums] = binding;
        m_samplers[m_resourceNums] = {k_invalid_index};
        ++m_resourceNums;
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::setLayout(DescriptorSetLayoutHandle handle){
        m_layout = handle;
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::textureSampler(TextureHandle handle, SamplerHandle sampler,u32 binding){
        m_resources[m_resourceNums] = handle.index;
        m_samplers[m_resourceNums] = sampler;
        m_bindings[m_resourceNums] = binding;
        ++m_resourceNums;
        return *this;
    }

    DescriptorSetCreateInfo& DescriptorSetCreateInfo::setName(const char* name){
        this->name = name;
        return *this;
    }

    DepthStencilCreation& DepthStencilCreation::setDepth(bool enable, bool write, VkCompareOp testOp){
        m_depthEnable = enable;
        m_depthWriteEnable = write;
        m_depthCompare = testOp;
        return *this;
    }

    BlendState& BlendState::setAlpha(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp operation){
        m_sourceColor = sourceColor;
        m_destinationColor = destinColor;
        m_alphaOperation = operation;
        m_blendEnabled = true;
        return *this;
    }

    BlendState& BlendState::setColor(VkBlendFactor sourceColor, VkBlendFactor destinColor, VkBlendOp operation){
        m_sourceColor = sourceColor;
        m_destinationColor = destinColor;
        m_alphaOperation = operation;
        m_blendEnabled = true;
        return *this;
    }

    BlendState& BlendState::setColorWriteMask(ColorWriteEnabled::Mask value){
        m_colorWriteMask = value;
        return *this;
    }

    BlendStateCreation& BlendStateCreation::reset(){
        m_activeStates = 0;
        return *this;
    }

    BlendState& BlendStateCreation::add_blend_state()
    {
        if(m_activeStates>=k_max_image_outputs){
            DG_ERROR("Max number of blend state is ", std::to_string(k_max_image_outputs)," can't add more blend state!")
            exit(-1);
        }
        return m_blendStates[m_activeStates++];
    }

    ShaderStateCreation& ShaderStateCreation::reset(){
        m_spvInput = 0;
        m_stageCount = 0;
        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::setName(const char* name){
        this->name = name;
        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::addStage(const char* code ,u32 codeSize, VkShaderStageFlagBits type){
        if(m_stageCount>=k_max_shader_stages){
            DG_ERROR("Max number of shader stages is ", std::to_string(k_max_shader_stages)," can't add more shader stage!")
            exit(-1);
        }
        m_stages[m_stageCount].m_code = code;
        m_stages[m_stageCount].m_codeSize = codeSize;
        m_stages[m_stageCount].m_type = type;
        ++m_stageCount;
        setSpvInput(true);
        return *this;
    }

    ShaderStateCreation& ShaderStateCreation::setSpvInput(bool value){
        m_spvInput = value;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::reset(){
        m_numColorFormats = 0;
        for(int i = 0;i<k_max_image_outputs;++i){
            m_colorFormats[i] = VK_FORMAT_UNDEFINED;
        }
        m_depthStencilFormat = VK_FORMAT_UNDEFINED;
        m_colorOperation = RenderPassOperation::DontCare;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::color(VkFormat format){
        m_colorFormats[m_numColorFormats++] = format;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::depth(VkFormat format){
        m_depthStencilFormat = format;
        return *this;
    }

    RenderPassOutput& RenderPassOutput::setOperations(RenderPassOperation::Enum color, RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil){
        m_colorOperation = color;
        m_depthOperation = depth;
        m_stencilOperation = stencil;
        return *this;
    }

    pipelineCreateInfo& pipelineCreateInfo::addDescriptorSetlayout(const DescriptorSetLayoutHandle& info){
        if(m_numActivateLayouts>=k_max_descriptor_set_layouts){
            DG_ERROR("The max descriptor set layout num for per pipeline is limited to ", std::to_string(k_max_descriptor_set_layouts)," can't add more layout info!")
            exit(-1);
        }
        m_descLayout[m_numActivateLayouts++] = info;
        return *this;
    }

    RenderPassOutput& pipelineCreateInfo::renderPassOutput(){
        return m_renderPassOutput;
    }

    bool RenderPass::operator==(RenderPass& pass){
        if(pass.m_outputTextures.size()!=m_outputTextures.size()) return false;
        for(int i = 0;i<m_outputTextures.size();++i){
            if(m_outputTextures[i].index!=pass.m_outputTextures[i].index) return false;
        }
        if(m_depthTexture.index!=pass.m_depthTexture.index) return false;
        return true;
    }

    bool RenderPass::operator!=(RenderPass& pass){
        if(pass.m_outputTextures.size()!=m_outputTextures.size()) return true;
        for(int i = 0;i<m_outputTextures.size();++i){
            if(m_outputTextures[i].index!=pass.m_outputTextures[i].index) return true;
        }
        if(m_depthTexture.index!=pass.m_depthTexture.index) return true;
        return false;
    }

    ExecutionBarrier&   ExecutionBarrier::reset(){
        srcStage = PipelineStage::Enum::DrawIndirect;
        dstStage = PipelineStage::Enum::DrawIndirect;
        numBufferBarriers = numImageBarriers = 0;
        return *this;
    }

    ExecutionBarrier&   ExecutionBarrier::set(PipelineStage::Enum src,PipelineStage::Enum dst){
        srcStage = src;
        dstStage = dst;
        return *this;
    }

    ExecutionBarrier&   ExecutionBarrier::addImageBarrier(const ImageBarrier& imageBarrier){
        if(numBufferBarriers>=maxBarrierNum){
            DG_ERROR("Max barrier num is", std::to_string(maxBarrierNum),"can't add more image barrier!")
            exit(-1);
        }
        imageBarriers[numImageBarriers++] = imageBarrier;
        return *this;
    }

    ExecutionBarrier&   ExecutionBarrier::addmemoryBarrier(const MemBarrier& memBarrier){
        if(numBufferBarriers>=maxBarrierNum){
            DG_ERROR("Max barrier num is", std::to_string(maxBarrierNum),"can't add more memory barrier!")
            exit(-1);
        }
        memoryBarriers[numBufferBarriers++] = memBarrier;
        return *this;
    }
}   
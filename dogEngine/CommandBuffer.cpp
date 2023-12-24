
#include "CommandBuffer.h"

namespace dg{
    void CommandBuffer::init(DeviceContext* context){
        m_context = context;
        reset();
    }

    void CommandBuffer::destroy(){
        m_isRecording = false;
        reset();
    }

    void CommandBuffer::reset(){
        m_currRenderPass = nullptr;
        m_isRecording = false;
        m_pipeline = nullptr;
        m_currentCommand = 0;
        m_clears[0].color = {{0.2f,0.2f,0.2f,1.0f}};
        m_clears[1].depthStencil = {1.0f,0};
    }

    void CommandBuffer::bindPass(RenderPassHandle pass){
        m_isRecording = true;
        RenderPass* renderpas = m_context->accessRenderPass(pass);
        if(m_currRenderPass&&(m_currRenderPass->m_type!=RenderPassType::Enum::Compute)&&renderpas!=m_currRenderPass){
            vkCmdEndRenderPass(m_commandBuffer);
        }
        if(renderpas!=m_currRenderPass&&(renderpas->m_type!=RenderPassType::Enum::Compute)){
            VkRenderPassBeginInfo  passBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            passBeginInfo.renderPass = renderpas->m_renderPass;
            passBeginInfo.renderArea.extent = {renderpas->m_width,renderpas->m_height};
            passBeginInfo.clearValueCount = m_clears.size();
            passBeginInfo.pClearValues = m_clears.data();
            passBeginInfo.framebuffer =renderpas->m_type==RenderPassType::Enum::SwapChain? m_context->m_swapchainFbos[m_context->m_currentSwapchainImageIndex] : renderpas->m_frameBuffer;
            m_currFrameBuffer = passBeginInfo.framebuffer;
            passBeginInfo.renderArea.offset = {0,0};
            vkCmdBeginRenderPass(m_commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }
        m_currRenderPass = renderpas;

    }   

    void CommandBuffer::bindPipeline(PipelineHandle pip){
        m_isRecording = true;
        Pipeline* pipline = m_context->accessPipeline(pip);
        if(!pipline){DG_ERROR("You are trying to bind an invalid pipeline")}
        vkCmdBindPipeline(m_commandBuffer, pipline->m_bindPoint, pipline->m_pipeline);
        m_pipeline = pipline;
    }

    void CommandBuffer::bindVertexBuffer(BufferHandle vb, u32 binding, u32 offset){
        Buffer* vbuffer = m_context->accessBuffer(vb);
        if(!vbuffer){DG_ERROR("You are trying to bind an invalid vertex buffer")}
        VkBuffer realBuffer = vbuffer->m_buffer;
        VkDeviceSize offse = offset;
        if(vbuffer->m_parentHandle.index!=k_invalid_index){
            Buffer* parentBuffer = m_context->accessBuffer(vbuffer->m_parentHandle);
            realBuffer = parentBuffer->m_buffer;
            offse = vbuffer->m_globalOffset;
        }
        vkCmdBindVertexBuffers(m_commandBuffer, binding, 1, &realBuffer, (VkDeviceSize*)&offse);
    }

    void CommandBuffer::bindIndexBuffer(BufferHandle ib,u32 offset, VkIndexType iType){
        Buffer* ibuffer = m_context->accessBuffer(ib);
        if(!ibuffer){DG_ERROR("You are trying to bind an invalid index Buffer")}
        VkBuffer realBuffer = ibuffer->m_buffer;
        VkDeviceSize offse{};
        if(ibuffer->m_parentHandle.index!=k_invalid_index){
            Buffer* parentBuffer = m_context->accessBuffer(ibuffer->m_parentHandle);
            realBuffer = parentBuffer->m_buffer;
            offse = ibuffer->m_globalOffset;
        }
        vkCmdBindIndexBuffer(m_commandBuffer, realBuffer, offse, iType);
    }

    void CommandBuffer::bindDescriptorSet(std::vector<DescriptorSetHandle> set, u32 firstSet, u32* offsets, u32 numOffsets){
        for(int i = 0;i<set.size();++i){
            DescriptorSet* dset = m_context->accessDescriptorSet(set[i]);
            if(!dset){DG_ERROR("You are trying to bind an invalid descriptor set")}
            m_descriptorSets[i] = dset->m_vkdescriptorSet;
        }
        vkCmdBindDescriptorSets(m_commandBuffer, m_pipeline->m_bindPoint, m_pipeline->m_pipelineLayout, firstSet, set.size(), m_descriptorSets, 0, nullptr);
    }

    void CommandBuffer::setScissor(const Rect2DInt* rect){
        VkRect2D ret;
        if(rect){
            ret.offset.x = rect->x;
            ret.offset.y = rect->y;
            ret.extent.width = rect->width;
            ret.extent.height = rect->height;
        }   else{
            ret.offset = {0,0};
            ret.extent = {m_context->m_swapChainWidth, m_context->m_swapChainHeight};
        }
        vkCmdSetScissor(m_commandBuffer, 0, 1, &ret);
    }

    void CommandBuffer::setDepthStencilState(VkBool32 enable){
        vkCmdSetDepthTestEnable(m_commandBuffer, enable);
    }

    void CommandBuffer::setViewport(const ViewPort* vp){
        VkViewport viewPort;
        if(vp)
        {
            viewPort.x = vp->rect.x;
            viewPort.y = vp->rect.y;
            viewPort.width = vp->rect.width;
            viewPort.height = vp->rect.height;
            viewPort.maxDepth  = vp->max_depth;
            viewPort.minDepth = vp->min_depth;
        }else {
            viewPort.x = 0;
            viewPort.y = 0;
            viewPort.width = m_context->m_swapChainWidth;
            viewPort.height = m_context->m_swapChainHeight;
            viewPort.maxDepth = 1.0f;
            viewPort.minDepth = 0.0f;
        }
        vkCmdSetViewport(m_commandBuffer, 0, 1, &viewPort);
    }

    void CommandBuffer::clearColor(){
        m_clears[0].color = {0.0,0.0,0.0,1.0f};
    }

    void CommandBuffer::clearDepthStencil(float depth, u8 stencil){
        m_clears[1].depthStencil.depth = depth;
        m_clears[1].depthStencil.stencil = stencil;
    }

    void CommandBuffer::draw(TopologyType::Enum tpType, u32 firstVertex, u32 VertexCount, u32 firstInstance, u32 instanceNum){
        vkCmdDraw(m_commandBuffer, VertexCount, instanceNum, firstVertex, firstInstance);
    }

    void CommandBuffer::drawIndexed(TopologyType::Enum tpType, u32 indexCount, u32 instanceCount, u32 firstIndex,u32 vertexOffset, u32 firstInstance){
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer::disPatch(u32 groupX, u32 groupY, u32 groupZ){
        vkCmdDispatch(m_commandBuffer, groupX, groupY, groupZ);
    }

    void CommandBuffer::barrier(const ExecutionBarrier& barrier){
        if(m_currRenderPass&&(m_currRenderPass->m_type!=RenderPassType::Enum::Compute)){
            vkCmdEndRenderPass(m_commandBuffer);
            m_currRenderPass = nullptr;
        }
        std::vector<VkImageMemoryBarrier> imageBarriers;
        std::vector<VkBufferMemoryBarrier> bufferBarriers;

        VkAccessFlags sourceAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags sourceBufferAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags sourceDepthAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkAccessFlags dstAccessMask = VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;
        VkAccessFlags dstBufferAccessmask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;
        VkAccessFlags dstDepthAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        bool isDepth = false;
        if(barrier.numImageBarriers){
            imageBarriers.resize(barrier.numBufferBarriers);
            for(int i = 0;i<imageBarriers.size();++i){
                Texture* tex = m_context->accessTexture(barrier.imageBarriers[i].texture);
                isDepth = TextureFormat::has_depth_or_stencil(tex->m_format);
                if(!tex){DG_ERROR("You are trying to add a barrier for an invalid texture")}
                VkImageMemoryBarrier& imgBarr = imageBarriers[i];
                imgBarr.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imgBarr.image = tex->m_image;
                imgBarr.oldLayout = tex->m_imageInfo.imageLayout;
                imgBarr.newLayout = barrier.newLayout;
                imgBarr.srcQueueFamilyIndex = barrier.oldQueueIndex!=VK_QUEUE_FAMILY_IGNORED?barrier.oldQueueIndex:VK_QUEUE_FAMILY_IGNORED;
                imgBarr.dstQueueFamilyIndex = barrier.newQueueIndex!=VK_QUEUE_FAMILY_IGNORED?barrier.newQueueIndex:VK_QUEUE_FAMILY_IGNORED;
                imgBarr.subresourceRange.aspectMask = isDepth?VK_IMAGE_ASPECT_COLOR_BIT:VK_IMAGE_ASPECT_DEPTH_BIT;
                imgBarr.subresourceRange.baseArrayLayer = 0;
                imgBarr.subresourceRange.baseMipLevel = 0;
                imgBarr.subresourceRange.layerCount = 1;
                imgBarr.subresourceRange.levelCount = 1;
                imgBarr.srcAccessMask = barrier.srcImageAccessFlag!=VK_ACCESS_FLAG_BITS_MAX_ENUM?barrier.srcImageAccessFlag:sourceAccessMask;
                imgBarr.dstAccessMask = barrier.dstImageAccessFlag!=VK_ACCESS_FLAG_BITS_MAX_ENUM?barrier.dstImageAccessFlag:dstAccessMask;
                tex->m_imageInfo.imageLayout = imgBarr.newLayout;
            }
        }
        if(barrier.numBufferBarriers){
            bufferBarriers.resize(barrier.numBufferBarriers);
            for(int i = 0;i<bufferBarriers.size();++i){
                Buffer* buf = m_context->accessBuffer(barrier.memoryBarriers[i].buffer);
                if(!buf){DG_ERROR("You are trying to add a barrier for an invalid buffer")}
                VkBufferMemoryBarrier& bufBarr = bufferBarriers[i];
                bufBarr.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufBarr.buffer = buf->m_buffer;
                bufBarr.srcAccessMask = barrier.srcBufferAccessFlag!=VK_ACCESS_FLAG_BITS_MAX_ENUM?barrier.srcBufferAccessFlag:sourceAccessMask;
                bufBarr.dstAccessMask = barrier.dstBufferAccessFlag!=VK_ACCESS_FLAG_BITS_MAX_ENUM?barrier.dstBufferAccessFlag:dstAccessMask;
                bufBarr.srcQueueFamilyIndex = barrier.oldQueueIndex!=VK_QUEUE_FAMILY_IGNORED?barrier.oldQueueIndex:VK_QUEUE_FAMILY_IGNORED;
                bufBarr.dstQueueFamilyIndex = barrier.newQueueIndex!=VK_QUEUE_FAMILY_IGNORED?barrier.newQueueIndex:VK_QUEUE_FAMILY_IGNORED;
                bufBarr.size = buf->m_size;
                bufBarr.offset = 0;
            }
        }
        VkShaderStageFlags srcPipFlags = to_vk_pipeline_stage(barrier.srcStage);
        VkShaderStageFlags dstPipFlags = to_vk_pipeline_stage(barrier.dstStage);
        if(isDepth){
            srcPipFlags |=VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dstPipFlags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        vkCmdPipelineBarrier(m_commandBuffer, srcPipFlags, dstPipFlags, 0, 0, nullptr, bufferBarriers.size(), bufferBarriers.data(), imageBarriers.size(), imageBarriers.data());
    }



} //namespace dg
#ifndef COMMANDBUFFERHANDLER_H
#define COMMANDBUFFERHANDLER_H
#include "DeviceContext.h"
#include <vulkan/vulkan.h>

namespace dg {

struct CommandBuffer {
  void init(DeviceContext* context);
  void destroy();

  void begin();
  void end();

  //just for temproary one time submit command buffer
  void flush(VkQueue queue);
  void bindPass(RenderPassHandle pass);
  void bindPipeline(PipelineHandle pip);
  void bindVertexBuffer(BufferHandle vb, u32 binding, u32 offset);
  void bindIndexBuffer(BufferHandle ib, u32 offset, VkIndexType index_type);
  void bindDescriptorSet(const std::vector<DescriptorSetHandle>& set, u32 firstSet, u32* offsets,
                         u32 numOffsets);
  template<typename T>
  void bindPushConstants(VkShaderStageFlags pipelineStage, T* pushBlock);
  void endpass() {
    vkCmdEndRenderPass(m_commandBuffer);
    m_currRenderPass = nullptr;
    m_isRecording = false;
  }

  void setScissor(const Rect2DInt* rect);
  void setViewport(const ViewPort* viewport);
  void setDepthStencilState(VkBool32 enable);

  void clearColor();
  void clearDepthStencil(float depth, u8 stencil);

  void draw(TopologyType::Enum tpType, u32 firstVertex, u32 VertexCount, u32 firstInstance,
            u32 instanceNum);
  void drawIndexed(TopologyType::Enum tpType, u32 indexCount, u32 instanctCount, u32 firstIndex,
                   u32 vertexOffset, u32 firstInstance);
  void traceRay(u32 width, u32 height, u32 depth);

  void disPatch(u32 groupX, u32 groupY, u32 groupZ);

  void barrier(const ExecutionBarrier& barrier);

  void reset();

  DeviceContext*              m_context;
  VkCommandBuffer             m_commandBuffer;
  VkDescriptorSet             m_descriptorSets[16];
  RenderPass*                 m_currRenderPass;
  Pipeline*                   m_pipeline;
  VkFramebuffer               m_currFrameBuffer;
  std::array<VkClearValue, 2> m_clears;
  ResourceHandle              m_handle;
  u32                         m_currentCommand;
  bool                        m_isBegin = false;
  bool                        m_isRecording;
  bool                        m_baked;
};
template<typename T>
void CommandBuffer::bindPushConstants(VkShaderStageFlags pipelineStage, T* pushBlock) {
  vkCmdPushConstants(m_commandBuffer, m_pipeline->m_pipelineLayout, pipelineStage, 0, sizeof(T),
                     pushBlock);
}
}// namespace dg

#endif//COMMANDBUFFERHANDLER_H
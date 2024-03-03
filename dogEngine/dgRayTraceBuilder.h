#ifndef DGRAYTRACEBUILDER_H
#define DGRAYTRACEBUILDER_H
#include "CommandBuffer.h"

namespace dg {

class RayTracingBuilder {
 public:
  struct BlasInput {
    std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
    VkBuildAccelerationStructureFlagsKHR                  flags{0};
  };

  void            setup(DeviceContext* context);
  void            destroy();
  AccelKHR        getAccelerationStructure() const;
  VkDeviceAddress getBlasDeviceAddress(u32 blasID);
  void            buildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
  void            updateBlas(u32 blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);
  void            buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, bool update = false);
  void            cmdCreateTlas(CommandBuffer* cmd, u32 instanceCount, VkDeviceAddress instBufferAddr, BufferHandle& scratchBufferHandle, VkBuildAccelerationStructureFlagsKHR flags, bool update, bool motion);

  template<class T>
  void buildTlas(const std::vector<T>&                instances,
                 VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
                 bool                                 update = false,
                 bool                                 motion = false) {
    DGASSERT(m_tlas.accel == VK_NULL_HANDLE || update);
    u32              countInstance = static_cast<u32>(instances.size());
    CommandBuffer*   cmd = m_context->getInstantCommandBuffer();
    BufferCreateInfo instanceBuffer{};
    instanceBuffer.reset().setName("InstanceBuffer").setDeviceOnly(true).setUsageSize(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, sizeof(T) * instances.size()).setData(instances.data());
    BufferHandle bufferHandle = m_context->createBuffer(instanceBuffer);
    Buffer*      buffer = m_context->accessBuffer(bufferHandle);

    cmd->begin();
    VkBufferDeviceAddressInfo bufferAddInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    bufferAddInfo.buffer = buffer->m_buffer;
    VkDeviceAddress instBufferAddr = vkGetBufferDeviceAddress(m_context->m_logicDevice, &bufferAddInfo);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(cmd->m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    BufferHandle scratchBuffer{k_invalid_index};
    cmdCreateTlas(cmd, countInstance, instBufferAddr, scratchBuffer, flags, update, motion);
    cmd->flush(m_context->m_graphicsQueue);
    m_context->destroyBufferInstant(scratchBuffer.index);
    m_context->destroyBufferInstant(bufferHandle.index);
  }

 protected:
  std::vector<AccelKHR> m_blas;
  AccelKHR              m_tlas;

  DeviceContext* m_context;
  VkCommandPool  m_commandPool;

  struct BuildAccelerationStructure {
    VkAccelerationStructureBuildGeometryInfoKHR     buildInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    VkAccelerationStructureBuildSizesInfoKHR        sizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
    AccelKHR                                        as;
    AccelKHR                                        cleanupAs;
  };

  void cmdCreateBlas(CommandBuffer* cmd, std::vector<u32> indices, std::vector<BuildAccelerationStructure>& buildAs, VkDeviceAddress scratchAddress, VkQueryPool queryPool);
  void cmdCompactBlas(CommandBuffer* cmd, std::vector<u32> indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool);
  void destroyNonCompacted(std::vector<u32> indices, std::vector<BuildAccelerationStructure>& buildAs);
  bool hasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; };
};

}// namespace dg

#endif// DGRAYTRACEBUILDER_H
#include "dgRayTraceBuilder.h"
namespace dg {

void RayTracingBuilder::setup(DeviceContext* context) { m_context = context; }

void RayTracingBuilder::destroy() {
  for (auto& i : m_blas) {
    vkDestroyAccelerationStructureKHR(m_context->m_logicDevice, i.accel, nullptr);
    m_context->DestroyBuffer(i.buffer);
  }
  vkDestroyAccelerationStructureKHR(m_context->m_logicDevice, m_tlas.accel, nullptr);
  m_context->DestroyBuffer(m_tlas.buffer);
}

AccelKHR RayTracingBuilder::getAccelerationStructure() const { return m_tlas; }

VkDeviceAddress RayTracingBuilder::getBlasDeviceAddress(u32 blasID) {
  DGASSERT(blasID < m_blas.size());
  Buffer*                   blasBuffer = m_context->accessBuffer(m_blas[blasID].buffer);
  VkBufferDeviceAddressInfo bufferAddInfo{
      VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
  };
  bufferAddInfo.buffer = blasBuffer->m_buffer;

  return vkGetBufferDeviceAddressKHR(m_context->m_logicDevice, &bufferAddInfo);
}

void RayTracingBuilder::buildBlas(const std::vector<BlasInput>&        input,
                                  VkBuildAccelerationStructureFlagsKHR flags) {
  CommandBuffer* cmd = m_context->getInstantCommandBuffer();
  cmd->begin();
  auto         nbBlas = static_cast<u32>(input.size());
  VkDeviceSize asTotalSize = 0;
  u32          nbCompactions = 0;
  VkDeviceSize maxScrachSize = 0;

  std::vector<BuildAccelerationStructure> buildAs(nbBlas);
  for (u32 idx = 0; idx < nbBlas; ++idx) {
    buildAs[idx].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildAs[idx].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildAs[idx].buildInfo.flags = input[idx].flags | flags;
    buildAs[idx].buildInfo.geometryCount = static_cast<u32>(input[idx].asGeometry.size());
    buildAs[idx].buildInfo.pGeometries = input[idx].asGeometry.data();

    buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo.data();
    std::vector<u32> maxPrimCount(input[idx].asBuildOffsetInfo.size());
    for (auto tt = 0; tt < input[idx].asBuildOffsetInfo.size(); tt++) {
      maxPrimCount[tt] = input[idx].asBuildOffsetInfo[tt].primitiveCount;
    }
    vkGetAccelerationStructureBuildSizesKHR(
        m_context->m_logicDevice, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildAs[idx].buildInfo, maxPrimCount.data(), &buildAs[idx].sizeInfo);
    asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
    maxScrachSize = std::max(maxScrachSize, buildAs[idx].sizeInfo.buildScratchSize);
    nbCompactions += hasFlag(buildAs[idx].buildInfo.flags,
                             VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
  }
  BufferCreateInfo scratchBufferInfo{};
  scratchBufferInfo.reset()
      .setUsageSize(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    maxScrachSize)
      .setDeviceOnly(true)
      .setAlignment(128)
      .setName("blasScratchBufferInfo");
  auto                      scratchBufferHandle = m_context->createBuffer(scratchBufferInfo);
  Buffer*                   scratchBuffer = m_context->accessBuffer(scratchBufferHandle);
  VkBufferDeviceAddressInfo bufferAddInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  bufferAddInfo.buffer = scratchBuffer->m_buffer;
  VkDeviceAddress scrachAddress =
      vkGetBufferDeviceAddress(m_context->m_logicDevice, &bufferAddInfo);

  VkQueryPool queryPool{VK_NULL_HANDLE};
  if (nbCompactions > 0) {
    DGASSERT(nbCompactions == nbBlas);
    VkQueryPoolCreateInfo qpci{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    qpci.queryCount = nbBlas;
    qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    vkCreateQueryPool(m_context->m_logicDevice, &qpci, nullptr, &queryPool);
  }

  std::vector<u32> indices;
  VkDeviceSize     batchSize = 0;
  VkDeviceSize     batchLimit{256'000'000};//256MB
  for (u32 idx = 0; idx < nbBlas; ++idx) {
    indices.push_back(idx);
    batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
    if (batchSize >= batchLimit || idx == nbBlas - 1) {
      cmdCreateBlas(cmd, indices, buildAs, scrachAddress, queryPool);
      cmd->flush(m_context->m_graphicsQueue);
      if (queryPool) {
        cmd->begin();
        cmdCompactBlas(cmd, indices, buildAs, queryPool);
        cmd->flush(m_context->m_graphicsQueue);
        destroyNonCompacted(indices, buildAs);
      }
      batchSize = 0;
      indices.clear();
    }
  }
  for (auto& b : buildAs) { m_blas.emplace_back(b.as); }
  vkDestroyQueryPool(m_context->m_logicDevice, queryPool, nullptr);
  m_context->destroyBufferInstant(scratchBufferHandle.index);
}

void RayTracingBuilder::cmdCreateBlas(CommandBuffer* cmd, std::vector<u32> indices,
                                      std::vector<BuildAccelerationStructure>& buildAs,
                                      VkDeviceAddress scratchAddress, VkQueryPool queryPool) {
  if (queryPool) {
    vkResetQueryPool(m_context->m_logicDevice, queryPool, 0, static_cast<u32>(indices.size()));
  }
  u32 queryCnt = 0;
  for (const auto& idx : indices) {
    VkAccelerationStructureCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
    BufferCreateInfo bufferInfo{};
    bufferInfo.reset().setUsageSize(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                                        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                    createInfo.size);
    buildAs[idx].as.buffer = m_context->createBuffer(bufferInfo);
    Buffer* buffer = m_context->accessBuffer(buildAs[idx].as.buffer);
    createInfo.buffer = buffer->m_buffer;

    vkCreateAccelerationStructureKHR(m_context->m_logicDevice, &createInfo, nullptr,
                                     &buildAs[idx].as.accel);
    DebugMessanger::GetInstance()->setObjectName((uint64_t) buildAs[idx].as.accel,
                                                 std::string("blas") + std::to_string(idx),
                                                 VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);

    buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.accel;
    buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;

    vkCmdBuildAccelerationStructuresKHR(cmd->m_commandBuffer, 1, &buildAs[idx].buildInfo,
                                        &buildAs[idx].rangeInfo);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmd->m_commandBuffer,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                         VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0,
                         nullptr, 0, nullptr);
    if (queryPool) {
      vkCmdWriteAccelerationStructuresPropertiesKHR(
          cmd->m_commandBuffer, 1, &buildAs[idx].buildInfo.dstAccelerationStructure,
          VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
    }
  }
}

void RayTracingBuilder::cmdCompactBlas(CommandBuffer* cmd, std::vector<u32> indices,
                                       std::vector<BuildAccelerationStructure>& buildAs,
                                       VkQueryPool                              queryPool) {
  u32                       queryCnt = 0;
  std::vector<VkDeviceSize> compactSizes(static_cast<u32>(indices.size()));
  vkGetQueryPoolResults(m_context->m_logicDevice, queryPool, 0, (u32) compactSizes.size(),
                        compactSizes.size() * sizeof(VkDeviceSize), compactSizes.data(),
                        sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);
  for (auto idx : indices) {
    buildAs[idx].cleanupAs = buildAs[idx].as;
    buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCnt++];

    VkAccelerationStructureCreateInfoKHR asCreateInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
    BufferCreateInfo bufferInfo{};
    bufferInfo.reset().setUsageSize(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                                        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                    asCreateInfo.size);
    buildAs[idx].as.buffer = m_context->createBuffer(bufferInfo);
    Buffer* buffer = m_context->accessBuffer(buildAs[idx].as.buffer);
    asCreateInfo.buffer = buffer->m_buffer;
    vkCreateAccelerationStructureKHR(m_context->m_logicDevice, &asCreateInfo, nullptr,
                                     &buildAs[idx].as.accel);
    DebugMessanger::GetInstance()->setObjectName((uint64_t) buildAs[idx].as.accel,
                                                 std::string("blas") + std::to_string(idx),
                                                 VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
    VkCopyAccelerationStructureInfoKHR copyInfo{
        VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
    copyInfo.src = buildAs[idx].cleanupAs.accel;
    copyInfo.dst = buildAs[idx].as.accel;
    copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
    vkCmdCopyAccelerationStructureKHR(cmd->m_commandBuffer, &copyInfo);
  }
}

void RayTracingBuilder::destroyNonCompacted(std::vector<u32>                         indices,
                                            std::vector<BuildAccelerationStructure>& buildAs) {
  for (auto& i : indices) {
    vkDestroyAccelerationStructureKHR(m_context->m_logicDevice, buildAs[i].cleanupAs.accel,
                                      nullptr);
    m_context->destroyBufferInstant(buildAs[i].cleanupAs.buffer.index);
    buildAs[i].cleanupAs.buffer = {k_invalid_index};
  }
}

void RayTracingBuilder::buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
                                  VkBuildAccelerationStructureFlagsKHR flags, bool update) {
  buildTlas(instances, flags, update, false);
}

void RayTracingBuilder::cmdCreateTlas(CommandBuffer* cmd, u32 instanceCount,
                                      VkDeviceAddress                      instBufferAddr,
                                      BufferHandle&                        scratchBufferHandle,
                                      VkBuildAccelerationStructureFlagsKHR flags, bool update,
                                      bool motion) {
  VkAccelerationStructureGeometryInstancesDataKHR instancesVk{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
  instancesVk.data.deviceAddress = instBufferAddr;

  VkAccelerationStructureGeometryKHR topASGeometry{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
  topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  topASGeometry.geometry.instances = instancesVk;

  VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
  buildInfo.flags = flags;
  buildInfo.geometryCount = 1;
  buildInfo.pGeometries = &topASGeometry;
  buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
                          : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  VkAccelerationStructureBuildSizesInfoKHR sizeInfo{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  vkGetAccelerationStructureBuildSizesKHR(m_context->m_logicDevice,
                                          VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                          &buildInfo, &instanceCount, &sizeInfo);
  if (update == false) {
    VkAccelerationStructureCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    createInfo.size = sizeInfo.accelerationStructureSize;
    BufferCreateInfo tlasBufferInfo{};
    tlasBufferInfo.reset()
        .setName("tlasBuffer")
        .setUsageSize(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                          | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                      createInfo.size)
        .setDeviceOnly(true);
    m_tlas.buffer = m_context->createBuffer(tlasBufferInfo);
    Buffer* tlasBuffer = m_context->accessBuffer(m_tlas.buffer);
    createInfo.buffer = tlasBuffer->m_buffer;
    vkCreateAccelerationStructureKHR(m_context->m_logicDevice, &createInfo, nullptr, &m_tlas.accel);
    DebugMessanger::GetInstance()->setObjectName((uint64_t) m_tlas.accel, "tlas",
                                                 VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
  }
  BufferCreateInfo scratchBufferInfo{};
  scratchBufferInfo.reset()
      .setDeviceOnly(true)
      .setName("scratchBuffer")
      .setAlignment(128)
      .setUsageSize(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    sizeInfo.buildScratchSize);

  scratchBufferHandle = m_context->createBuffer(scratchBufferInfo);
  Buffer*                   scratchBuffer = m_context->accessBuffer(scratchBufferHandle);
  VkBufferDeviceAddressInfo scratchAddrInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
  scratchAddrInfo.buffer = scratchBuffer->m_buffer;
  VkDeviceAddress scratchAddr =
      vkGetBufferDeviceAddressKHR(m_context->m_logicDevice, &scratchAddrInfo);

  buildInfo.srcAccelerationStructure = update ? m_tlas.accel : VK_NULL_HANDLE;
  buildInfo.dstAccelerationStructure = m_tlas.accel;
  buildInfo.scratchData.deviceAddress = scratchAddr;

  VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{instanceCount, 0, 0, 0};
  const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;
  vkCmdBuildAccelerationStructuresKHR(cmd->m_commandBuffer, 1, &buildInfo, &pBuildOffsetInfo);
}
}// namespace dg

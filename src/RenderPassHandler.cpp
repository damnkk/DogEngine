#include "RenderPassHandler.h"

namespace dg{
RenderPassHandler::RenderPassHandler() {
  m_RenderPass = 0;
  m_OffScreenRenderPass = 0;
}

RenderPassHandler::RenderPassHandler(MainDevice *mainDevice,
                                     SwapChain *swaphcain) {
  m_MainDevice = mainDevice;
  m_SwapChainHandler = swaphcain;
}

void RenderPassHandler::CreateOffScreenRenderPass() {
  std::array<VkSubpassDescription, 1> subpasses = {};
  VkFormat image_format = m_SwapChainHandler->GetSwapChainImageFormat();

  VkAttachmentDescription input_position_attachment =
      InputPositionAttachment(image_format);
  VkAttachmentDescription input_color_attachment =
      InputColourAttachment(image_format);
  VkAttachmentDescription input_normal_attachment =
      InputPositionAttachment(image_format);
  VkAttachmentDescription input_depth_attachment = InputDepthAttachment();

  VkAttachmentReference position_attach_ref = {};
  position_attach_ref.attachment = 0;
  position_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_attach_ref = {};
  color_attach_ref.attachment = 1;
  color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference normal_attach_ref = {};
  normal_attach_ref.attachment = 2;
  normal_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  std::array<VkAttachmentReference, 3> attach_refs{
      position_attach_ref, color_attach_ref, normal_attach_ref};

  VkAttachmentReference depth_attach_ref = {};
  depth_attach_ref.attachment = 3;
  depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount = static_cast<uint32_t>(attach_refs.size());
  subpasses[0].pColorAttachments = attach_refs.data();
  subpasses[0].pDepthStencilAttachment = &depth_attach_ref;

  std::array<VkSubpassDependency, 2> subpass_dep = SetSubpassDependencies();

  std::array<VkAttachmentDescription, 4> renderPassAttachments = {
      input_position_attachment, input_color_attachment,
      input_normal_attachment, input_depth_attachment};

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount =
      static_cast<uint32_t>(renderPassAttachments.size());
  renderPassCreateInfo.pAttachments = renderPassAttachments.data();
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
  renderPassCreateInfo.pSubpasses = subpasses.data();
  renderPassCreateInfo.dependencyCount =
      static_cast<uint32_t>(subpass_dep.size());
  renderPassCreateInfo.pDependencies = subpass_dep.data();
  if (vkCreateRenderPass(m_MainDevice->logicalDevice, &renderPassCreateInfo,
                         nullptr, &m_OffScreenRenderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create offscreen render pass!");
  }
}

void RenderPassHandler::CreateRenderPass() {
  std::array<VkSubpassDescription, 1> subpasses = {};
  VkFormat image_format = m_SwapChainHandler->GetSwapChainImageFormat();

  VkAttachmentDescription input_depth_attachment = InputDepthAttachment();
  VkAttachmentDescription swapchain_color_attachment =
      SwapchainColourAttachment(image_format);

  VkAttachmentReference color_attach_ref = {};
  color_attach_ref.attachment = 0;
  color_attach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_attach_ref = {};
  depth_attach_ref.attachment = 1;
  depth_attach_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpasses[0].colorAttachmentCount = 1;
  subpasses[0].pColorAttachments = &color_attach_ref;
  subpasses[0].pDepthStencilAttachment = &depth_attach_ref;

  std::array<VkSubpassDependency, 2> subpass_dep = SetSubpassDependencies();

  std::array<VkAttachmentDescription, 2> renderPassAttachments = {
      swapchain_color_attachment, input_depth_attachment};

  VkRenderPassCreateInfo renderPassCreateInfo = {};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
  renderPassCreateInfo.pSubpasses = subpasses.data();
  renderPassCreateInfo.attachmentCount =
      static_cast<uint32_t>(renderPassAttachments.size());
  renderPassCreateInfo.pAttachments = renderPassAttachments.data();
  renderPassCreateInfo.dependencyCount =
      static_cast<uint32_t>(subpass_dep.size());
  renderPassCreateInfo.pDependencies = subpass_dep.data();

  if (vkCreateRenderPass(m_MainDevice->logicalDevice, &renderPassCreateInfo,
                         nullptr, &m_RenderPass) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swapchain renderPass!");
  }
}

VkAttachmentDescription
RenderPassHandler::SwapchainColourAttachment(const VkFormat &imageFormat) {
  VkAttachmentDescription colour_attachment = {};
  colour_attachment.format = imageFormat;
  colour_attachment.flags = 0;
  colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  return colour_attachment;
}

VkAttachmentDescription
RenderPassHandler::InputColourAttachment(const VkFormat &imageFormat) {
  VkAttachmentDescription color_attachment_input = {};
  color_attachment_input.format = Utility::findSupportedFormat(
      {VK_FORMAT_R32G32B32A32_SFLOAT}, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  color_attachment_input.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment_input.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment_input.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment_input.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment_input.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment_input.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment_input.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return color_attachment_input;
}

VkAttachmentDescription
RenderPassHandler::InputPositionAttachment(const VkFormat &imageFormat) {
  VkAttachmentDescription position_attachment_input = {};
  position_attachment_input.format = Utility::findSupportedFormat(
      {VK_FORMAT_R32G32B32A32_SFLOAT}, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  position_attachment_input.samples = VK_SAMPLE_COUNT_1_BIT;
  position_attachment_input.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  position_attachment_input.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  position_attachment_input.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  position_attachment_input.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  position_attachment_input.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  position_attachment_input.finalLayout =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  return position_attachment_input;
}

VkAttachmentDescription RenderPassHandler::InputDepthAttachment() {
  std::vector<VkFormat> formats = {VK_FORMAT_D32_SFLOAT,
                                   VK_FORMAT_D32_SFLOAT_S8_UINT,
                                   VK_FORMAT_D24_UNORM_S8_UINT};
  VkAttachmentDescription depth_attachment_input = {};
  depth_attachment_input.format = Utility::findSupportedFormat(
      formats, VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  depth_attachment_input.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment_input.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment_input.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment_input.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment_input.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment_input.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment_input.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  return depth_attachment_input;
}

std::array<VkSubpassDependency, 2> RenderPassHandler::SetSubpassDependencies() {
  std::array<VkSubpassDependency, 2> dependices = {};
  dependices[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependices[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependices[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependices[0].dstSubpass = 0;
  dependices[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependices[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependices[0].dependencyFlags = 0;

  dependices[1].srcSubpass = 0;
  dependices[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependices[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependices[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependices[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependices[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependices[1].dependencyFlags = 0;
  return dependices;
}

void RenderPassHandler::DestroyRenderPass() {
  vkDestroyRenderPass(m_MainDevice->logicalDevice, m_RenderPass, nullptr);
  vkDestroyRenderPass(m_MainDevice->logicalDevice, m_OffScreenRenderPass,
                      nullptr);
}

} //namespace dg
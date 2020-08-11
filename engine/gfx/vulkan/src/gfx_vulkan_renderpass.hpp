#pragma once

#include <vulkan/vulkan.h>

#include "gfx_renderpass.hpp"

class GFXVulkanRenderPass : public GFXRenderPass {
public:
	VkRenderPass handle = VK_NULL_HANDLE;

	unsigned int numAttachments = 0;
    int depth_attachment = -1;
	
	bool hasDepthAttachment = false;
};

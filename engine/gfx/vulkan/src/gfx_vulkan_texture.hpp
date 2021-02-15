#pragma once

#include <vulkan/vulkan.h>

#include "gfx_texture.hpp"

class GFXVulkanTexture : public GFXTexture {
public:
	VkImage handle;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;

	int width, height;

	VkFormat format;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageAspectFlagBits aspect;
    VkImageSubresourceRange range;
};

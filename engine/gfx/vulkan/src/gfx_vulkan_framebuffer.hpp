#pragma once

#include <vulkan/vulkan.h>

#include "gfx_framebuffer.hpp"

class GFXVulkanFramebuffer : public GFXFramebuffer {
public:
	VkFramebuffer handle;
    
    int width = 0, height = 0;
};

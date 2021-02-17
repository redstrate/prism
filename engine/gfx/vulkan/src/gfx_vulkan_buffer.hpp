#pragma once

#include <vulkan/vulkan.h>

#include "gfx_buffer.hpp"
#include "gfx_vulkan_constants.hpp"

class GFXVulkanBuffer : public GFXBuffer {
public:
    VkBuffer handle;
    VkDeviceMemory memory;
    
	VkDeviceSize size;
    uint32_t frame_index = 0;
};

#pragma once

#include <vulkan/vulkan.h>

#include "gfx_commandbuffer.hpp"

class GFXVulkanCommandBuffer : public GFXCommandBuffer {
public:
    VkCommandBuffer handle = VK_NULL_HANDLE;
};

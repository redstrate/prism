#pragma once

#include <vulkan/vulkan.h>

#include "gfx_buffer.hpp"
#include "gfx_vulkan_constants.hpp"

class GFXVulkanBuffer : public GFXBuffer {
public:
    bool is_dynamic_data = false;

    struct Data {
        VkBuffer handle;
        VkDeviceMemory memory;
    } data[MAX_FRAMES_IN_FLIGHT];

    Data get(int frameIndex) {
        if(is_dynamic_data) {
            return data[frameIndex];
        } else {
            return data[0];
        }
    }

	VkDeviceSize size;
};

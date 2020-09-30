#pragma once

#include <vulkan/vulkan.h>

#include "gfx_sampler.hpp"

class GFXVulkanSampler: public GFXSampler {
public:
	VkSampler sampler;
};

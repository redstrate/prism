#pragma once

#include <vulkan/vulkan.h>

#include "gfx_pipeline.hpp"

class GFXVulkanPipeline : public GFXPipeline {
public:
	VkPipeline handle;
	VkPipelineLayout layout;

	VkDescriptorSetLayout descriptorLayout;
    
    // dynamic descriptor sets
    std::map<uint64_t, VkDescriptorSet> cachedDescriptorSets;
};

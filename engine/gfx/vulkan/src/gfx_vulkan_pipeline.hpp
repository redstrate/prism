#pragma once

#include <vulkan/vulkan.h>

#include "gfx_pipeline.hpp"

class GFXVulkanTexture;

class GFXVulkanPipeline : public GFXPipeline {
public:
	std::string label;

	VkPipeline handle;
	VkPipelineLayout layout;

	VkDescriptorSetLayout descriptorLayout;
	
	std::vector<int> bindings_marked_as_storage_images;
	std::vector<int> bindings_marked_as_sampled_images;

    // dynamic descriptor sets
    std::map<uint64_t, VkDescriptorSet> cachedDescriptorSets;
};

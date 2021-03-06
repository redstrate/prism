#pragma once

#ifdef PLATFORM_WINDOWS
#define NOMINMAX // donut define max in windows.h
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>

#include <map>
#include <array>

#include "gfx.hpp"
#include "gfx_vulkan_constants.hpp"

struct NativeSurface {
    int identifier = -1;
    
    void* windowNativeHandle;
    uint32_t surfaceWidth = -1, surfaceHeight = -1;
    
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    size_t currentFrame = 0;
    
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D swapchainExtent = {};
    
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    
    VkRenderPass swapchainRenderPass = VK_NULL_HANDLE;
    
    std::vector<VkFramebuffer> swapchainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
};

class GFXVulkanPipeline;

class GFXVulkan : public GFX {
public:
    bool is_supported() { return true; }
	ShaderLanguage accepted_shader_language() override { return ShaderLanguage::SPIRV; }
	GFXContext required_context() { return GFXContext::Vulkan; }
	const char* get_name() override;

    bool supports_feature(const GFXFeature feature) override;

    bool initialize(const GFXCreateInfo& info) override;

	void initialize_view(void* native_handle, const int identifier, const uint32_t width, const uint32_t height) override;
	void recreate_view(const int identifier, const uint32_t width, const uint32_t height) override;

    // buffer operations
    GFXBuffer* create_buffer(void* data, const GFXSize size, const bool dynamic_data, const GFXBufferUsage usage) override;
    void copy_buffer(GFXBuffer* buffer, void* data, const GFXSize offset, const GFXSize size) override;

    void* get_buffer_contents(GFXBuffer* buffer) override;
    void release_buffer_contents(GFXBuffer* buffer, void* handle) override;

    // texture operations
    GFXTexture* create_texture(const GFXTextureCreateInfo& info) override;
    void copy_texture(GFXTexture* texture, void* data, const GFXSize size) override;
    void copy_texture(GFXTexture* from, GFXTexture* to) override;
    void copy_texture(GFXTexture* from, GFXBuffer* to) override;

	// sampler operations
	GFXSampler* create_sampler(const GFXSamplerCreateInfo& info) override;

    // framebuffer operations
    GFXFramebuffer* create_framebuffer(const GFXFramebufferCreateInfo& info) override;

	// render pass operations
	GFXRenderPass* create_render_pass(const GFXRenderPassCreateInfo& info) override;

    // pipeline operations
    GFXPipeline* create_graphics_pipeline(const GFXGraphicsPipelineCreateInfo& info) override;
    GFXPipeline* create_compute_pipeline(const GFXComputePipelineCreateInfo& info) override;

    // misc operations
	GFXSize get_alignment(const GFXSize size) override;

    GFXCommandBuffer* acquire_command_buffer(bool for_presentation_use = false) override;

    void submit(GFXCommandBuffer* command_buffer, const int identifier) override;

private:
	void createInstance(std::vector<const char*> layers, std::vector<const char*> extensions);
	void createLogicalDevice(std::vector<const char*> extensions);
	void createSwapchain(NativeSurface* native_surface, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void createDescriptorPool();
    void createSyncPrimitives(NativeSurface* native_surface);

	// dynamic descriptor sets
	void resetDescriptorState();
	void cacheDescriptorState(GFXVulkanPipeline* pipeline, VkDescriptorSetLayout layout);
    uint64_t getDescriptorHash(GFXVulkanPipeline* pipeline);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageSubresourceRange range, VkImageLayout oldLayout, VkImageLayout newLayout);
	void inlineTransitionImageLayout(VkCommandBuffer command_buffer,
                                  VkImage image,
                                  VkFormat format,
                                  VkImageAspectFlags aspect,
                                  VkImageSubresourceRange range,
                                  VkImageLayout oldLayout,
                                  VkImageLayout newLayout,
                                  VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	VkShaderModule createShaderModule(const uint32_t* code, const int length);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkInstance instance = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	VkCommandPool commandPool = VK_NULL_HANDLE;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    std::vector<NativeSurface*> native_surfaces;

	struct BoundShaderBuffer {
		GFXBuffer* buffer = nullptr;
		VkDeviceSize size = 0, offset = 0;
	};

	std::array<BoundShaderBuffer, 25> boundShaderBuffers;
	std::array<GFXTexture*, 25> boundTextures;
	std::array<GFXSampler*, 25> boundSamplers;
};

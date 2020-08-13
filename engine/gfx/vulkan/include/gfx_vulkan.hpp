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

class GFXVulkanPipeline;

class GFXVulkan : public GFX {
public:
    bool is_supported() { return true; }
	GFXContext required_context() { return GFXContext::Vulkan; }const char* get_name() override;

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

    // framebuffer operations
    GFXFramebuffer* create_framebuffer(const GFXFramebufferCreateInfo& info) override;

	// render pass operations
	GFXRenderPass* create_render_pass(const GFXRenderPassCreateInfo& info) override;

    // pipeline operations
    GFXPipeline* create_graphics_pipeline(const GFXGraphicsPipelineCreateInfo& info) override;

    // misc operations
	GFXSize get_alignment(const GFXSize size) override;

    void submit(GFXCommandBuffer* command_buffer, const int identifier) override;

private:
	void createInstance(std::vector<const char*> layers, std::vector<const char*> extensions);
	void createLogicalDevice(std::vector<const char*> extensions);
	void createSwapchain(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	void createDescriptorPool();
	void createSyncPrimitives();

	// dynamic descriptor sets
	void resetDescriptorState();
	void cacheDescriptorState(GFXVulkanPipeline* pipeline, VkDescriptorSetLayout layout);
    uint64_t getDescriptorHash(GFXVulkanPipeline* pipeline);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkShaderModule createShaderModule(const unsigned char* code, const int length);
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	VkInstance instance = VK_NULL_HANDLE;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;

	VkCommandPool commandPool = VK_NULL_HANDLE;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

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

	struct BoundShaderBuffer {
		GFXBuffer* buffer = nullptr;
		VkDeviceSize size = 0, offset = 0;
	};

	std::array<BoundShaderBuffer, 25> boundShaderBuffers;
	std::array<GFXTexture*, 25> boundTextures;
};

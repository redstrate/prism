#include "gfx_vulkan.hpp"

#include <vector>
#include <limits>
#include <cstddef>
#include <array>
#include <sstream>

#include "gfx_vulkan_buffer.hpp"
#include "gfx_vulkan_pipeline.hpp"
#include "gfx_commandbuffer.hpp"
#include "gfx_vulkan_texture.hpp"
#include "gfx_vulkan_framebuffer.hpp"
#include "gfx_vulkan_renderpass.hpp"
#include "gfx_vulkan_sampler.hpp"
#include "file.hpp"
#include "log.hpp"
#include "utility.hpp"

void* windowNativeHandle;

VkFormat toVkFormat(GFXPixelFormat format) {
	switch (format) {
    case GFXPixelFormat::R_32F:
        return VK_FORMAT_R32_SFLOAT;

	case GFXPixelFormat::R_16F:
		return VK_FORMAT_R16_SFLOAT;

	case GFXPixelFormat::RGBA_32F:
		return VK_FORMAT_R32G32B32A32_SFLOAT;

	case GFXPixelFormat::RGBA8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;

	case GFXPixelFormat::R8_UNORM:
		return VK_FORMAT_R8_UNORM;

	case GFXPixelFormat::R8G8_UNORM:
		return VK_FORMAT_R8G8_UNORM;

	case GFXPixelFormat::R8G8_SFLOAT:
		return VK_FORMAT_R16G16_SFLOAT;

    case GFXPixelFormat::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;

    case GFXPixelFormat::R16G16B16A16_SFLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;

	case GFXPixelFormat::DEPTH_32F:
		return VK_FORMAT_D32_SFLOAT;
    }

	return VK_FORMAT_UNDEFINED;
}

VkFormat toVkFormat(GFXVertexFormat format) {
	switch (format) {
	case GFXVertexFormat::FLOAT2:
		return VK_FORMAT_R32G32_SFLOAT;
	case GFXVertexFormat::FLOAT3:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case GFXVertexFormat::FLOAT4:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case GFXVertexFormat::INT:
		return VK_FORMAT_R8_SINT;
    case GFXVertexFormat::INT4:
        return VK_FORMAT_R32G32B32A32_SINT;
	case GFXVertexFormat::UNORM4:
		return VK_FORMAT_R8G8B8A8_UNORM;
	}

	return VK_FORMAT_UNDEFINED;
}

VkBlendFactor toVkFactor(GFXBlendFactor factor) {
	switch (factor) {
	case GFXBlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case GFXBlendFactor::OneMinusSrcAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case GFXBlendFactor::OneMinusSrcColor:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case GFXBlendFactor::SrcAlpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	}

	return VK_BLEND_FACTOR_ONE;
}

VkSamplerAddressMode toSamplerMode(SamplingMode mode) {
	switch (mode) {
	case SamplingMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case SamplingMode::ClampToBorder:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case SamplingMode::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	}
	
	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkFilter toFilter(GFXFilter filter) {
	switch (filter) {
	case GFXFilter::Nearest:
		return VK_FILTER_NEAREST;
	case GFXFilter::Linear:
		return VK_FILTER_LINEAR;
	}

	return VK_FILTER_LINEAR;
}

VkBorderColor toBorderColor(GFXBorderColor color) {
	switch (color) {
	case GFXBorderColor::OpaqueBlack:
		return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	case GFXBorderColor::OpaqueWhite:
		return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	}

	return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
}

VkCompareOp toCompareFunc(GFXCompareFunction func) {
	switch (func) {
	case GFXCompareFunction::Never:
		return VK_COMPARE_OP_NEVER;
		break;
	case GFXCompareFunction::Less:
		return VK_COMPARE_OP_LESS;
		break;
	case GFXCompareFunction::Equal:
		return VK_COMPARE_OP_EQUAL;
		break;
	case GFXCompareFunction::LessOrEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
		break;
	case GFXCompareFunction::Greater:
		return VK_COMPARE_OP_GREATER;
		break;
	case GFXCompareFunction::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
		break;
	case GFXCompareFunction::GreaterOrEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
		break;
	case GFXCompareFunction::Always:
		return VK_COMPARE_OP_ALWAYS;
		break;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

	console::debug(System::GFX, pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult name_object(VkDevice device, VkObjectType type, uint64_t object, std::string_view name) {
    VkDebugUtilsObjectNameInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.objectType = type;
    info.pObjectName = name.data();
    info.objectHandle = object;

    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    if (func != nullptr)
        return func(device, &info);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

bool GFXVulkan::initialize(const GFXCreateInfo& info) {
#ifdef PLATFORM_WINDOWS
    const char* surface_name = "VK_KHR_win32_surface";
#else
    const char* surface_name = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#endif

	uint32_t extensionPropertyCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, nullptr);

	std::vector<VkExtensionProperties> extensionProperties(extensionPropertyCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, extensionProperties.data());

	std::vector<const char*> enabledExtensions = { "VK_KHR_surface", surface_name };
	for (auto prop : extensionProperties) {
		if (!strcmp(prop.extensionName, "VK_EXT_debug_utils"))
			enabledExtensions.push_back("VK_EXT_debug_utils");
	}

	createInstance({}, enabledExtensions);
	createLogicalDevice({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });
	createDescriptorPool();
	createSyncPrimitives();

    return true;
}

void GFXVulkan::initialize_view(void* native_handle, const int identifier, const uint32_t width, const uint32_t height) {
	vkDeviceWaitIdle(device);

	surfaceWidth = width;
	surfaceHeight = height;

	windowNativeHandle = native_handle;

	createSwapchain();
}

void GFXVulkan::recreate_view(const int identifier, const uint32_t width, const uint32_t height) {
	vkDeviceWaitIdle(device);

	surfaceWidth = width;
	surfaceHeight = height;

	createSwapchain(swapchain);
}

GFXBuffer* GFXVulkan::create_buffer(void *data, const GFXSize size, const bool dynamic_data, const GFXBufferUsage usage) {
	GFXVulkanBuffer* buffer = new GFXVulkanBuffer();
    buffer->is_dynamic_data = dynamic_data;

	vkDeviceWaitIdle(device);

	// choose buffer features
	VkBufferUsageFlags bufferUsage = 0;
	if (usage == GFXBufferUsage::Storage)
		bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	else if (usage == GFXBufferUsage::Vertex)
		bufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	else
		bufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

	// create buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(dynamic_data) {
        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
            vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->data[i].handle);
    } else {
        vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->data[0].handle);
    }

	buffer->size = size;

	// allocate memory
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer->data[0].handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if(dynamic_data) {
        for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkAllocateMemory(device, &allocInfo, nullptr, &buffer->data[i].memory);

            vkBindBufferMemory(device, buffer->data[i].handle, buffer->data[i].memory, 0);
        }
    } else {
        vkAllocateMemory(device, &allocInfo, nullptr, &buffer->data[0].memory);

        vkBindBufferMemory(device, buffer->data[0].handle, buffer->data[0].memory, 0);
    }

	if (data != nullptr) {
        if(dynamic_data) {
            for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                void* mapped_data;
                vkMapMemory(device, buffer->data[i].memory, 0, size, 0, &mapped_data);
                memcpy(mapped_data, data, size);
                vkUnmapMemory(device, buffer->data[i].memory);
            }
        } else {
            void* mapped_data;
            vkMapMemory(device, buffer->data[0].memory, 0, size, 0, &mapped_data);
            memcpy(mapped_data, data, size);
            vkUnmapMemory(device, buffer->data[0].memory);
        }
    }

	return buffer;
}

void GFXVulkan::copy_buffer(GFXBuffer* buffer, void* data, GFXSize offset, GFXSize size) {
	GFXVulkanBuffer* vulkanBuffer = (GFXVulkanBuffer*)buffer;

	void* mapped_data;
	vkMapMemory(device, vulkanBuffer->get(currentFrame).memory, offset, vulkanBuffer->size - offset, 0, &mapped_data);
	memcpy(mapped_data, data, size);
    vkUnmapMemory(device, vulkanBuffer->get(currentFrame).memory);
}

void* GFXVulkan::get_buffer_contents(GFXBuffer* buffer) {
    GFXVulkanBuffer* vulkanBuffer = (GFXVulkanBuffer*)buffer;

    void* mapped_data;
    vkMapMemory(device, vulkanBuffer->get(currentFrame).memory, 0, vulkanBuffer->size, 0, &mapped_data);

    return mapped_data;
}

void GFXVulkan::release_buffer_contents(GFXBuffer* buffer, void* handle) {
    GFXVulkanBuffer* vulkanBuffer = (GFXVulkanBuffer*)buffer;
    vkUnmapMemory(device, vulkanBuffer->get(currentFrame).memory);
}

GFXTexture* GFXVulkan::create_texture(const GFXTextureCreateInfo& info) {
	GFXVulkanTexture* texture = new GFXVulkanTexture();

	vkDeviceWaitIdle(device);

	// choose image features
	VkFormat imageFormat = toVkFormat(info.format);

	VkImageTiling imageTiling;
	imageTiling = VK_IMAGE_TILING_OPTIMAL;

	VkImageUsageFlags imageUsage = 0;
    if ((info.usage & GFXTextureUsage::Attachment) == GFXTextureUsage::Attachment) {
        if (info.format == GFXPixelFormat::DEPTH_32F) {
            imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		else {
			imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		}
	}
	else {
		imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	VkImageAspectFlagBits imageAspect;
	if (info.format == GFXPixelFormat::DEPTH_32F)
		imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	else
		imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;

	int array_length = info.array_length;
	if (info.type == GFXTextureType::Cubemap)
		array_length = 6;
	else if (info.type == GFXTextureType::CubemapArray)
		array_length *= 6;

	// create image
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = info.width;
	imageInfo.extent.height = info.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = info.mip_count;
	imageInfo.arrayLayers = array_length;
	imageInfo.format = imageFormat;
	imageInfo.tiling = imageTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = imageUsage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	if (info.type == GFXTextureType::Cubemap || info.type == GFXTextureType::CubemapArray)
		imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	vkCreateImage(device, &imageInfo, nullptr, &texture->handle);

	name_object(device, VK_OBJECT_TYPE_IMAGE, (uint64_t)texture->handle, info.label);

	texture->width = info.width;
    texture->height = info.height;
	texture->format = imageFormat;
	texture->aspect = imageAspect;
	texture->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// allocate memory
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, texture->handle, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkAllocateMemory(device, &allocInfo, nullptr, &texture->memory);

	vkBindImageMemory(device, texture->handle, texture->memory, 0);

	transitionImageLayout(texture->handle, imageFormat, imageAspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// create image view
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = texture->handle;
	
	switch (info.type) {
	case GFXTextureType::Single2D:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		break;
	case GFXTextureType::Array2D:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		break;
	case GFXTextureType::Cubemap:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		break;
	case GFXTextureType::CubemapArray:
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		break;
	}
	viewInfo.format = imageFormat;
	viewInfo.subresourceRange.aspectMask = imageAspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = info.mip_count;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = array_length;

	vkCreateImageView(device, &viewInfo, nullptr, &texture->view);

	const VkSamplerAddressMode samplerMode = toSamplerMode(info.samplingMode);

	// create sampler
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = samplerMode;
    samplerInfo.addressModeV = samplerMode;
    samplerInfo.addressModeW = samplerMode;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.compareEnable = info.compare_enabled;
	samplerInfo.borderColor = toBorderColor(info.border_color);
	samplerInfo.compareOp = toCompareFunc(info.compare_function);
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler);

	return texture;
}

void GFXVulkan::copy_texture(GFXTexture* texture, void* data, GFXSize size) {
	GFXVulkanTexture* vulkanTexture = (GFXVulkanTexture*)texture;

	vkDeviceWaitIdle(device);

	// create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

	// allocate staging memory
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory);

	vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

	// copy to staging buffer
	void* mapped_data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped_data);
	memcpy(mapped_data, data, size);
	vkUnmapMemory(device, stagingBufferMemory);

	// copy staging buffer to image
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	inlineTransitionImageLayout(commandBuffer, vulkanTexture->handle, vulkanTexture->format, vulkanTexture->aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = vulkanTexture->aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {
		(uint32_t)vulkanTexture->width,
		(uint32_t)vulkanTexture->height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vulkanTexture->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	inlineTransitionImageLayout(commandBuffer, vulkanTexture->handle, vulkanTexture->format, vulkanTexture->aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	endSingleTimeCommands(commandBuffer);
}

void GFXVulkan::copy_texture(GFXTexture* from, GFXTexture* to) {
    console::error(System::GFX, "Copy Texture->Texture unimplemented!");
}

void GFXVulkan::copy_texture(GFXTexture* from, GFXBuffer* to) {
	GFXVulkanTexture* vulkanTexture = (GFXVulkanTexture*)from;
	GFXVulkanBuffer* vulkanBuffer = (GFXVulkanBuffer*)to;

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = vulkanTexture->aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {
		(uint32_t)vulkanTexture->width,
		(uint32_t)vulkanTexture->height,
		1
	};

	vkCmdCopyImageToBuffer(commandBuffer, vulkanTexture->handle, vulkanTexture->layout, vulkanBuffer->get(0).handle, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

GFXSampler* GFXVulkan::create_sampler(const GFXSamplerCreateInfo& info) {
	GFXVulkanSampler* sampler = new GFXVulkanSampler();

	const VkSamplerAddressMode samplerMode = toSamplerMode(info.samplingMode);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = toFilter(info.mag_filter);
	samplerInfo.minFilter = toFilter(info.min_filter);
	samplerInfo.addressModeU = samplerMode;
	samplerInfo.addressModeV = samplerMode;
	samplerInfo.addressModeW = samplerMode;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = toBorderColor(info.border_color);
	samplerInfo.compareEnable = info.compare_enabled;
	samplerInfo.compareOp = toCompareFunc(info.compare_function);
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	vkCreateSampler(device, &samplerInfo, nullptr, &sampler->sampler);

	return sampler;
}

GFXFramebuffer* GFXVulkan::create_framebuffer(const GFXFramebufferCreateInfo& info) {
	GFXVulkanFramebuffer* framebuffer = new GFXVulkanFramebuffer();

	vkDeviceWaitIdle(device);

	GFXVulkanRenderPass* renderPass = (GFXVulkanRenderPass*)info.render_pass;

	std::vector<VkImageView> attachments;
	for (auto& attachment : info.attachments) {
		GFXVulkanTexture* texture = (GFXVulkanTexture*)attachment;
		attachments.push_back(texture->view);

		VkImageLayout expectedLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		if (texture->aspect & VK_IMAGE_ASPECT_DEPTH_BIT)
			expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		transitionImageLayout(texture->handle, texture->format, texture->aspect, texture->layout, expectedLayout);
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass->handle;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = ((GFXVulkanTexture*)info.attachments[0])->width; // FIXME: eww!!
    framebufferInfo.height = ((GFXVulkanTexture*)info.attachments[0])->height;
	framebufferInfo.layers = 1;

	vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer->handle);

	name_object(device, VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)framebuffer->handle, info.label);

    framebuffer->width = ((GFXVulkanTexture*)info.attachments[0])->width;
    framebuffer->height = ((GFXVulkanTexture*)info.attachments[0])->height;

	return framebuffer;
}

GFXRenderPass* GFXVulkan::create_render_pass(const GFXRenderPassCreateInfo& info) {
	GFXVulkanRenderPass* renderPass = new GFXVulkanRenderPass();

	vkDeviceWaitIdle(device);

	std::vector<VkAttachmentDescription> descriptions;
	std::vector<VkAttachmentReference> references;

	bool hasDepthAttachment = false;
	VkAttachmentDescription depthAttachment;
	VkAttachmentReference depthAttachmentRef;

	for (int i = 0; i < info.attachments.size(); i++) {
		bool isDepthAttachment = false;
		if (info.attachments[i] == GFXPixelFormat::DEPTH_32F)
			isDepthAttachment = true;

		VkAttachmentDescription attachment = {};
		attachment.format = toVkFormat(info.attachments[i]);
		attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (isDepthAttachment)
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		else
			attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference attachmentRef = {};
		attachmentRef.attachment = i;

		if (isDepthAttachment)
			attachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		else
			attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		if (isDepthAttachment) {
			hasDepthAttachment = true;
			depthAttachment = attachment;
			depthAttachmentRef = attachmentRef;
            renderPass->depth_attachment = i;
		}
		else {
			descriptions.push_back(attachment);
			references.push_back(attachmentRef);
		}
	}

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = static_cast<uint32_t>(references.size());
	subpass.pColorAttachments = references.data();

	if(hasDepthAttachment) {
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		descriptions.push_back(depthAttachment);
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
	renderPassInfo.pAttachments = descriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass->handle);

	name_object(device, VK_OBJECT_TYPE_RENDER_PASS, (uint64_t)renderPass->handle, info.label);

	renderPass->numAttachments = static_cast<unsigned int>(descriptions.size());
	renderPass->hasDepthAttachment = hasDepthAttachment;

	return renderPass;
}

GFXPipeline* GFXVulkan::create_graphics_pipeline(const GFXGraphicsPipelineCreateInfo& info) {
	GFXVulkanPipeline* pipeline = new GFXVulkanPipeline();

	vkDeviceWaitIdle(device);

	VkShaderModule vertex_module = VK_NULL_HANDLE, fragment_module = VK_NULL_HANDLE;

	const bool has_vertex_stage = !info.shaders.vertex_path.empty() || !info.shaders.vertex_src.empty();
	const bool has_fragment_stage = !info.shaders.fragment_path.empty() || !info.shaders.fragment_src.empty();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	if (has_vertex_stage) {
		const bool vertex_use_shader_source = info.shaders.vertex_path.empty();

		if (vertex_use_shader_source) {
			auto& vertex_shader_vector = info.shaders.vertex_src.as_bytecode();

			vertex_module = createShaderModule(vertex_shader_vector.data(), vertex_shader_vector.size() * sizeof(uint32_t));
		}
		else {
			auto vertex_shader = file::open(file::internal_domain / (std::string(info.shaders.vertex_path) + ".spv"), true);
			vertex_shader->read_all();

			vertex_module = createShaderModule(vertex_shader->cast_data<uint32_t>(), vertex_shader->size());
		}

		name_object(device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vertex_module, info.shaders.vertex_path);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertex_module;
		vertShaderStageInfo.pName = "main";

		shaderStages.push_back(vertShaderStageInfo);
	}

	if (has_fragment_stage) {
		const bool fragment_use_shader_source = info.shaders.fragment_path.empty();

		if (fragment_use_shader_source) {
			auto& fragment_shader_vector = info.shaders.fragment_src.as_bytecode();

			fragment_module = createShaderModule(fragment_shader_vector.data(), fragment_shader_vector.size() * sizeof(uint32_t));
		}
		else {
			auto fragment_shader = file::open(file::internal_domain / (std::string(info.shaders.fragment_path) + ".spv"), true);
			fragment_shader->read_all();

			fragment_module = createShaderModule(fragment_shader->cast_data<uint32_t>(), fragment_shader->size());
		}

		name_object(device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)fragment_module, info.shaders.fragment_path);

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragment_module;
		fragShaderStageInfo.pName = "main";

		shaderStages.push_back(fragShaderStageInfo);
	}

	// setup vertex inputs/bindings
	std::vector<VkVertexInputBindingDescription> inputs;
	for (auto& binding : info.vertex_input.inputs) {
		VkVertexInputBindingDescription b;
		b.binding = binding.location;
		b.stride = binding.stride;
		b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		inputs.push_back(b);
	}

	std::vector<VkVertexInputAttributeDescription> attributes;
	for (auto& attribute : info.vertex_input.attributes) {
		VkVertexInputAttributeDescription description;
		description.binding = attribute.binding;
		description.format = toVkFormat(attribute.format);
		description.location = attribute.location;
		description.offset = attribute.offset;

		attributes.push_back(description);
	}

	// fixed functions
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pVertexBindingDescriptions = inputs.data();
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(inputs.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	if (info.rasterization.primitive_type == GFXPrimitiveType::TriangleStrip)
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	switch (info.rasterization.culling_mode) {
	case GFXCullingMode::Backface:
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		break;
	case GFXCullingMode::Frontface:
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
		break;
	case GFXCullingMode::None:
		rasterizer.cullMode = VK_CULL_MODE_NONE;
	}

	switch (info.rasterization.winding_mode) {
	case GFXWindingMode::Clockwise:
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		break;
	case GFXWindingMode::CounterClockwise:
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		break;
	}

	if (info.rasterization.polygon_type == GFXPolygonType::Line)
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;


	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	if (info.blending.enable_blending) {
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = toVkFactor(info.blending.src_rgb);
		colorBlendAttachment.dstColorBlendFactor = toVkFactor(info.blending.dst_rgb);
		colorBlendAttachment.srcAlphaBlendFactor = toVkFactor(info.blending.src_alpha);
		colorBlendAttachment.dstAlphaBlendFactor = toVkFactor(info.blending.dst_alpha);
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	if (info.depth.depth_mode != GFXDepthMode::None) {
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;

		switch (info.depth.depth_mode) {
		case GFXDepthMode::Less:
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			break;
		case GFXDepthMode::LessOrEqual:
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			break;
		case GFXDepthMode::Greater:
			depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
			break;
		}
	}

	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// create push constants
	std::vector<VkPushConstantRange> pushConstants;
	for (auto& pushConstant : info.shader_input.push_constants) {
		VkPushConstantRange range;
		range.offset = pushConstant.offset;
        range.size = pushConstant.size;
		range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		pushConstants.push_back(range);
	}

	// create descriptor layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	for (auto& binding : info.shader_input.bindings) {
		// ignore push constants
		if (binding.type == GFXBindingType::PushConstant)
			continue;

		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		switch (binding.type) {
		case GFXBindingType::StorageBuffer:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		case GFXBindingType::Texture:
			descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case GFXBindingType::StorageImage:
		{
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			pipeline->bindings_marked_as_storage_images.push_back(binding.binding);
		}
			break;
		case GFXBindingType::SampledImage:
		{
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			pipeline->bindings_marked_as_sampled_images.push_back(binding.binding);
		}
		break;
		case GFXBindingType::Sampler:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		}

		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.binding = binding.binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = 1;
		layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		layoutBindings.push_back(layoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
	layoutCreateInfo.pBindings = layoutBindings.data();

	vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &pipeline->descriptorLayout);

	// create layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
	pipelineLayoutInfo.pSetLayouts = &pipeline->descriptorLayout;
	pipelineLayoutInfo.setLayoutCount = 1;

	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline->layout);

	// create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipeline->layout;

	if (info.render_pass != nullptr) {
		pipelineInfo.renderPass = ((GFXVulkanRenderPass*)info.render_pass)->handle;
	}
	else {
		pipelineInfo.renderPass = swapchainRenderPass;
	}

	if (info.render_pass != nullptr && ((GFXVulkanRenderPass*)info.render_pass)->hasDepthAttachment)
		pipelineInfo.pDepthStencilState = &depthStencil;

	vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->handle);

	if (info.label.empty())
		pipeline->label = std::string(info.shaders.vertex_path) + std::string(info.shaders.fragment_path);
	else
		pipeline->label = info.label;

	name_object(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)pipeline->handle, pipeline->label);
	name_object(device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)pipeline->layout, pipeline->label);

	return pipeline;
}

GFXSize GFXVulkan::get_alignment(GFXSize size) {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	VkDeviceSize minUboAlignment = properties.limits.minStorageBufferOffsetAlignment;

    return (size + minUboAlignment / 2) & ~int(minUboAlignment - 1);
}

GFXCommandBuffer* GFXVulkan::acquire_command_buffer() {
	return new GFXCommandBuffer();
}

void GFXVulkan::submit(GFXCommandBuffer* command_buffer, const int identifier) {
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return;

	VkCommandBuffer& cmd = commandBuffers[currentFrame];

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(cmd, &beginInfo);

	VkRenderPass currentRenderPass = VK_NULL_HANDLE;
	GFXVulkanPipeline* currentPipeline = nullptr;
	uint64_t lastDescriptorHash = 0;
	VkIndexType currentIndexType = VK_INDEX_TYPE_UINT32;

	const auto try_bind_descriptor = [cmd, this, &currentPipeline, &lastDescriptorHash]() -> bool {
		if (lastDescriptorHash != getDescriptorHash(currentPipeline)) {
			if (!currentPipeline->cachedDescriptorSets.count(getDescriptorHash(currentPipeline)))
				cacheDescriptorState(currentPipeline, currentPipeline->descriptorLayout);

			auto& descriptor_set = currentPipeline->cachedDescriptorSets[getDescriptorHash(currentPipeline)];
			if (descriptor_set == VK_NULL_HANDLE)
				return false;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline->layout, 0, 1, &descriptor_set, 0, nullptr);

			lastDescriptorHash = getDescriptorHash(currentPipeline);
		}

		return true;
	};

	for (auto command : command_buffer->commands) {
		switch (command.type) {
            case GFXCommandType::SetRenderPass:
		{
			// end the previous render pass
			if (currentRenderPass != VK_NULL_HANDLE) {
				vkCmdEndRenderPass(cmd);
			}

			GFXVulkanRenderPass* renderPass = (GFXVulkanRenderPass*)command.data.set_render_pass.render_pass;
			GFXVulkanFramebuffer* framebuffer = (GFXVulkanFramebuffer*)command.data.set_render_pass.framebuffer;

			if (renderPass != nullptr) {
				currentRenderPass = renderPass->handle;
			}
			else {
				currentRenderPass = swapchainRenderPass;
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = currentRenderPass;

			if (framebuffer != nullptr) {
				renderPassInfo.framebuffer = framebuffer->handle;

                VkViewport viewport = {};
				viewport.y = static_cast<float>(framebuffer->height);
                viewport.width = static_cast<float>(framebuffer->width);
                viewport.height = -static_cast<float>(framebuffer->height);
                viewport.maxDepth = 1.0f;

                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.extent.width = framebuffer->width;
                scissor.extent.height = framebuffer->height;

                vkCmdSetScissor(cmd, 0, 1, &scissor);
			}
			else {
				renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];

                VkViewport viewport = {};
				viewport.y = static_cast<float>(surfaceHeight);
                viewport.width = static_cast<float>(surfaceWidth);
                viewport.height = -static_cast<float>(surfaceHeight);
                viewport.maxDepth = 1.0f;

                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.extent.width = surfaceWidth;
                scissor.extent.height = surfaceHeight;

                vkCmdSetScissor(cmd, 0, 1, &scissor);
			}

			renderPassInfo.renderArea.offset = { command.data.set_render_pass.render_area.offset.x, command.data.set_render_pass.render_area.offset.y };
            renderPassInfo.renderArea.extent = { command.data.set_render_pass.render_area.extent.width, command.data.set_render_pass.render_area.extent.height };

			std::vector<VkClearValue> clearColors;
			if (renderPass != nullptr) {
				clearColors.resize(renderPass->numAttachments);
			}
			else {
				clearColors.resize(1);
			}

			clearColors[0].color.float32[0] = command.data.set_render_pass.clear_color.r;
			clearColors[0].color.float32[1] = command.data.set_render_pass.clear_color.g;
			clearColors[0].color.float32[2] = command.data.set_render_pass.clear_color.b;
			clearColors[0].color.float32[3] = command.data.set_render_pass.clear_color.a;

            if(renderPass != nullptr) {
                if(renderPass->depth_attachment != -1)
                    clearColors[renderPass->depth_attachment].depthStencil.depth = 1.0f;
            }

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
			renderPassInfo.pClearValues = clearColors.data();

			vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			currentPipeline = nullptr;
		}
		break;
		case GFXCommandType::SetGraphicsPipeline:
		{
			currentPipeline = (GFXVulkanPipeline*)command.data.set_graphics_pipeline.pipeline;
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline->handle);

			resetDescriptorState();
			lastDescriptorHash = 0;
		}
		break;
		case GFXCommandType::SetVertexBuffer:
		{
            VkBuffer buffer = ((GFXVulkanBuffer*)command.data.set_vertex_buffer.buffer)->get(currentFrame).handle;
			VkDeviceSize offset = command.data.set_vertex_buffer.offset;
			vkCmdBindVertexBuffers(cmd, command.data.set_vertex_buffer.index, 1, &buffer, &offset);
		}
			break;
		case GFXCommandType::SetIndexBuffer:
		{
			VkIndexType indexType = VK_INDEX_TYPE_UINT32;
			if (command.data.set_index_buffer.index_type == IndexType::UINT16)
				indexType = VK_INDEX_TYPE_UINT16;

            vkCmdBindIndexBuffer(cmd, ((GFXVulkanBuffer*)command.data.set_index_buffer.buffer)->get(currentFrame).handle, 0, indexType);

			currentIndexType = indexType;
		}
			break;
        case GFXCommandType::SetPushConstant:
		{
			if(currentPipeline != nullptr)
				vkCmdPushConstants(cmd, currentPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, command.data.set_push_constant.size, command.data.set_push_constant.bytes.data());
		}
			break;
        case GFXCommandType::BindShaderBuffer:
		{
			BoundShaderBuffer bsb;
			bsb.buffer = command.data.bind_shader_buffer.buffer;
			bsb.offset = command.data.bind_shader_buffer.offset;
			bsb.size = command.data.bind_shader_buffer.size;

			boundShaderBuffers[command.data.bind_shader_buffer.index] = bsb;
		}
			break;
		case GFXCommandType::BindTexture:
		{
			boundTextures[command.data.bind_texture.index] = command.data.bind_texture.texture;
		}
		break;
		case GFXCommandType::BindSampler:
		{
			boundSamplers[command.data.bind_sampler.index] = command.data.bind_sampler.sampler;
		}
		break;
		case GFXCommandType::Draw:
		{
			if(try_bind_descriptor())
				vkCmdDraw(cmd, command.data.draw.vertex_count, command.data.draw.instance_count, command.data.draw.vertex_offset, command.data.draw.base_instance);
		}
			break;
		case GFXCommandType::DrawIndexed:
		{
			if(try_bind_descriptor())
				vkCmdDrawIndexed(cmd, command.data.draw_indexed.index_count, 1, command.data.draw_indexed.first_index, command.data.draw_indexed.vertex_offset, 0);
		}
		break;
		case GFXCommandType::SetDepthBias:
		{
			vkCmdSetDepthBias(cmd, command.data.set_depth_bias.constant, command.data.set_depth_bias.clamp, command.data.set_depth_bias.slope_factor);
		}
		break;
		}
	}

	// end the last render pass
	if (currentRenderPass != VK_NULL_HANDLE) {
		vkCmdEndRenderPass(cmd);
	}

	vkEndCommandBuffer(cmd);

	// submit
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		return;

	// present
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(presentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

const char* GFXVulkan::get_name() {
    return "Vulkan";
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pCallback) {

    // Note: It seems that static_cast<...> doesn't work. Use the C-style forced cast.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void GFXVulkan::createInstance(std::vector<const char*> layers, std::vector<const char*> extensions) {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ;
    debugCreateInfo.pfnUserCallback = DebugCallback;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Prism Engine App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Prism Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	VkInstanceCreateInfo createInfo = {};
    createInfo.pNext = &debugCreateInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.ppEnabledLayerNames = layers.data();
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

	vkCreateInstance(&createInfo, nullptr, &instance);

    VkDebugUtilsMessengerEXT callback;
    CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &callback);
}

void GFXVulkan::createLogicalDevice(std::vector<const char*> extensions) {
	// pick physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	physicalDevice = devices[0];

	uint32_t graphicsFamilyIndex = 0, presentFamilyIndex = 0;

	// create logical device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamilyIndex = i;
		}

		i++;
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	if (graphicsFamilyIndex == presentFamilyIndex) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
		queueCreateInfo.queueCount = 1;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

	VkPhysicalDeviceFeatures enabledFeatures = {};
	//enabledFeatures.vertexPipelineStoresAndAtomics = true;
	enabledFeatures.fragmentStoresAndAtomics = true;
	enabledFeatures.samplerAnisotropy = true;
	enabledFeatures.fillModeNonSolid = true;
	enabledFeatures.imageCubeArray = true;

	createInfo.pEnabledFeatures = &enabledFeatures;

	vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);

	// get queues
	vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);
	vkGetDeviceQueue(device, presentFamilyIndex, 0, &presentQueue);

	// command pool
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamilyIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
}

void GFXVulkan::createSwapchain(VkSwapchainKHR oldSwapchain) {

#ifdef PLATFORM_WINDOWS
	// create win32 surface
	if(surface == VK_NULL_HANDLE)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = (HWND)windowNativeHandle;
		createInfo.hinstance = GetModuleHandle(nullptr);

		vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
	}
#else
    if(surface == VK_NULL_HANDLE) {
        struct WindowConnection {
            xcb_connection_t* connection;
            xcb_window_t window;
        };

        WindowConnection* wincon = reinterpret_cast<WindowConnection*>(windowNativeHandle);

        VkXcbSurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.window = wincon->window;
        createInfo.connection = wincon->connection;

        vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &surface);
    }
#endif

	// TODO: fix this pls
	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface, &supported);

	// query swapchain support
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

	std::vector<VkSurfaceFormatKHR> formats;

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());

	std::vector<VkPresentModeKHR> presentModes;
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	// choosing swapchain features
	VkSurfaceFormatKHR swapchainSurfaceFormat = formats[0];
	for (const auto& availableFormat : formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			swapchainSurfaceFormat = availableFormat;
		}
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& availablePresentMode : presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainPresentMode = availablePresentMode;
		}
	}

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	// create swapchain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = swapchainSurfaceFormat.format;
	createInfo.imageColorSpace = swapchainSurfaceFormat.colorSpace;
	createInfo.imageExtent.width = surfaceWidth;
	createInfo.imageExtent.height = surfaceHeight;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = swapchainPresentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = oldSwapchain;

	vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain);

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}

	swapchainExtent.width = surfaceWidth;
	swapchainExtent.height = surfaceHeight;

	// get swapchain images
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

	// create swapchain image views
	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainSurfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]);
	}

	// create render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapchainSurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	vkCreateRenderPass(device, &renderPassInfo, nullptr, &swapchainRenderPass);

	// create swapchain framebuffers
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = swapchainRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = surfaceWidth;
		framebufferInfo.height = surfaceHeight;
		framebufferInfo.layers = 1;

		vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]);
	}

	// allocate command buffers
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());

	for (auto [i, cmdbuf] : utility::enumerate(commandBuffers))
		name_object(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdbuf, ("main cmd buf " + std::to_string(i)).c_str());
}

void GFXVulkan::createDescriptorPool() {
	const std::array<VkDescriptorPoolSize, 2> poolSizes = { {
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}
	}};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 100;

	vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
}

void GFXVulkan::createSyncPrimitives() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
		vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]);
	}
}

void GFXVulkan::resetDescriptorState() {
	for (auto& buffer : boundShaderBuffers)
		buffer.buffer = nullptr;

	for (auto& texture : boundTextures)
		texture = nullptr;

	for (auto& sampler : boundSamplers)
		sampler = nullptr;
}

void GFXVulkan::cacheDescriptorState(GFXVulkanPipeline* pipeline, VkDescriptorSetLayout layout) {
	uint64_t hash = getDescriptorHash(pipeline);

	vkDeviceWaitIdle(device);

	// create set object
	VkDescriptorSet descriptorSet;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);

	name_object(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)descriptorSet, pipeline->label);

	if (descriptorSet == VK_NULL_HANDLE)
		return;

	// update set
	int i = 0;
	for (auto& buffer : boundShaderBuffers) {
		if (buffer.buffer != nullptr) {
			GFXVulkanBuffer* vulkanBuffer = (GFXVulkanBuffer*)buffer.buffer;

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = vulkanBuffer->get(currentFrame).handle; // will this break?
			bufferInfo.offset = buffer.offset;
			bufferInfo.range = buffer.size;

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSet;
			descriptorWrite.dstBinding = i;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}

		i++;
	}

	i = 0;
	for (auto& texture : boundTextures) {
		if (texture == nullptr) {
			i++;
			continue;
		}

		GFXVulkanTexture* vulkanTexture = (GFXVulkanTexture*)texture;

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = vulkanTexture->layout;

		// color attachments are not the right layout
		if (imageInfo.imageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageInfo.imageView = vulkanTexture->view;
		imageInfo.sampler = vulkanTexture->sampler;

		//if (imageInfo.imageLayout != vulkanTexture->layout) {
			GFXVulkanPipeline::ExpectedTransisition trans;
			trans.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			trans.newLayout = imageInfo.imageLayout;

			pipeline->expectedTransisitions[vulkanTexture] = trans;
		//}

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = i;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		if (utility::contains(pipeline->bindings_marked_as_storage_images, i)) {
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		} else if (utility::contains(pipeline->bindings_marked_as_sampled_images, i)) {
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		} else {
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

		i++;
	}

	i = 0;
	for (auto& sampler : boundSamplers) {
		if (sampler == nullptr) {
			i++;
			continue;
		}

		GFXVulkanSampler* vulkanSampler = (GFXVulkanSampler*)sampler;

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = vulkanSampler->sampler;

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = i;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

		i++;
	}

	pipeline->cachedDescriptorSets[hash] = descriptorSet;
}

uint64_t GFXVulkan::getDescriptorHash(GFXVulkanPipeline* pipeline) {
	uint64_t hash = 0;
    hash += (int64_t)pipeline;

	int i = 0;
	for (auto& buffer : boundShaderBuffers) {
		if (buffer.buffer != nullptr) {
			hash += (uint64_t)buffer.buffer * (i + 1);
		}
	}

	i = 0;
	for (auto& texture : boundTextures) {
		if (texture != nullptr) {
			hash += (uint64_t)texture * (i + 1);
        }
	}

	return hash;
}

uint32_t GFXVulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	return -1;
}

void GFXVulkan::transitionImageLayout(VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	inlineTransitionImageLayout(commandBuffer, image, format, aspect, oldLayout, newLayout);

	endSingleTimeCommands(commandBuffer);
}

void GFXVulkan::inlineTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

VkShaderModule GFXVulkan::createShaderModule(const uint32_t* code, const int length) {
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = length;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

	VkShaderModule shaderModule;
	vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);

	return shaderModule;
}

VkCommandBuffer GFXVulkan::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void GFXVulkan::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

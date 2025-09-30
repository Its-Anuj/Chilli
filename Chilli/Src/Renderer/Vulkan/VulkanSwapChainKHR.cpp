#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "VulkanRenderer.h"
#include "VulkanSwapChainKHR.h"
#include <cstdint>
#include <limits>
#include <algorithm>

namespace Chilli
{
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;

		// TODO: make a score system in future
		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;

		// Always availale
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, int FrameBufferWidth, int FrameBufferHeight)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(FrameBufferWidth),
				static_cast<uint32_t>(FrameBufferHeight) };

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void VulkanSwapChainKHR::Destroy(VulkanDevice& device)
	{
		for (int i = 0; i < _ImageViews.size(); i++)
			vkDestroyImageView(device.GetHandle(), _ImageViews[i], nullptr);

		vkDestroySwapchainKHR(device.GetHandle(), _SwapChain, nullptr);
		VULKAN_PRINTLN("SwapChain deleted!");
	}

	void VulkanSwapChainKHR::Init(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height)
	{
		_CreateSwapChainKHR(device, SurfaceKHR, Width, Height);
		_CreateSwapChainImageViews(device);
	}

	void VulkanSwapChainKHR::Recreate(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height)
	{
		Destroy(device);
		Init(device, SurfaceKHR, Width, Height);
	}

	void VulkanSwapChainKHR::_CreateSwapChainKHR(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height)
	{
		auto& deviceInfo = device.GetPhysicalDevice()->Info;
		SwapChainSupportDetails& support = deviceInfo.SwapChainDetails;

		// Choose surface format
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
		_Format = surfaceFormat.format;

		// Choose present mode (prefer mailbox for triple buffering)
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.presentModes);

		// Choose extent
		_Extent = ChooseSwapExtent(support.capabilities, Width, Height);

		// Determine image count
		uint32_t imageCount = support.capabilities.minImageCount + 1;
		if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
			imageCount = support.capabilities.maxImageCount;

		// Create swapchain
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = SurfaceKHR;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = _Extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		// Handle queue families
		QueueFamilyIndicies& indices = deviceInfo.QueueIndicies;
		uint32_t queueFamilyIndices[] = { indices.Queues[QueueFamilies::GRAPHICS].value(), indices.Queues[QueueFamilies::PRESENT].value() };

		if (indices.Queues[QueueFamilies::GRAPHICS].value() != indices.Queues[QueueFamilies::PRESENT].value())
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

		createInfo.preTransform = support.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		//createInfo.presentMode = presentMode;
		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VULKAN_SUCCESS_ASSERT(vkCreateSwapchainKHR(device.GetHandle(), &createInfo, nullptr, &_SwapChain), "SwapChain createion Failed!");
		VULKAN_PRINTLN("SwapChain created!");

		// Acquire Images
		uint32_t ImageCount = 0;
		vkGetSwapchainImagesKHR(device.GetHandle(), _SwapChain, &ImageCount, nullptr);
		_Images.resize(ImageCount);
		vkGetSwapchainImagesKHR(device.GetHandle(), _SwapChain, &ImageCount, _Images.data());
	}

	void VulkanSwapChainKHR::_CreateSwapChainImageViews(VulkanDevice& device)
	{
		// Imave Views
		_ImageViews.resize(_Images.size());

		for (int i = 0; i < _Images.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = _Images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = _Format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			VULKAN_SUCCESS_ASSERT(vkCreateImageView(device.GetHandle(), &viewInfo, nullptr, &_ImageViews[i]), "Image View Creation Failed!");
		}
		VULKAN_PRINTLN("Image Views created!");
	}
}

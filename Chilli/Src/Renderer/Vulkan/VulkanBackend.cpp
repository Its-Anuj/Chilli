#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanBackend.h"
#include "VulkanConversions.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "VulkanTextures.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkCommandPool __VkCreateCommandPool(VkDevice Device, uint32_t QueueFamilyIndex, VkCommandPoolCreateFlags Flag)
{
	VkCommandPoolCreateInfo Info{};
	Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	Info.queueFamilyIndex = QueueFamilyIndex;
	Info.flags = Flag;

	VkCommandPool Pool;
	VULKAN_SUCCESS_ASSERT(vkCreateCommandPool(Device, &Info, nullptr, &Pool), "Command Pool Creation Failed!");
	return Pool;
}

void __VkDestroyCommandPool(VkDevice Device, VkCommandPool Pool)
{
	vkDestroyCommandPool(Device, Pool, nullptr);
}
/**
 * Transitions an image between different layouts with proper memory barriers
 *
 * @param commandBuffer Command buffer to record the barrier into
 * @param image The image to transition
 * @param oldLayout Current layout of the image
 * @param newLayout Desired new layout for the image
 * @param aspectFlags Image aspect flags (color, depth, stencil)
 */
void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkImageAspectFlags aspectFlags);
namespace Chilli
{
#pragma region VulkanGraphicsBackendApi
	void VulkanGraphicsBackendApi::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Spec = Spec;

		_CreateInstance();
		_CreateDebugMessenger();
		_CreateSurfaceKHR();

		std::vector<const char*> DeviceExtensions;

		if (_Spec.DeviceFeatures.EnableSwapChain)
			DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		// Add only extensions NOT promoted to core in 1.3
		// optional if not core on your driver
		if (_Spec.DeviceFeatures.EnableDescriptorIndexing)
			DeviceExtensions.push_back("VK_EXT_descriptor_indexing");

		_CreatePhysicalDevice(DeviceExtensions);
		_CreateLogicalDevice(DeviceExtensions);

		_CreateSwapChainKHR();

		_CreateCommandPools();
		_CreateAllocator();
		_CreateSynchornization();

		_CreateStagingBufferManager();
		_SetupBindlessSetManager();
		VULKAN_PRINTLN("Vulkan Backend Initiaed!");
	}

	void VulkanGraphicsBackendApi::Terminate()
	{
		_FreeStagingBufferManager();
		_ShutDownBindlessSetManager();

		if (_BufferSet.GetActiveCount() > 0)
		{
			VULKAN_PRINT("All Buffers Have not Been Freed! Alive Left: " << _BufferSet.GetActiveCount());
		}

		_LayoutManager.Clear(_Data.Device.GetHandle());
		_DestroySynchornization();
		this->_DeleteCommandPools();
		_DestroyAllocator();

		_Data.SwapChainKHR.Destroy(_Data.Device);
		VULKAN_PRINTLN("Swap Chain Destroy!");

		_Data.Device.Destroy();
		VULKAN_PRINTLN("Device ShutDown!");

		vkDestroySurfaceKHR(_Data.Instance, _Data.SurfaceKHR, nullptr);
		VULKAN_PRINTLN("Surface KHR ShutDown!");

		if (_Spec.EnableValidation)
		{
			DestroyDebugUtilsMessengerEXT(_Data.Instance, _Data.DebugMessenger, nullptr);
			VULKAN_PRINTLN("Debug Messenger ShutDown!");
		}

		vkDestroyInstance(_Data.Instance, nullptr);
		VULKAN_PRINTLN("Instance ShutDown!");
		VULKAN_PRINTLN("Vulkan ShutDown!");
	}

	void VulkanGraphicsBackendApi::BeginCommandBuffer(const CommandBufferAllocInfo& Info)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = CommandBufferSubmitStateToVk(Info.State);

		_ActiveCommandBuffer = *_CommandBuffers.Get(Info.CommandBufferHandle);
		vkBeginCommandBuffer(_ActiveCommandBuffer, &beginInfo);
		_ActiveCommandBufferHandle = Info.CommandBufferHandle;
	}

	void VulkanGraphicsBackendApi::EndCommandBuffer()
	{
		_ActiveCommandBufferHandle = 0;
		vkEndCommandBuffer(_ActiveCommandBuffer);
		_ActiveCommandBuffer = nullptr;
	}

	void VulkanGraphicsBackendApi::SubmitCommandBuffer(const CommandBufferAllocInfo& Info, const Fence& SubmitFence)
	{
		VkSubmitInfo submit{};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = _CommandBuffers.Get(Info.CommandBufferHandle);

		VkQueue SubmitQueue = _Data.Device.GetQueue(QueueFamilies::GRAPHICS);
		if (Info.Purpose == CommandBufferPurpose::TRANSFER)
			SubmitQueue = _Data.Device.GetQueue(QueueFamilies::TRANSFER);

		VULKAN_SUCCESS_ASSERT(vkQueueSubmit(SubmitQueue, 1, &submit, _Fences.Get(SubmitFence.Handle)),
			"Failed to Submit Queue");
	}

	bool VulkanGraphicsBackendApi::RenderBegin(const CommandBufferAllocInfo& Info, uint32_t FrameIndex)
	{
		auto ActiveCommandBuffer = *_CommandBuffers.Get(Info.CommandBufferHandle);
		_Data.CurrentFrameIndex = FrameIndex;
		vkWaitForFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences[_Data.CurrentFrameIndex], VK_TRUE, UINT64_MAX);

		VkResult result = vkAcquireNextImageKHR(_Data.Device.GetHandle(), _Data.SwapChainKHR.GetHandle(), UINT64_MAX,
			_Data.ImageAvailableSemaphores[_Data.CurrentFrameIndex], VK_NULL_HANDLE,
			&_Data.CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_Spec.ViewPortResized = true;
			_ReCreateSwapChainKHR();
			return false;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		if (result != VK_SUCCESS)
		{
			std::cout << "ERROR: AcquireNextImage failed: " << result << "\n";
		}

		vkResetFences(_Data.Device.GetHandle(), 1, &_Data.InFlightFences[_Data.CurrentFrameIndex]);
		vkResetCommandBuffer(ActiveCommandBuffer, 0);

		BeginCommandBuffer(Info);
		return true;
	}

	void VulkanGraphicsBackendApi::BeginRenderPass(const RenderPass& Pass)
	{
		auto commandBuffer = _ActiveCommandBuffer;

		// Continue with dynamic rendering...
		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.layerCount = 1;

		renderingInfo.colorAttachmentCount = 0;
		renderingInfo.pColorAttachments = nullptr;

		std::vector< VkRenderingAttachmentInfo> ColorAttachments;
		ColorAttachments.reserve(Pass.ColorAttachmentCount);

		if (Pass.ColorAttachmentCount > 0)
		{
			for (int i = 0; i < Pass.ColorAttachmentCount; i++)
			{
				auto& ActiveColorAttachmentInfo = Pass.pColorAttachments[i];

				VkRenderingAttachmentInfo colorAttachment{};
				colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // ✅ Now correct
				colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colorAttachment.clearValue = { {ActiveColorAttachmentInfo.ClearColor.x,
					ActiveColorAttachmentInfo.ClearColor.y, ActiveColorAttachmentInfo.ClearColor.z,
					ActiveColorAttachmentInfo.ClearColor.w} };

				if (ActiveColorAttachmentInfo.UseSwapChainImage)
				{
					// 🆕 TRANSITION: undefined → color attachment optimal (for rendering)
					VkImageMemoryBarrier barrier{};
					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					barrier.image = _Data.SwapChainKHR.GetImages()[_Data.CurrentImageIndex]; // 🆕 Use the actual image
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					barrier.subresourceRange.baseMipLevel = 0;
					barrier.subresourceRange.levelCount = 1;
					barrier.subresourceRange.baseArrayLayer = 0;
					barrier.subresourceRange.layerCount = 1;
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

					vkCmdPipelineBarrier(
						commandBuffer,
						VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,             // Before any operations
						VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // Before color attachment output
						0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

					colorAttachment.imageView = _Data.SwapChainKHR.GetImageViews()[_Data.CurrentImageIndex];
				}

				ColorAttachments.push_back(colorAttachment);
			}
		}
		renderingInfo.pDepthAttachment = nullptr;

		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachments.size());
		renderingInfo.pColorAttachments = ColorAttachments.data();

		renderingInfo.renderArea = { {0, 0}, {(unsigned int)_Data.SwapChainKHR.GetExtent().width
			,(unsigned int)_Data.SwapChainKHR.GetExtent().height} };
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
	}

	void VulkanGraphicsBackendApi::EndRenderPass()
	{
		auto commandBuffer = _ActiveCommandBuffer;

		vkCmdEndRendering(commandBuffer);

		// 🆕 TRANSITION: color attachment optimal → present source (for presentation)
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ✅ Required for presentation
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = _Data.SwapChainKHR.GetImages()[_Data.CurrentImageIndex]; // 🆕 Use the actual image
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // After color attachment writing
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Before presentation
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	void VulkanGraphicsBackendApi::RenderEnd()
	{
		auto commandBuffer = _ActiveCommandBuffer;
		EndCommandBuffer();

		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { _Data.ImageAvailableSemaphores[_Data.CurrentFrameIndex] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores[_Data.CurrentFrameIndex] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			VULKAN_SUCCESS_ASSERT(vkQueueSubmit(_Data.Device.GetQueue(QueueFamilies::GRAPHICS), 1, &submitInfo, _Data.InFlightFences[_Data.CurrentFrameIndex]), "Failed to Submit Graphics Queue");
		}
		{

			VkSemaphore signalSemaphores[] = { _Data.RenderFinishedSemaphores[_Data.CurrentFrameIndex] };

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { _Data.SwapChainKHR.GetHandle() };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &_Data.CurrentImageIndex;
			presentInfo.pResults = nullptr; // Optional

			auto result = vkQueuePresentKHR(_Data.Device.GetQueue(QueueFamilies::PRESENT), &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _Spec.ViewPortResized) {
				_Spec.ViewPortResized = true;
				_ReCreateSwapChainKHR();
			}
			else if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}
		}
	}

	void VulkanGraphicsBackendApi::BindGraphicsPipeline(uint32_t PipelineHandle)
	{
		VkPipelineLayout OldLayout = nullptr;

		if (_ActivePipeline != nullptr)
			OldLayout = _ActivePipeline->GetLayoutFromInfo(_LayoutManager);
		_ActivePipeline = _GraphicsPipelineSet.Get(PipelineHandle);
		vkCmdBindPipeline(_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			_ActivePipeline->GetHandle());

		auto NewLayout = _ActivePipeline->GetLayoutFromInfo(_LayoutManager);
		if (NewLayout != OldLayout)
			_ActivePipelineLayout = NewLayout;

		VkDescriptorSet Sets[] = {
			_BindlessSetManager.Sets()[_Data.CurrentFrameIndex][int(BindlessSetTypes::GLOBAL_SCENE)],
			_BindlessSetManager.Sets()[_Data.CurrentFrameIndex][int(BindlessSetTypes::TEX_SAMPLERS)],
			_BindlessSetManager.Sets()[_Data.CurrentFrameIndex][int(BindlessSetTypes::MATERIAl)],
			_BindlessSetManager.Sets()[_Data.CurrentFrameIndex][int(BindlessSetTypes::PER_OBJECT)]
		};

		// Bind the Bindless Descriptor Sets
		vkCmdBindDescriptorSets(_ActiveCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			_ActivePipelineLayout, 0, 4,
			Sets,
			0, nullptr);
	}

	void VulkanGraphicsBackendApi::BindVertexBuffers(uint32_t* BufferHandles, uint32_t Count)
	{
		// 🆕 Bind vertex buffer
		VkBuffer vertexBuffers[] = { _BufferSet.Get(BufferHandles[0])->Buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_ActiveCommandBuffer, 0, Count, vertexBuffers, offsets);
	}

	void VulkanGraphicsBackendApi::BindIndexBuffer(uint32_t IBHandle)
	{
		vkCmdBindIndexBuffer(_ActiveCommandBuffer, _BufferSet.Get(IBHandle)->Buffer, 0, VK_INDEX_TYPE_UINT16);
	}

	void VulkanGraphicsBackendApi::SetViewPortSize(int Width, int Height)
	{
		// Set dynamic viewport (matches pipeline dynamic state)
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(Width);
		viewport.height = static_cast<float>(Height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(_ActiveCommandBuffer, 0, 1, &viewport);
	}

	void VulkanGraphicsBackendApi::SetScissorSize(int Width, int Height)
	{
		// Set dynamic scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent.width = Width;
		scissor.extent.height = Height;
		vkCmdSetScissor(_ActiveCommandBuffer, 0, 1, &scissor);
	}

	void VulkanGraphicsBackendApi::FrameBufferResize(int Width, int Height)
	{
		_Spec.ViewPortResized = true;
		_Spec.ViewPortSize = { Width, Height };
		_ReCreateSwapChainKHR();
	}

	void VulkanGraphicsBackendApi::DrawArrays(uint32_t Count)
	{
		vkCmdDraw(_ActiveCommandBuffer, Count, 1, 0, 0);
	}

	void VulkanGraphicsBackendApi::SendPushConstant(ShaderStageType Stage, void* Data, uint32_t Size, uint32_t Offset)
	{
		if (_ActivePipeline->GetLayoutInfo().PushConstants.size() > 0 && _ActivePipeline->GetLayoutInfo().PushConstants[0].size < Size)
		{
			VULKAN_PRINTLN("Given Size: " << Size << " Exceeds Size On Push COnstant: " <<
				_ActivePipeline->GetLayoutInfo().PushConstants[0].size << " of the Pipeline!");
			return;
		}
		vkCmdPushConstants(_ActiveCommandBuffer, _ActivePipelineLayout,
			ShaderStageToVk(Stage), Offset, Size, Data);
	}

	uint32_t VulkanGraphicsBackendApi::AllocateBuffer(const BufferCreateInfo& Info)
	{
		if (Info.SizeInBytes == 0)
			return SparseSet<VulkanBuffer>::npos;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = Info.SizeInBytes;
		bufferInfo.usage = BufferTypesToVk(Info.Type);
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};

		if (Info.State == BufferState::STATIC_DRAW)
		{
			// Static GPU-only buffer: You should use a staging upload.
			bufferInfo.usage += VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// No MAPPED flag — GPU_ONLY means it’s not directly mappable
			allocInfo.flags = 0;
		}
		else if (Info.State == BufferState::DYNAMIC_DRAW)
		{
			// Frequent updates, can be directly mapped.
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			allocInfo.flags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}
		else if (Info.State == BufferState::DYNAMIC_STREAM)
		{
			// Extremely frequent updates, maybe even per draw call.
			allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			// STREAM can also use sequential write + mapped, but could also benefit
			// from `HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT` if you double-buffer updates.
			allocInfo.flags =
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VkBuffer Buffer;
		VmaAllocation Allocation;
		VmaAllocationInfo AllocationInfo;

		auto Result = vmaCreateBuffer(_Allocator, &bufferInfo, &allocInfo, &Buffer, &Allocation, &AllocationInfo);

		if (Result != VK_SUCCESS) {
			// Handle error
			VULKAN_ERROR("vmaCreateBuffer Error!");
			return SparseSet<VulkanBuffer>::npos;
		}

		auto NewBufferHandle = _BufferSet.Create(VulkanBuffer{ Buffer, Allocation, AllocationInfo, Info });

		if (Info.State == BufferState::STATIC_DRAW)
		{
			// Use Staging Buffer
			// Check Data is not NUll
			assert(Info.Data != nullptr && "For a STATIC_DRAW Buffer Create Info State Data should not be nullptr!");

			if (Info.SizeInBytes > _StagingBufferManager.StagingBufferPoolSize)
			{
				// Chunked copy for large buffers
				size_t remainingBytes = Info.SizeInBytes;
				size_t offset = 0;
				void* dataPtr = static_cast<void*>(Info.Data);

				while (remainingBytes > 0)
				{
					size_t chunkSize = min(remainingBytes,
						static_cast<size_t>(_StagingBufferManager.StagingBufferPoolSize));
					MapBufferData(_StagingBufferManager.BufferID, dataPtr, chunkSize, offset);

					BufferCopyInfo CopyInfo{};
					CopyInfo.DstOffset = offset;
					CopyInfo.SrcOffset = 0;
					CopyInfo.Size = chunkSize;

					CopyBufferToBuffer(_StagingBufferManager.BufferID, NewBufferHandle, CopyInfo);

					offset += chunkSize;
					remainingBytes -= chunkSize;
				};
			}
			else
			{
				// Copy Whole
				MapBufferData(_StagingBufferManager.BufferID, Info.Data, Info.SizeInBytes, 0);

				// Copy Buffer
				BufferCopyInfo CopyInfo{};
				CopyInfo.DstOffset = 0;
				CopyInfo.SrcOffset = 0;
				CopyInfo.Size = Info.SizeInBytes;

				CopyBufferToBuffer(_StagingBufferManager.BufferID, NewBufferHandle, CopyInfo);
			}
		}

		return NewBufferHandle;
	}

	void VulkanGraphicsBackendApi::MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset)
	{
		auto Buffer = _BufferSet.Get(BufferHandle);
		if (Buffer == nullptr)
			return;

		void* data;
		vmaMapMemory(_Allocator, Buffer->Allocation, &data);
		memcpy((uint8_t*)data + Offset, Data, static_cast<uint32_t>(Size));
		vmaUnmapMemory(_Allocator, Buffer->Allocation);
	}

	void VulkanGraphicsBackendApi::FreeBuffer(uint32_t BufferHandle)
	{
		if (_BufferSet.HasVal(BufferHandle) == false) return;

		auto Buffer = _BufferSet.Get(BufferHandle);
		vmaDestroyBuffer(_Allocator, Buffer->Buffer, Buffer->Allocation);
		_BufferSet.Destroy(BufferHandle);
	}
#pragma region Textures and Images
	uint32_t VulkanGraphicsBackendApi::AllocateImage(const ImageSpec& ImgSpec, const char* FilePath)
	{
		VulkanTexture NewTexutre;
		ImageSpec Spec = ImgSpec;

		stbi_uc* Pixels = nullptr;

		if (FilePath != nullptr)
		{
			stbi_set_flip_vertically_on_load(ImgSpec.YFlip);

			// Load image data using stb_image
			int texWidth, texHeight, texChannels;
			std::string Filepath = FilePath;

			Pixels = stbi_load(Filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!Pixels)
				throw std::runtime_error("failed to load texture image!");
			Spec.Resolution = { texWidth, texHeight };
		}

		NewTexutre.Init(_Data.Device.GetHandle(), _Allocator, Spec, FilePath);

		// Copy Data Into Image
		// Step 1 - Copy Data into staging Buffer
		MapBufferData(_StagingBufferManager.BufferID, (void*)Pixels, Spec.Resolution.Width * Spec.Resolution.Height * 4, 0);
		// Transition Image to Be able to Buffer Data copied into Image

		auto NewCmdInfo = AllocateCommandBuffer(CommandBufferPurpose::GRAPHICS);

		Fence fence;
		fence.Handle = CreateFence();

		BeginCommandBuffer(NewCmdInfo);

		_TransitionImageLayout(_ActiveCommandBuffer, NewTexutre.GetImageHandle(), VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		EndCommandBuffer();

		ResetFence(fence);
		SubmitCommandBuffer(NewCmdInfo, fence);;
		WaitForFence(fence);

		NewCmdInfo.Purpose = CommandBufferPurpose::TRANSFER;
		BeginCommandBuffer(NewCmdInfo);
		// Copy Data
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;   // Tightly packed
		region.bufferImageHeight = 0; // Tightly packed

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = Spec.Resolution.Width;
		region.imageExtent.height = Spec.Resolution.Height;
		region.imageExtent.depth = 1;

		// Issue copy command
		vkCmdCopyBufferToImage(
			_ActiveCommandBuffer,
			_BufferSet.Get(_StagingBufferManager.BufferID)->Buffer,
			NewTexutre.GetImageHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);

		EndCommandBuffer();

		ResetFence(fence);
		SubmitCommandBuffer(NewCmdInfo, fence);;
		WaitForFence(fence);

		NewCmdInfo.Purpose = CommandBufferPurpose::GRAPHICS;
		BeginCommandBuffer(NewCmdInfo);
		_TransitionImageLayout(_ActiveCommandBuffer, NewTexutre.GetImageHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

		EndCommandBuffer();

		ResetFence(fence);
		SubmitCommandBuffer(NewCmdInfo, fence);;
		WaitForFence(fence);

		DestroyFence(fence);
		FreeCommandBuffer(NewCmdInfo);

		auto ReturnIndex = _TextureSet.Create(NewTexutre);

		uint32_t  GpuMaterialID;
		if (_BindlessSetManager.GetTextureMap().Contains(ReturnIndex) == false)
			GpuMaterialID = _BindlessSetManager.GetTextureMap().Create(ReturnIndex);
		else
			GpuMaterialID = *_BindlessSetManager.GetTextureMap().Get(ReturnIndex);

		_BindlessSetManager.WriteBindlessTextures(
			_Data.Device.GetHandle(),
			NewTexutre.GetHandle(),
			GpuMaterialID);

		if (Pixels != nullptr)
			stbi_image_free(Pixels);

		return ReturnIndex;
	}

	void VulkanGraphicsBackendApi::LoadImageData(uint32_t TexHandle, const char* FilePath)
	{
	}

	void VulkanGraphicsBackendApi::FreeTexture(uint32_t TexHandle)
	{
		auto Texture = _TextureSet.Get(TexHandle);
		Texture->Terminate(_Data.Device.GetHandle(), _Allocator);
		_TextureSet.Destroy(TexHandle);
		_BindlessSetManager.GetTextureMap().Destroy(TexHandle);
	}

	uint32_t VulkanGraphicsBackendApi::CreateSampler(const Sampler& sampler)
	{
		auto NewSampler = VulkanSampler{
			VulkanSampler::Create(
				_Data.Device.GetHandle(),
				_Data.Device.GetPhysicalDevice()->Info.features.samplerAnisotropy,
				sampler
			) };
		auto ReturnIndex = _Samplers.Create(NewSampler);

		_BindlessSetManager.WriteBindlessSamplers(
			_Data.Device.GetHandle(),
			NewSampler.Handle,
			_BindlessSetManager.GetSamplerMap().Create(ReturnIndex));
		return ReturnIndex;
	}

	void VulkanGraphicsBackendApi::DestroySampler(uint32_t sampler)
	{
		VulkanSampler::Destroy(_Data.Device.GetHandle(), *_Samplers.Get(sampler));
		_Samplers.Destroy(sampler);
		_BindlessSetManager.GetSamplerMap().Destroy(sampler);
	}
#pragma endregion

	uint32_t VulkanGraphicsBackendApi::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& CreateInfo)
	{
		VulkanGraphicsPipeline Shader;

		std::array<VkDescriptorSetLayout, int(BindlessSetTypes::COUNT_NON_USER)> BindlessLayouts;
		for (int i = 0; i < int(BindlessSetTypes::COUNT_NON_USER); i++)
			BindlessLayouts[i] = _BindlessSetManager.SetLayouts()[i];

		Shader.Init(CreateInfo, _Data.Device.GetHandle(), _Data.SwapChainKHR.GetFormat(), _LayoutManager,
			BindlessLayouts);
		return _GraphicsPipelineSet.Create(Shader);
	}

	void VulkanGraphicsBackendApi::DestroyGraphicsPipeline(uint32_t PipelineHandle)
	{
		auto Shader = _GraphicsPipelineSet.Get(PipelineHandle);
		Shader->Terminate(_Data.Device.GetHandle());
		_GraphicsPipelineSet.Destroy(PipelineHandle);
	}

	void VulkanGraphicsBackendApi::_CreateInstance()
	{
		std::vector<const char*> RequiredExts, RequiredLayers;

		uint32_t glfwExtCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);

		RequiredExts.push_back(glfwExtensions[0]);
		RequiredExts.push_back(glfwExtensions[1]);

		if (_Spec.EnableValidation)
		{
			// Check if validation layer exists
			RequiredLayers.push_back("VK_LAYER_KHRONOS_validation");
			RequiredExts.push_back("VK_EXT_debug_utils");
		}

		VkApplicationInfo appinfo{};
		appinfo.apiVersion = VK_API_VERSION_1_3;
		appinfo.pEngineName = nullptr;
		appinfo.pApplicationName = _Spec.Name;
		appinfo.applicationVersion = VK_API_VERSION_1_3;
		appinfo.engineVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appinfo;

		createInfo.enabledExtensionCount = RequiredExts.size();
		createInfo.enabledLayerCount = RequiredLayers.size();

		createInfo.ppEnabledExtensionNames = nullptr;
		createInfo.ppEnabledLayerNames = nullptr;
		if (RequiredLayers.size() != 0)
			createInfo.ppEnabledLayerNames = RequiredLayers.data();
		if (RequiredExts.size() != 0)
			createInfo.ppEnabledExtensionNames = RequiredExts.data();

		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;

		if (_Spec.EnableValidation)
		{
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

			_PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		VULKAN_SUCCESS_ASSERT(vkCreateInstance(&createInfo, nullptr, &_Data.Instance), "Instance Creation Failed!");
		VULKAN_PRINTLN("Instance Created! for: " << _Spec.Name);
	}

	void VulkanGraphicsBackendApi::_CreateDebugMessenger()
	{
		if (!_Spec.EnableValidation)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		_PopulateDebugMessengerCreateInfo(createInfo);

		VULKAN_SUCCESS_ASSERT(CreateDebugUtilsMessengerEXT(_Data.Instance, &createInfo, nullptr, &_Data.DebugMessenger), "Debug Messenger not loaded!");
		VULKAN_PRINTLN("Debug Messenger Loaded!");
	}

	void VulkanGraphicsBackendApi::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
		createInfo.pNext = nullptr;     // Optional
		createInfo.flags = 0;           // Must be 0
	}

	void VulkanGraphicsBackendApi::_CreateSurfaceKHR()
	{
		glfwCreateWindowSurface(_Data.Instance, (GLFWwindow*)_Spec.WindowSurfaceHandle, nullptr, &_Data.SurfaceKHR);
		VULKAN_PRINTLN("Window Surface KHR Created!");
	}

	void VulkanGraphicsBackendApi::_CreatePhysicalDevice(std::vector<const char*>& DeviceExtensions)
	{
		uint32_t PDeviceCount = 0;
		vkEnumeratePhysicalDevices(_Data.Instance, &PDeviceCount, nullptr);

		std::vector<VkPhysicalDevice> PDevices;
		PDevices.resize(PDeviceCount);
		vkEnumeratePhysicalDevices(_Data.Instance, &PDeviceCount, PDevices.data());

		if (PDevices.size() == 0)
			throw std::runtime_error("No Vulkan Supporting device found!");

		VULKAN_PRINTLN("Vulkan Supported devices: " << PDevices.size())
			_Data.PhysicalDevices.resize(PDevices.size());

		for (int i = 0; i < PDevices.size(); i++)
			_Data.PhysicalDevices[i].PhysicalDevice = PDevices[i];

		// Check how good each phsyical device is in features
		for (auto& device : _Data.PhysicalDevices)
		{
			_FillVulkanPhysicalDevice(device, _Data.SurfaceKHR);

			device.Info.SwapChainDetails = QuerySwapChainSupport(device.PhysicalDevice, _Data.SurfaceKHR);

			VULKAN_PRINTLN("Device Found: " << device.Info.properties.deviceName << "\n");
		}
		for (auto& device : _Data.PhysicalDevices)
			CheckVulkanPhysicalDeviceExtensions(device, DeviceExtensions);

		_FindSuitablePhysicalDevice();
	}

	void VulkanGraphicsBackendApi::_FindSuitablePhysicalDevice()
	{
		std::vector<int> Scores(_Data.PhysicalDevices.size());

		int i = 0;
		for (auto& device : _Data.PhysicalDevices)
		{
			auto& deviceinfo = device.Info;
			int CurrentScore = 0;

			if (deviceinfo.features13.dynamicRendering == true)
				CurrentScore += 1000;
			if (deviceinfo.features13.synchronization2 == true)
				CurrentScore += 1000;
			if (deviceinfo.features13.maintenance4 == true)
				CurrentScore += 1000;
			if (deviceinfo.features13.pipelineCreationCacheControl)
				CurrentScore += 1000;
			if (deviceinfo.features13.shaderDemoteToHelperInvocation)
				CurrentScore += 1000;
			if (deviceinfo.features13.shaderTerminateInvocation)
				CurrentScore += 1000;
			if (deviceinfo.features12.samplerFilterMinmax)
				CurrentScore += 1000;
			if (deviceinfo.features12.bufferDeviceAddressCaptureReplay)
				CurrentScore += 1000;

			if (deviceinfo.features.geometryShader)
				CurrentScore += 1000;
			if (deviceinfo.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				CurrentScore += 10000;
			if (deviceinfo.QueueIndicies.Complete())
				CurrentScore += 10000;

			if ((!deviceinfo.SwapChainDetails.formats.empty() && !deviceinfo.SwapChainDetails.presentModes.empty()) == true)
				CurrentScore += 10000;
			VULKAN_PRINTLN(deviceinfo.properties.deviceName << " scored: " << CurrentScore);
			Scores[i++] = CurrentScore;
		}

		int Bestid = 0;
		for (int i = 0; i < _Data.PhysicalDevices.size(); i++)
		{
			if (Scores[Bestid] < Scores[i])
				Bestid = i;
		}
		VULKAN_PRINTLN("Chosen Device: " << _Data.PhysicalDevices[Bestid].Info.properties.deviceName << "\n")
			_Data.ActivePhysicalDeviceIndex = Bestid;
	}

	void VulkanGraphicsBackendApi::_CreateLogicalDevice(std::vector<const char*>& DeviceExtensions)
	{
		_Data.Device.Init(&_Data.PhysicalDevices[_Data.ActivePhysicalDeviceIndex], DeviceExtensions);
	}

	void VulkanGraphicsBackendApi::_CreateSwapChainKHR()
	{
		_Data.SwapChainKHR.Init(_Data.Device, _Data.SurfaceKHR, _Spec.ViewPortSize.x, _Spec.ViewPortSize.y, _Spec.VSync);
		_Spec.ViewPortResized = false;
	}

	void VulkanGraphicsBackendApi::_ReCreateSwapChainKHR()
	{
		if (_Spec.ViewPortResized)
			_Data.SwapChainKHR.Recreate(_Data.Device, _Data.SurfaceKHR, _Spec.ViewPortSize.x, _Spec.ViewPortSize.y, _Spec.VSync);
		_Spec.ViewPortResized = false;
	}

#pragma region Command Buffers and Pools
	void VulkanGraphicsBackendApi::_CreateCommandPools()
	{
		_CommandPools[int(CommandBufferPurpose::GRAPHICS)] = __VkCreateCommandPool(_Data.Device.GetHandle(),
			_Data.Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::GRAPHICS].value(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		_CommandPools[int(CommandBufferPurpose::TRANSFER)] = __VkCreateCommandPool(_Data.Device.GetHandle(),
			_Data.Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[QueueFamilies::TRANSFER].value(),
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
	}

	void VulkanGraphicsBackendApi::_DeleteCommandPools()
	{
		__VkDestroyCommandPool(_Data.Device.GetHandle(), _CommandPools[int(CommandBufferPurpose::GRAPHICS)]);
		__VkDestroyCommandPool(_Data.Device.GetHandle(), _CommandPools[int(CommandBufferPurpose::TRANSFER)]);
	}

	void VulkanGraphicsBackendApi::FreeCommandBuffer(CommandBufferAllocInfo& Info)
	{
		std::vector< CommandBufferAllocInfo> Infos;
		Infos.push_back(Info);
		FreeCommandBuffers(Infos);
	}

	CommandBufferAllocInfo VulkanGraphicsBackendApi::AllocateCommandBuffer(CommandBufferPurpose Purpose)
	{
		std::vector<CommandBufferAllocInfo> Info;
		AllocateCommandBuffers(Purpose, Info, 1);
		return Info[0];
	}

	void VulkanGraphicsBackendApi::AllocateCommandBuffers(CommandBufferPurpose Purpose, std::vector<CommandBufferAllocInfo>& Infos, uint32_t Count)
	{
		Infos.reserve(Count);

		std::vector<VkCommandBuffer> CmdBuffers;
		CmdBuffers.resize(Count);

		VkCommandBufferAllocateInfo Info{};
		Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		Info.commandBufferCount = (uint32_t)CmdBuffers.size();
		Info.commandPool = _CommandPools[int(Purpose)];
		Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VULKAN_SUCCESS_ASSERT(vkAllocateCommandBuffers(_Data.Device.GetHandle(), &Info, CmdBuffers.data()), "Command Buffer Failed to Allocate!");

		for (auto& Cmd : CmdBuffers)
			Infos.push_back(CommandBufferAllocInfo{ .CommandBufferHandle = _CommandBuffers.Create(Cmd),
				   .Purpose = Purpose });
	}

	void VulkanGraphicsBackendApi::FreeCommandBuffers(std::vector<CommandBufferAllocInfo>& Infos)
	{
		std::vector<VkCommandBuffer> Buffers;
		for (auto& CmdInfo : Infos)
			Buffers.push_back(*_CommandBuffers.Get(CmdInfo.CommandBufferHandle));

		vkFreeCommandBuffers(_Data.Device.GetHandle(), _CommandPools[int(Infos[0].Purpose)]
			, static_cast<uint32_t>(Buffers.size()), Buffers.data());

		for (auto& CmdInfo : Infos)
			_CommandBuffers.Destroy(CmdInfo.CommandBufferHandle);
	}

	void VulkanGraphicsBackendApi::ResetCommandBuffer(const CommandBufferAllocInfo& Info)
	{
		vkResetCommandBuffer(*_CommandBuffers.Get(Info.CommandBufferHandle), 0);
	}
#pragma endregion

	void VulkanGraphicsBackendApi::_CreateAllocator()
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _Data.Device.GetPhysicalDevice()->PhysicalDevice;
		allocatorInfo.device = _Data.Device.GetHandle();
		allocatorInfo.instance = _Data.Instance;
		VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Allocator), "VMA Allocator failed to initialize!");
		VULKAN_PRINTLN("VMA Created!!");
	}

	void VulkanGraphicsBackendApi::_DestroyAllocator()
	{
		vmaDestroyAllocator(_Allocator);
	}

	void VulkanGraphicsBackendApi::_CreateSynchornization()
	{
		_Data.ImageAvailableSemaphores.resize(_Spec.MaxFrameInFlight);
		_Data.RenderFinishedSemaphores.resize(_Spec.MaxFrameInFlight);
		_Data.InFlightFences.resize(_Spec.MaxFrameInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		auto device = _Data.Device.GetHandle();

		for (size_t i = 0; i < _Spec.MaxFrameInFlight; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_Data.RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &_Data.InFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	void VulkanGraphicsBackendApi::_DestroySynchornization()
	{
		auto device = _Data.Device.GetHandle();

		for (size_t i = 0; i < _Spec.MaxFrameInFlight; i++) {
			vkDestroySemaphore(device, _Data.RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, _Data.ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, _Data.InFlightFences[i], nullptr);
		}
	}
#pragma region Staging Buffers
	void VulkanGraphicsBackendApi::_CreateStagingBufferManager()
	{
		// Create StagingBuffer Manager
		auto CmdHandle = AllocateCommandBuffer(CommandBufferPurpose::TRANSFER);
		CmdHandle.State = COMMAND_BUFFER_SUBMIT_STATE_ONE_TIME;
		_StagingBufferManager.StagingBufferCmd = CmdHandle;

		// Allocate Staging Buffer
		BufferCreateInfo StagingBufferInfo{};
		StagingBufferInfo.State = BufferState::DYNAMIC_DRAW;
		StagingBufferInfo.Data = nullptr;
		StagingBufferInfo.SizeInBytes = _StagingBufferManager.StagingBufferPoolSize;
		StagingBufferInfo.Type = BUFFER_TYPE_STORAGE | BUFFER_TYPE_TRANSFER_SRC;
		auto StagingBufferID = AllocateBuffer(StagingBufferInfo);

		_StagingBufferManager.BufferID = StagingBufferID;

		_StagingBufferManager.Fence.Handle = CreateFence();
	}

	void VulkanGraphicsBackendApi::_FreeStagingBufferManager()
	{
		FreeBuffer(_StagingBufferManager.BufferID);
		FreeCommandBuffer(_StagingBufferManager.StagingBufferCmd);
		DestroyFence(_StagingBufferManager.Fence);
	}

	void VulkanGraphicsBackendApi::CopyBufferToBuffer(uint32_t SrcHandle, uint32_t DstHandle, const BufferCopyInfo& Info)
	{
		auto srcBuffer = _BufferSet.Get(SrcHandle);
		auto dstBuffer = _BufferSet.Get(DstHandle);

		if (!srcBuffer || !dstBuffer) {
			VULKAN_ERROR("Invalid buffer handles in CopyBufferToBuffer");
			return;
		}

		auto& StagingBufferCmd = _StagingBufferManager.StagingBufferCmd;
		auto VkCmdHandle = *_CommandBuffers.Get(StagingBufferCmd.CommandBufferHandle);
		BeginCommandBuffer(StagingBufferCmd);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = Info.SrcOffset; // Offset in bytes from the start of srcBuffer
		copyRegion.dstOffset = Info.DstOffset; // Offset in bytes from the start of dstBuffer
		copyRegion.size = Info.Size; // Size of the data to copy in bytes

		vkCmdCopyBuffer(_ActiveCommandBuffer, srcBuffer->Buffer,
			dstBuffer->Buffer, 1, &copyRegion);

		EndCommandBuffer();
		auto device = _Data.Device.GetHandle();

		ResetFence(_StagingBufferManager.Fence);
		SubmitCommandBuffer(_StagingBufferManager.StagingBufferCmd, _StagingBufferManager.Fence);
		WaitForFence(_StagingBufferManager.Fence);

		//VkSubmitInfo submitInfo{};
	//submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//submitInfo.commandBufferCount = 1;
	//submitInfo.pCommandBuffers = &VkCmdHandle;

	//// Add Fences
	//vkQueueSubmit(_Data.Device.GetQueue(QueueFamilies::TRANSFER), 1, &submitInfo, _StagingBufferManager.Fence);
	//vkWaitForFences(_Data.Device.GetHandle(), 1, &_StagingBufferManager.Fence, VK_TRUE, UINT64_MAX);

		ResetCommandBuffer(_StagingBufferManager.StagingBufferCmd);
	}
#pragma endregion 
	uint32_t VulkanGraphicsBackendApi::CreateFence(bool signaled)
	{
		return _Fences.CreateFence(_Data.Device.GetHandle(), signaled);
	}

	void VulkanGraphicsBackendApi::DestroyFence(const Fence& fence)
	{
		_Fences.Destroy(_Data.Device.GetHandle(), fence.Handle);
	}

	void VulkanGraphicsBackendApi::ResetFence(const Fence& fence)
	{
		_Fences.Reset(_Data.Device.GetHandle(), fence.Handle);
	}

	bool VulkanGraphicsBackendApi::IsFenceSignaled(const Fence& fence)
	{
		return _Fences.IsSignaled(_Data.Device.GetHandle(), fence.Handle);
	}

	void VulkanGraphicsBackendApi::WaitForFence(const Fence& fence, uint64_t TimeOut)
	{
		_Fences.Wait(_Data.Device.GetHandle(), fence.Handle, TimeOut);
	}
#pragma region Bindless Descriptor Set

	void VulkanGraphicsBackendApi::UpdateGlobalShaderData(const GlobalShaderData& Data)
	{
		assert(_ActiveCommandBuffer != VK_NULL_HANDLE && "Cant be updated without a CommandBuffer Recording!");
		_BindlessSetManager.UpdateGlobalShaderData(Data);
	}

	void VulkanGraphicsBackendApi::UpdateSceneShaderData(const SceneShaderData& Data)
	{
		assert(_ActiveCommandBuffer != VK_NULL_HANDLE && "Cant be updated without a CommandBuffer Recording!");
		_BindlessSetManager.UpdateSceneShaderData(Data);
	}

	void VulkanGraphicsBackendApi::UpdateMaterialSSBO(const BackBone::AssetHandle<Material>& Mat)
	{
		uint32_t  GpuMaterialID;
		if (!_BindlessSetManager.GetMaterialMap().Contains(Mat.Handle))
			GpuMaterialID = _BindlessSetManager.GetMaterialMap().Create(Mat.Handle);
		else
			GpuMaterialID = *_BindlessSetManager.GetMaterialMap().Get(Mat.Handle);

		_BindlessSetManager.UpdateMaterialShaderData(*Mat.ValPtr,
			GpuMaterialID);
	}

	void VulkanGraphicsBackendApi::UpdateObjectSSBO(const glm::mat4& TransformationMat, BackBone::Entity EntityID)
	{
		uint32_t  GpuMaterialID;
		if (!_BindlessSetManager.GetObjectMap().Contains(EntityID))
			GpuMaterialID = _BindlessSetManager.GetObjectMap().Create(EntityID);
		else
			GpuMaterialID = *_BindlessSetManager.GetObjectMap().Get(EntityID);

		_BindlessSetManager.UpdateObjectShaderData(TransformationMat,
			GpuMaterialID);
	}

	void VulkanGraphicsBackendApi::_SetupBindlessSetManager()
	{
		CreateInfo.Device = &_Data.Device;
		CreateInfo.MaxFrameInFlight = _Spec.MaxFrameInFlight;
		CreateInfo.GetBuffer = [&](uint32_t id) {
			return this->_BufferSet.Get(id)->Buffer;
			};
		CreateInfo.AllocateBuffer = [&](const BufferCreateInfo& info) {
			return this->AllocateBuffer(info);
			};

		CreateInfo.FreeBuffer = [&](uint32_t id) {
			this->FreeBuffer(id);
			};

		CreateInfo.MapBufferData = [&](uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset) {
			this->MapBufferData(BufferHandle, Data, Size, Offset);
			};

		_BindlessSetManager.Init(CreateInfo);
	}

	void VulkanGraphicsBackendApi::_ShutDownBindlessSetManager()
	{
		_BindlessSetManager.Destroy(CreateInfo);
	}
#pragma endregion 

#pragma endregion

#pragma region VulkanFenceManager
	uint32_t VulkanFenceManager::CreateFence(VkDevice Device, bool signaled)
	{
		VkFenceCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		VkFence backendFence;
		if (vkCreateFence(Device, &info, nullptr, &backendFence) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create fence!");
		}

		FenceEntry entry;
		entry.BackendFence = backendFence;
		entry.inUse = true;

		return  _Fences.Create(entry); // returns the id from sparse set
	}

	void VulkanFenceManager::Wait(VkDevice Device, const uint32_t& handle, uint64_t timeoutNs)
	{
		auto entry = _Fences.Get(handle);
		vkWaitForFences(Device, 1, &entry->BackendFence, VK_TRUE, timeoutNs);
	}

	bool VulkanFenceManager::IsSignaled(VkDevice Device, const uint32_t& handle)
	{
		auto entry = _Fences.Get(handle);
		VkResult res = vkGetFenceStatus(Device, entry->BackendFence);
		return res == VK_SUCCESS;
	}

	void VulkanFenceManager::Reset(VkDevice Device, const uint32_t& handle)
	{
		auto entry = _Fences.Get(handle);
		vkResetFences(Device, 1, &entry->BackendFence);
	}

	void VulkanFenceManager::Destroy(VkDevice Device, const uint32_t& handle)
	{
		auto entry = _Fences.Get(handle);
		vkDestroyFence(Device, entry->BackendFence, nullptr);
		_Fences.Destroy(handle);
	}
#pragma endregion
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void _TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
	VkImageLayout oldLayout, VkImageLayout newLayout,
	VkImageAspectFlags aspectFlags)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	// We're not transferring queue ownership
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1; // Transition only first mip level
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1; // Transition only first array layer

	VkPipelineStageFlags sourceStage = 0;
	VkPipelineStageFlags destinationStage = 0;

	// --- UNDEFINED to various layouts ---
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
	{
		// Initial layout - no need to preserve existing contents
		barrier.srcAccessMask = 0;

		if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			// Image will be used as transfer destination (texture loading)
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			// Image will be used as transfer source (texture copying)
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			// Image will be used as depth/stencil attachment
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			// Image will be used as color attachment
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			// Image will be sampled in shaders
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			// Image will be presented to swapchain
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
	}
	// --- TRANSFER_DST to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// After texture upload, make it available for shader sampling
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	// --- COLOR_ATTACHMENT to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// After rendering to color attachment, use as texture (post-processing)
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	// --- SHADER_READ_ONLY to COLOR_ATTACHMENT ---
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		// Texture used as render target (for example in compute shader output)
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// --- COLOR_ATTACHMENT to PRESENT_SRC ---
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		// After rendering, transition to presentable layout
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	// --- PRESENT_SRC to COLOR_ATTACHMENT ---
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		// Before rendering, transition from presentable to color attachment
		barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// --- Add more transitions as needed for your engine ---
	else
	{
		throw std::invalid_argument("Unsupported layout transition from " +
			std::to_string(oldLayout) + " to " +
			std::to_string(newLayout));
	}

	// Record the pipeline barrier
	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0, // dependency flags
		0, nullptr, // memory barrier count and data
		0, nullptr, // buffer memory barrier count and data
		1, &barrier // image memory barrier count and data
	);
}
const char* VkResultToChar(VkResult result)
{
	switch (result)
	{
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_NOT_READY: return "VK_NOT_READY";
	case VK_TIMEOUT: return "VK_TIMEOUT";
	case VK_EVENT_SET: return "VK_EVENT_SET";
	case VK_EVENT_RESET: return "VK_EVENT_RESET";
	case VK_INCOMPLETE: return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
	case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
	case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
	case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
	case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
	case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
	case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
	case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
	case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
	case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
	case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
	case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
	case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
	case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
	case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
	case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
	case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
	case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
	case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
	case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
	case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
	case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
	case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
	case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR: return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
	case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
	case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT: return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
	default: return "VK_ERROR_UNKNOWN_ENUM";
	}
}

Chilli::SwapChainSupportDetails Chilli::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	Chilli::SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}
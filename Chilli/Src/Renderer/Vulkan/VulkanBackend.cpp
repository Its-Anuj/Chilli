#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanBackend.h"
#include "VulkanConversions.h"

#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

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

PFN_vkCmdSetPolygonModeEXT pfn_vkCmdSetPolygonModeEXT;
PFN_vkCmdSetColorBlendEnableEXT pfn_vkCmdSetColorBlendEnableEXT;
PFN_vkCmdSetColorWriteEnableEXT pfn_vkCmdSetColorWriteEnableEXT;

// Get Vulkan 1.3 function
PFN_vkCmdPipelineBarrier2 pfn_vkCmdPipelineBarrier2 = nullptr;

PFN_vkCmdSetRasterizationSamplesEXT pfn_vkCmdSetRasterizationSamplesEXT = nullptr;
PFN_vkCmdSetSampleMaskEXT pfn_vkCmdSetSampleMaskEXT = nullptr;
PFN_vkCmdSetAlphaToCoverageEnableEXT pfn_vkCmdSetAlphaToCoverageEnableEXT = nullptr;
PFN_vkCmdSetColorWriteMaskEXT pfn_vkCmdSetColorWriteMaskEXT = nullptr;
PFN_vkCmdSetColorBlendEquationEXT pfn_vkCmdSetColorBlendEquationEXT = nullptr;
PFN_vkCmdSetVertexInputEXT pfn_vkCmdSetVertexInputEXT = nullptr;

void LoadExtendedDynamicStateFunctions(VkDevice device)
{
	pfn_vkCmdSetColorWriteEnableEXT = (PFN_vkCmdSetColorWriteEnableEXT)vkGetDeviceProcAddr(device, "vkCmdSetColorWriteEnableEXT");
	pfn_vkCmdSetColorBlendEnableEXT = (PFN_vkCmdSetColorBlendEnableEXT)vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEnableEXT");
	pfn_vkCmdSetPolygonModeEXT = (PFN_vkCmdSetPolygonModeEXT)vkGetDeviceProcAddr(device, "vkCmdSetPolygonModeEXT");
	pfn_vkCmdPipelineBarrier2 = (PFN_vkCmdPipelineBarrier2)vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2");
	LoadShaderObjectEXTFunctions(device);

	pfn_vkCmdSetRasterizationSamplesEXT = (PFN_vkCmdSetRasterizationSamplesEXT)vkGetDeviceProcAddr(device, "vkCmdSetRasterizationSamplesEXT");
	pfn_vkCmdSetSampleMaskEXT = (PFN_vkCmdSetSampleMaskEXT)vkGetDeviceProcAddr(device, "vkCmdSetSampleMaskEXT");
	pfn_vkCmdSetAlphaToCoverageEnableEXT = (PFN_vkCmdSetAlphaToCoverageEnableEXT)vkGetDeviceProcAddr(device, "vkCmdSetAlphaToCoverageEnableEXT");
	pfn_vkCmdSetColorWriteMaskEXT = (PFN_vkCmdSetColorWriteMaskEXT)vkGetDeviceProcAddr(device, "vkCmdSetColorWriteMaskEXT");
	pfn_vkCmdSetColorBlendEquationEXT = (PFN_vkCmdSetColorBlendEquationEXT)vkGetDeviceProcAddr(device, "vkCmdSetColorBlendEquationEXT");
	pfn_vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)vkGetDeviceProcAddr(device, "vkCmdSetVertexInputEXT");
	// optional: check nullptr for each pointer
}

namespace Chilli
{
	VulkanGraphicsBackend::VulkanGraphicsBackend(const GraphcisBackendCreateSpec& Spec)
	{
		Init(Spec);
	}

	void VulkanGraphicsBackend::Init(const GraphcisBackendCreateSpec& Spec)
	{
		_Spec = Spec;
		_Data.MaxSets = CH_MATERIAL_SHADER_DATA_AMOUNT * Spec.MaxFrameInFlight * int(BindlessSetTypes::COUNT_USER);

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
		_CreateVMAAllocator();
		LoadExtendedDynamicStateFunctions(_Data.Device.GetHandle());

		_CommandManager.Init(_Data.Device);

		_CreateVulkanDataUploader();
		_CreateVulkanImageDataManager();

		_BufferManager.Init(_Data.Device, _Data.Allocator, &_Uploader, 1e6);
		_CreateFrameResources();

		_BindlessCreateInfo.Device = &_Data.Device;
		_BindlessCreateInfo.MaxFrameInFlight = _Spec.MaxFrameInFlight;
		_BindlessCreateInfo.GetBuffer = [&](uint32_t id) {
			return this->_BufferManager.Get(id)->Buffer;
			};
		_BindlessCreateInfo.AllocateBuffer = [&](const BufferCreateInfo& info) {
			return this->AllocateBuffer(info);
			};

		_BindlessCreateInfo.FreeBuffer = [&](uint32_t id) {
			this->FreeBuffer(id);
			};

		_BindlessCreateInfo.MapBufferData = [&](uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset) {
			this->MapBufferData(BufferHandle, Data, Size, Offset);
			};

		_BindlessManager.Init(_BindlessCreateInfo);
		_CreateGeneralDescriptorPool();

		VULKAN_PRINTLN("Vulkan Backend Initiaed!");
	}

	void VulkanGraphicsBackend::Terminate()
	{
		_CommandManager.Free(_Data.Device.GetHandle());
		_DestroyVulkanDataUploader();
		_BindlessManager.Destroy(_BindlessCreateInfo);
		_DestroyVulkanImageDataManager();
		_DestroyFrameResources();
		_DestroyGeneralDescriptorPool();

		_BufferManager.Free(_Data.Device, _Data.Allocator);
		_DestroyVMAAllocator();

		_DestroySwapChainKHR();
		_DestroyLogicalDevice();

		_DestroySurfaceKHR();
		_DestroyDebugMessenger();
		_DestroyInstance();
		VULKAN_PRINTLN("Vulkan ShutDown!");
	}

	void VulkanGraphicsBackend::BeginFrame(uint32_t Index)
	{
		_FrameResource.CurrentFrameIndex = Index;
		_DescriptorSetsData.WritingBufferInfos.clear();
		_DescriptorSetsData.WritingImageInfos.clear();
		_DescriptorSetsData.WritingSets.clear();
	}

	void VulkanGraphicsBackend::EndFrame(const std::vector<GraphicsCommandBuffer>& CmdBuffers)
	{
		if (_Spec.ViewPortResized) {
			_ReCreateSwapChainKHR();
			return;
		}
		auto device = _Data.Device.GetHandle();

		// Create a Sorted Vector of render passe so like of only specific to rneder pass 0 then 1 etc 
		// Acqire Image
		vkWaitForFences(device, 1, &_FrameResource.InFlightFences[_FrameResource.CurrentFrameIndex], VK_TRUE, UINT64_MAX);
		vkResetFences(_Data.Device.GetHandle(), 1, &_FrameResource.InFlightFences[_FrameResource.CurrentFrameIndex]);
		VkResult result = vkAcquireNextImageKHR(_Data.Device.GetHandle(), _Data.SwapChainKHR.GetHandle(), UINT64_MAX,
			_FrameResource.ImageAvailableSemaphores[_FrameResource.CurrentFrameIndex], VK_NULL_HANDLE,
			&_FrameResource.CurrentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_Spec.ViewPortResized = true;
			_ReCreateSwapChainKHR();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		if (result != VK_SUCCESS)
		{
			std::cout << "ERROR: AcquireNextImage failed: " << result << "\n";
		}

		auto ActiveCommandBuffer = _CommandManager.Get(_FrameResource.CommandBuffers[_FrameResource.CurrentFrameIndex].CommandBufferHandle);
		_ActiveCommandBuffer = ActiveCommandBuffer;

		vkResetCommandBuffer(ActiveCommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		vkBeginCommandBuffer(ActiveCommandBuffer, &beginInfo);

		for (auto& CommandBuffer : CmdBuffers)
		{
			const std::vector<RenderCommandInfo>& RenderCommandInfos = CommandBuffer._RenderCommandInfos;
			const std::vector<CompiledPass>& RenderPasses = CommandBuffer._RenderPasses;
			const std::vector<PipelineStateInfo>& PipelineStates = CommandBuffer._PipelineStates;
			const std::vector< RenderShaderDataUpdateInfo>& ShaderUpdateInfoss = CommandBuffer._ShaderUpdateInfos;

			FillDescriptorSetInfos(ShaderUpdateInfoss);
			// This is the function that must be called after the loop completes
			vkUpdateDescriptorSets(
				device,
				static_cast<uint32_t>(_DescriptorSetsData.WritingSets.size()),
				_DescriptorSetsData.WritingSets.data(),
				0,
				nullptr // No copies are being performed here
			);
			uint32_t CurrentPassId = 0;
			_ActivePipelineState = PipelineStateInfo();

			for (auto& RenderPass : RenderPasses)
			{
				_PreparePassBarriers(ActiveCommandBuffer, RenderPass.PrePassBarriers);
				_BeginDynamicRendering(ActiveCommandBuffer, RenderPass.Info);
				_SetViewPortSize(ActiveCommandBuffer, RenderPass.Info.RenderArea.x, RenderPass.Info.RenderArea.y);
				_SetScissorSize(ActiveCommandBuffer, RenderPass.Info.RenderArea.x, RenderPass.Info.RenderArea.y);

				int ActivePipelineStateID = UINT32_MAX;
				int ActiveShaderProgram = UINT32_MAX;
				int ActiveVertexBufferID = UINT32_MAX;
				int ActiveIndexBufferID = UINT32_MAX;
				int ActiveMaterialID = UINT32_MAX;

				for (auto& RenderDrawInfo : RenderCommandInfos)
				{
					// If this draw command doesn't belong to this pass, skip it
					if (RenderDrawInfo.RenderState.RenderPassIndex != CurrentPassId)
						continue; // <--- Jumps to the next RenderDrawInfo

					if (ActivePipelineStateID != RenderDrawInfo.RenderState.PipelineStateID)
					{
						SetActiveGraphicsPipelineState(ActiveCommandBuffer, PipelineStates[RenderDrawInfo.RenderState.PipelineStateID]);
						ActivePipelineStateID = RenderDrawInfo.RenderState.PipelineStateID;
					}
					if (ActiveShaderProgram != RenderDrawInfo.RenderState.ShaderProgramID)
					{
						auto ActiveShaderInfo = _ShaderManager.GetProgramInfo(RenderDrawInfo.RenderState.ShaderProgramID);

						for (const auto& reflectedInput : ActiveShaderInfo.VertexInputs) {
							bool found = false;
							for (const auto& binding : _ActivePipelineState.VertexInputLayout.Bindings) {
								for (const auto& attr : binding.Attribs) {
									if (attr.Location == reflectedInput.Location) {
										// Validate Format Size
										if (ShaderTypeToSize(attr.Type) != ShaderTypeToSize(reflectedInput.Format)) {
											VULKAN_ERROR("Attribute at Location " << attr.Location << " size mismatch!");
										}
										found = true;
										break;
									}
								}
							}
							if (!found) VULKAN_ERROR("Shader expects input at Location " << reflectedInput.Location << " but none provided!");
						}

						_ShaderManager.BindShaderProgram(ActiveCommandBuffer, RenderDrawInfo.RenderState.ShaderProgramID);
						_ShaderManager.BindBindlessSets(ActiveCommandBuffer, RenderDrawInfo.RenderState.ShaderProgramID,
							_FrameResource.CurrentFrameIndex, _BindlessManager);

						ActiveShaderProgram = RenderDrawInfo.RenderState.ShaderProgramID;
					}

					if (ActiveMaterialID != RenderDrawInfo.RenderState.MaterialID)
					{
						auto ActiveMaterial = _MaterialManager.Get(RenderDrawInfo.RenderState.MaterialID);
						if (ActiveMaterial->ProgramID != ActiveShaderProgram)
							VULKAN_ERROR("Given Material Should be valid with bounded Shader!");
						_ShaderManager.BindUserSets(ActiveCommandBuffer, ActiveShaderProgram,
							ActiveMaterial->Sets[_FrameResource.CurrentFrameIndex]);
						ActiveMaterialID = RenderDrawInfo.RenderState.MaterialID;
					}

					if (_ActivePipelineState.ShaderCullMode != RenderDrawInfo.ShaderCullMode)
						SetCullMode(ActiveCommandBuffer, RenderDrawInfo.ShaderCullMode);

					if (_ActivePipelineState.ShaderFillMode != RenderDrawInfo.ShaderFillMode)
						SetPolygonMode(ActiveCommandBuffer, RenderDrawInfo.ShaderFillMode);

					if (_ActivePipelineState.FrontFace != RenderDrawInfo.FrontFace)
						SetFrontFace(ActiveCommandBuffer, RenderDrawInfo.FrontFace);

					if (_ActivePipelineState.LineWidth != RenderDrawInfo.LineWidth)
						SetLineWidth(ActiveCommandBuffer, RenderDrawInfo.LineWidth);

					if (_ActivePipelineState.RasterizerDiscardEnable != RenderDrawInfo.RasterizerDiscardEnable)
						SetRasterizerDiscardEnable(ActiveCommandBuffer, RenderDrawInfo.RasterizerDiscardEnable);

					if (_ActivePipelineState.DepthBiasEnable != RenderDrawInfo.DepthBiasEnable)
						SetDepthBiasEnable(ActiveCommandBuffer, RenderDrawInfo.DepthBiasEnable);

					if (_ActivePipelineState.DepthBiasConstantFactor != RenderDrawInfo.DepthBiasConstantFactor ||
						_ActivePipelineState.DepthBiasClamp != RenderDrawInfo.DepthBiasClamp ||
						_ActivePipelineState.DepthBiasSlopeFactor != RenderDrawInfo.DepthBiasSlopeFactor)
						SetDepthBias(
							ActiveCommandBuffer,
							RenderDrawInfo.DepthBiasConstantFactor,
							RenderDrawInfo.DepthBiasClamp,
							RenderDrawInfo.DepthBiasSlopeFactor
						);

					if (RenderDrawInfo.InlineUniformData.Block != nullptr)
					{
						_ShaderManager.PushConstants(ActiveCommandBuffer, ActiveShaderProgram,
							RenderDrawInfo.InlineUniformData.Stages,
							RenderDrawInfo.InlineUniformData.Block
							, MAX_INLINE_UNIFORM_DATA_SIZE,
							0);
					}

					bool DrawIndexed = false;

					if (RenderDrawInfo.RenderState.VertexBufferID != UINT32_MAX)
						if (ActiveVertexBufferID != RenderDrawInfo.RenderState.VertexBufferID)
						{
							// 🆕 Bind vertex buffer
							VkBuffer vertexBuffers[] = {
								_BufferManager.Get(RenderDrawInfo.RenderState.VertexBufferID)->Buffer };
							VkDeviceSize offsets[] = { 0 };
							vkCmdBindVertexBuffers(ActiveCommandBuffer, 0, 1, vertexBuffers, offsets);
							ActiveVertexBufferID = RenderDrawInfo.RenderState.VertexBufferID;
						}

					if (RenderDrawInfo.RenderState.IndexBufferID != UINT32_MAX)
					{
						if (ActiveIndexBufferID != RenderDrawInfo.RenderState.IndexBufferID)
						{
							vkCmdBindIndexBuffer(ActiveCommandBuffer,
								_BufferManager.Get(RenderDrawInfo.RenderState.IndexBufferID)->Buffer,
								0,
								VK_INDEX_TYPE_UINT32
							);
							ActiveIndexBufferID = RenderDrawInfo.RenderState.IndexBufferID;
							DrawIndexed = true;
						}
					}
					else if (ActiveIndexBufferID != UINT32_MAX)
					{
						vkCmdBindIndexBuffer(ActiveCommandBuffer,
							VK_NULL_HANDLE,
							0,
							VK_INDEX_TYPE_UINT32
						);
						ActiveIndexBufferID = UINT32_MAX;
						DrawIndexed = false;
					}

					if (!DrawIndexed && RenderDrawInfo.Type == CommandType::Draw)
						vkCmdDraw(ActiveCommandBuffer,
							RenderDrawInfo.ElementCount,
							RenderDrawInfo.InstanceCount,
							RenderDrawInfo.FirstElement,
							RenderDrawInfo.FirstInstance
						);
					else if (DrawIndexed && RenderDrawInfo.Type == CommandType::Draw)
						vkCmdDrawIndexed(ActiveCommandBuffer,
							RenderDrawInfo.ElementCount,
							RenderDrawInfo.InstanceCount,
							RenderDrawInfo.FirstElement,
							0,
							RenderDrawInfo.FirstInstance
						);
				}

				vkCmdEndRendering(ActiveCommandBuffer);
				CurrentPassId++;
			}
		}

		vkEndCommandBuffer(ActiveCommandBuffer);

		{
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { _FrameResource.ImageAvailableSemaphores[_FrameResource.CurrentFrameIndex] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &ActiveCommandBuffer;

			VkSemaphore signalSemaphores[] = { _FrameResource.RenderFinishedSemaphores[_FrameResource.CurrentFrameIndex] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			VULKAN_SUCCESS_ASSERT(vkQueueSubmit(_Data.Device.GetQueue(QueueFamilies::GRAPHICS), 1, &submitInfo, _FrameResource.InFlightFences[_FrameResource.CurrentFrameIndex]), "Failed to Submit Graphics Queue");
		}

		{

			VkSemaphore signalSemaphores[] = { _FrameResource.RenderFinishedSemaphores[_FrameResource.CurrentFrameIndex] };

			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;

			VkSwapchainKHR swapChains[] = { _Data.SwapChainKHR.GetHandle() };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &_FrameResource.CurrentImageIndex;
			presentInfo.pResults = nullptr; // Optional

			auto result = vkQueuePresentKHR(_Data.Device.GetQueue(QueueFamilies::PRESENT), &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
				_Spec.ViewPortResized = true;
			}
			else if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to present swap chain image!");
			}
		}
	}

	void VulkanGraphicsBackend::SetActiveGraphicsPipelineState(VkCommandBuffer CmdBuffer, const PipelineStateInfo& State)
	{
		VULKAN_ASSERT(CmdBuffer != VK_NULL_HANDLE, "Valid Command Buffer Not Given!");

		// Dynamic (changed on whenever user desires)
		vkCmdSetPrimitiveTopology(CmdBuffer, TopologyToVk(State.TopologyMode));

		SetCullMode(CmdBuffer, State.ShaderCullMode);
		SetFrontFace(CmdBuffer, State.FrontFace);
		SetPolygonMode(CmdBuffer, State.ShaderFillMode);
		SetLineWidth(CmdBuffer, State.LineWidth);
		SetRasterizerDiscardEnable(CmdBuffer, State.RasterizerDiscardEnable);

		SetDepthTestEnable(CmdBuffer, State.DepthTestEnable);
		SetDepthWriteEnable(CmdBuffer, State.DepthWriteEnable);
		SetDepthCompareOp(CmdBuffer, State.DepthCompareOp);

		SetStencilTestEnable(CmdBuffer, State.StencilTestEnable);

		SetStencilFrontOp(CmdBuffer, State.StencilFront);
		SetStencilBackOp(CmdBuffer, State.StencilBack);

		SetDepthBiasEnable(CmdBuffer, State.DepthBiasEnable);
		SetDepthBias(CmdBuffer, State.DepthBiasConstantFactor, State.DepthBiasClamp, State.DepthBiasSlopeFactor);

		SetAlphaToCoverageEnable(CmdBuffer, State.AlphaToCoverageEnable);

		// Static (Changed on New Pipelien state)
		SetPrimitiveRestartEnable(CmdBuffer, State.PrimitiveRestartEnable);

		SetColorBlendState(CmdBuffer, State.ColorBlendAttachments);

		SetRasterizationSamples(CmdBuffer, State.SampleCount); // Or your actual MSAA count

		uint32_t sampleMask = State.SampleMask & 0xFFFFFFFF;
		SetSampleMask(CmdBuffer, State.SampleCount, sampleMask);

		ChBool8 colorWriteEnables[] = { CH_TRUE };
		SetColorWriteEnable(CmdBuffer, 1, colorWriteEnables);

		SetVertexInputState(CmdBuffer, State.VertexInputLayout);

		_ActivePipelineState = State;
	}

	void VulkanGraphicsBackend::SetColorBlendState(VkCommandBuffer CmdBuffer, const std::vector<ColorBlendAttachmentState>& ColorBlendAttachments)
	{
		// The number of attachments being set dynamically
		uint32_t attachmentCount = (uint32_t)ColorBlendAttachments.size();

		// Arrays to hold the values for the simultaneous Vulkan calls

		auto blendEnables = (VkBool32*)alloca(attachmentCount * sizeof(VkBool32));
		auto colorWriteMasks = (VkColorComponentFlags*)alloca(attachmentCount
			* sizeof(VkColorComponentFlags));
		auto blendEquations = (VkColorBlendEquationEXT*)alloca(attachmentCount * sizeof(VkColorBlendEquationEXT));

		for (uint32_t i = 0; i < attachmentCount; ++i)
		{
			const auto& Attachment = ColorBlendAttachments[i];

			// 1. Blend Enable (VK_TRUE or VK_FALSE)
			blendEnables[i] = Attachment.BlendEnable ? VK_TRUE : VK_FALSE;

			// 2. Color Write Mask
			// Convert the simple 0xF flag to Vulkan's bitmask
			VkColorComponentFlags vkMask = 0;
			if (Attachment.ColorWriteMask & 0x1) vkMask |= VK_COLOR_COMPONENT_R_BIT;
			if (Attachment.ColorWriteMask & 0x2) vkMask |= VK_COLOR_COMPONENT_G_BIT;
			if (Attachment.ColorWriteMask & 0x4) vkMask |= VK_COLOR_COMPONENT_B_BIT;
			if (Attachment.ColorWriteMask & 0x8) vkMask |= VK_COLOR_COMPONENT_A_BIT;
			colorWriteMasks[i] = vkMask;

			// 3. Color Blend Equation (Requires VkColorBlendEquationEXT struct)
			VkColorBlendEquationEXT equation = {};
			equation.srcColorBlendFactor = BlendFactorToVk(Attachment.SrcColorFactor);
			equation.dstColorBlendFactor = BlendFactorToVk(Attachment.DstColorFactor);
			equation.colorBlendOp = BlendOpToVk(Attachment.ColorBlendOp);
			equation.srcAlphaBlendFactor = BlendFactorToVk(Attachment.SrcAlphaFactor);
			equation.dstAlphaBlendFactor = BlendFactorToVk(Attachment.DstAlphaFactor);
			equation.alphaBlendOp = BlendOpToVk(Attachment.AlphaBlendOp);
			blendEquations[i] = equation;
		}

		// --- Vulkan Command Calls (Setting the Dynamic State) ---
		uint32_t firstAttachment = 0; // Assuming we always start at slot 0

		// 1. Set Blend Enable for all attachments
		pfn_vkCmdSetColorBlendEnableEXT(CmdBuffer, firstAttachment, attachmentCount, blendEnables);

		// 2. Set Color Write Mask for all attachments
		pfn_vkCmdSetColorWriteMaskEXT(CmdBuffer, firstAttachment, attachmentCount, colorWriteMasks);

		// 3. Set Blend Equation for all attachments
		// This command covers the factors (Src, Dst) and the operators (Add, Subtract, etc.)
		pfn_vkCmdSetColorBlendEquationEXT(CmdBuffer, firstAttachment, attachmentCount, blendEquations);
	}

	void VulkanGraphicsBackend::SetPrimitiveTopology(VkCommandBuffer CmdBuffer, InputTopologyMode mode)
	{
		if (_ActivePipelineState.TopologyMode != mode)
		{
			// vkCmdSetPrimitiveTopology requires VK_EXT_extended_dynamic_state2
			vkCmdSetPrimitiveTopology(CmdBuffer, TopologyToVk(mode));
			_ActivePipelineState.TopologyMode = mode;
		}
	}

	void VulkanGraphicsBackend::SetPrimitiveRestartEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.PrimitiveRestartEnable != enable)
		{
			// vkCmdSetPrimitiveRestartEnable requires VK_EXT_extended_dynamic_state2
			vkCmdSetPrimitiveRestartEnable(CmdBuffer, bEnable);
			_ActivePipelineState.PrimitiveRestartEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetCullMode(VkCommandBuffer CmdBuffer, CullMode mode)
	{
		if (_ActivePipelineState.ShaderCullMode != mode)
		{
			// vkCmdSetCullMode requires VK_EXT_extended_dynamic_state
			vkCmdSetCullMode(CmdBuffer, CullModeToVk(mode));
			_ActivePipelineState.ShaderCullMode = mode;
		}
	}

	void VulkanGraphicsBackend::SetFrontFace(VkCommandBuffer CmdBuffer, FrontFaceMode mode)
	{
		if (_ActivePipelineState.FrontFace != mode)
		{
			// vkCmdSetFrontFace requires VK_EXT_extended_dynamic_state
			vkCmdSetFrontFace(CmdBuffer, FrontFaceToVk(mode));
			_ActivePipelineState.FrontFace = mode;
		}
	}

	void VulkanGraphicsBackend::SetPolygonMode(VkCommandBuffer CmdBuffer, PolygonMode mode)
	{
		if (_ActivePipelineState.ShaderFillMode != mode)
		{
			// pfn_vkCmdSetPolygonModeEXT requires VK_EXT_extended_dynamic_state3
			pfn_vkCmdSetPolygonModeEXT(CmdBuffer, PolygonModeToVk(mode));
			_ActivePipelineState.ShaderFillMode = mode;
		}
	}

	void VulkanGraphicsBackend::SetLineWidth(VkCommandBuffer CmdBuffer, float width)
	{
		if (_ActivePipelineState.LineWidth != width)
		{
			// vkCmdSetLineWidth is core dynamic state
			vkCmdSetLineWidth(CmdBuffer, width);
			_ActivePipelineState.LineWidth = width;
		}
	}

	void VulkanGraphicsBackend::SetRasterizerDiscardEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.RasterizerDiscardEnable != enable)
		{
			// vkCmdSetRasterizerDiscardEnable requires VK_EXT_extended_dynamic_state3
			vkCmdSetRasterizerDiscardEnable(CmdBuffer, bEnable);
			_ActivePipelineState.RasterizerDiscardEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetDepthBiasEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.DepthBiasEnable != enable)
		{
			// vkCmdSetDepthBiasEnable requires VK_EXT_extended_dynamic_state
			vkCmdSetDepthBiasEnable(CmdBuffer, bEnable);
			_ActivePipelineState.DepthBiasEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetDepthBias(VkCommandBuffer CmdBuffer, float constantFactor, float clamp, float slopeFactor)
	{
		// vkCmdSetDepthBias is core dynamic state
		if (_ActivePipelineState.DepthBiasConstantFactor != constantFactor ||
			_ActivePipelineState.DepthBiasClamp != clamp ||
			_ActivePipelineState.DepthBiasSlopeFactor != slopeFactor)
		{
			vkCmdSetDepthBias(CmdBuffer, constantFactor, clamp, slopeFactor);
			_ActivePipelineState.DepthBiasConstantFactor = constantFactor;
			_ActivePipelineState.DepthBiasClamp = clamp;
			_ActivePipelineState.DepthBiasSlopeFactor = slopeFactor;
		}
	}

	void VulkanGraphicsBackend::SetDepthTestEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.DepthTestEnable != enable)
		{
			// vkCmdSetDepthTestEnable requires VK_EXT_extended_dynamic_state
			vkCmdSetDepthTestEnable(CmdBuffer, bEnable);
			_ActivePipelineState.DepthTestEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetDepthWriteEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.DepthWriteEnable != enable)
		{
			// vkCmdSetDepthWriteEnable requires VK_EXT_extended_dynamic_state
			vkCmdSetDepthWriteEnable(CmdBuffer, bEnable);
			_ActivePipelineState.DepthWriteEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetDepthCompareOp(VkCommandBuffer CmdBuffer, CompareOp op)
	{
		if (_ActivePipelineState.DepthCompareOp != op)
		{
			// vkCmdSetDepthCompareOp requires VK_EXT_extended_dynamic_state
			vkCmdSetDepthCompareOp(CmdBuffer, CompareOpToVk(op));
			_ActivePipelineState.DepthCompareOp = op;
		}
	}

	void VulkanGraphicsBackend::SetStencilTestEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.StencilTestEnable != enable)
		{
			// vkCmdSetStencilTestEnable requires VK_EXT_extended_dynamic_state
			vkCmdSetStencilTestEnable(CmdBuffer, bEnable);
			_ActivePipelineState.StencilTestEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetStencilFrontOp(VkCommandBuffer CmdBuffer, const StencilOpState& state)
	{
		// NOTE: Requires a full comparison if StencilOpState doesn't have an operator==
		if (state != _ActivePipelineState.StencilFront)
		{
			// vkCmdSetStencilOp requires VK_EXT_extended_dynamic_state
			vkCmdSetStencilOp(CmdBuffer, VK_STENCIL_FACE_FRONT_BIT,
				StencilOpToVulkan(state.FailOp),
				StencilOpToVulkan(state.PassOp),
				StencilOpToVulkan(state.DepthFailOp),
				CompareOpToVk(state.CompareFunction));

			_ActivePipelineState.StencilFront = state;
		}
	}

	void VulkanGraphicsBackend::SetStencilBackOp(VkCommandBuffer CmdBuffer, const StencilOpState& state)
	{
		// NOTE: Requires a full comparison if StencilOpState doesn't have an operator==
		if (state != _ActivePipelineState.StencilBack)
		{
			// vkCmdSetStencilOp requires VK_EXT_extended_dynamic_state
			vkCmdSetStencilOp(CmdBuffer, VK_STENCIL_FACE_BACK_BIT,
				StencilOpToVulkan(state.FailOp),
				StencilOpToVulkan(state.PassOp),
				StencilOpToVulkan(state.DepthFailOp),
				CompareOpToVk(state.CompareFunction));

			_ActivePipelineState.StencilBack = state;
		}
	}

	void VulkanGraphicsBackend::SetAlphaToCoverageEnable(VkCommandBuffer CmdBuffer, ChBool8 enable)
	{
		bool bEnable = ChBoolToBool(enable);
		if (_ActivePipelineState.AlphaToCoverageEnable != enable)
		{
			// pfn_vkCmdSetAlphaToCoverageEnableEXT requires VK_EXT_extended_dynamic_state3
			pfn_vkCmdSetAlphaToCoverageEnableEXT(CmdBuffer, bEnable);
			_ActivePipelineState.AlphaToCoverageEnable = enable;
		}
	}

	void VulkanGraphicsBackend::SetVertexInputState(VkCommandBuffer CmdBuffer, const VertexInputShaderLayout& Layout)
	{
		size_t BindingCount = Layout.Bindings.size();

		VkVertexInputBindingDescription2EXT* BindingDescriptions = (VkVertexInputBindingDescription2EXT*)alloca(
			sizeof(VkVertexInputBindingDescription2EXT) * BindingCount
		);

		size_t AttribCount = 0;

		for (auto& Bindings : Layout.Bindings)
			AttribCount += Bindings.Attribs.size();

		VkVertexInputAttributeDescription2EXT* AttribDescriptions = (VkVertexInputAttributeDescription2EXT*)alloca(
			sizeof(VkVertexInputAttributeDescription2EXT) * AttribCount
		);

		int ioffset = 0;

		for (int i = 0; i < BindingCount; i++)
		{
			// 1. Define empty descriptions
			VkVertexInputBindingDescription2EXT bindingDescriptions{};
			bindingDescriptions.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT;
			bindingDescriptions.binding = Layout.Bindings[i].BindingIndex;
			bindingDescriptions.divisor = 1;
			bindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			bindingDescriptions.stride = Layout.Bindings[i].Stride;

			BindingDescriptions[i] = bindingDescriptions;

			for (int x = 0; x < Layout.Bindings[i].Attribs.size(); x++)
			{
				VkVertexInputAttributeDescription2EXT attributeDescriptions{};
				attributeDescriptions.sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT;
				attributeDescriptions.binding = Layout.Bindings[i].BindingIndex;
				attributeDescriptions.location = Layout.Bindings[i].Attribs[x].Location;
				attributeDescriptions.format = ShaderObjectTypeToVkFormat(Layout.Bindings[i].Attribs[x].Type);
				attributeDescriptions.offset = Layout.Bindings[i].Attribs[x].Offset;
				AttribDescriptions[ioffset] = attributeDescriptions;
				ioffset++;
			}
		}

		// 2. Call the dynamic command with zero counts
		pfn_vkCmdSetVertexInputEXT(
			CmdBuffer,
			BindingCount, // bindingCount must be 0
			BindingDescriptions,
			AttribCount, // attributeCount must be 0
			AttribDescriptions
		);

	}

	void VulkanGraphicsBackend::SetSampleMask(VkCommandBuffer CmdBuffer, uint32_t sampleCount,
		uint32_t sampleMask)
	{
		if (_ActivePipelineState.SampleMask != sampleMask ||
			_ActivePipelineState.SampleCount != sampleCount)
		{
			VkSampleMask vkMask = sampleMask & 0xFFFFFFFF;

			// Requires VK_EXT_extended_dynamic_state3
			pfn_vkCmdSetSampleMaskEXT(
				CmdBuffer,
				SampleCountToVk(sampleCount),
				&vkMask
			);

			_ActivePipelineState.SampleMask = sampleMask;
			_ActivePipelineState.SampleCount = sampleCount;
		}
	}

	void VulkanGraphicsBackend::SetRasterizationSamples(VkCommandBuffer CmdBuffer, uint32_t sampleCount)
	{
		if (_ActivePipelineState.SampleCount != sampleCount)
		{
			// Requires VK_EXT_extended_dynamic_state3
			pfn_vkCmdSetRasterizationSamplesEXT(
				CmdBuffer,
				SampleCountToVk(sampleCount)
			);

			_ActivePipelineState.SampleCount = sampleCount;
		}
	}

	void VulkanGraphicsBackend::FillDescriptorSetInfos(const std::vector<RenderShaderDataUpdateInfo>& Info)
	{
		// Reserve to prevent pointer invalidation during push_back
		_DescriptorSetsData.WritingBufferInfos.reserve(Info.size());
		_DescriptorSetsData.WritingImageInfos.reserve(Info.size());
		_DescriptorSetsData.WritingSets.reserve(Info.size());

		for (const auto& UpdateInfo : Info) {
			// 2. Fetch Material & Shader Metadata
			auto* ActiveMaterial = _MaterialManager.Get(UpdateInfo.MaterialHandle);
			const auto& ProgramInfo = _ShaderManager.GetProgramInfo(ActiveMaterial->ProgramID);

			// 3. Find the binding slot using your 32-char fixed name
			// Internally this uses memcmp(Name, UpdateInfo.Name, 32)
			auto BindingLocation = _ShaderManager.FindUserShaderDataLocation(
				ActiveMaterial->ProgramID,
				UpdateInfo.Name
			);

			if (!BindingLocation.ReturnState) {
				VULKAN_ERROR("Uniform '%s' not found in shader!" << UpdateInfo.Name);
				continue;
			}

			uint32_t SetIdx = BindingLocation.BindingInfo.Set - int(BindlessSetTypes::USER_0);
			uint32_t BindingSlot = BindingLocation.BindingInfo.Binding;

			// 4. Ensure Cache Capacity (No-op after first run per material)
			auto& CurrentSetCache = ActiveMaterial->SetBindingsStates[_FrameResource.CurrentFrameIndex][SetIdx];
			if (CurrentSetCache.size() <= BindingSlot) {
				CurrentSetCache.resize(ProgramInfo.UniformInputs[SetIdx].Bindings.size(), UINT32_MAX);
			}
			bool Skip = false;

			// ----- Uniform / Storage Buffer -----
			if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::UNIFORM_BUFFER ||
				BindingLocation.BindingInfo.Type == ShaderUniformTypes::STORAGE_BUFFER)
			{
				if ((CurrentSetCache[BindingSlot] & 0xFFFFFFFF) == UpdateInfo.BufferInfo.Handle) {
					Skip = true;
				}
				else {
					// update cache
					CurrentSetCache[BindingSlot] = UpdateInfo.BufferInfo.Handle;
				}
			}

			// ----- Sampled Image -----
			else if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::SAMPLED_IMAGE)
			{
				if ((CurrentSetCache[BindingSlot] & 0xFFFFFFFF) == UpdateInfo.ImageInfo.Handle) {
					Skip = true;
				}
				else {
					CurrentSetCache[BindingSlot] = UpdateInfo.ImageInfo.Handle;
				}
			}

			// ----- Sampler -----
			else if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::SAMPLER)
			{
				if ((CurrentSetCache[BindingSlot] & 0xFFFFFFFF) == UpdateInfo.ImageInfo.Sampler) {
					Skip = true;
				}
				else {
					CurrentSetCache[BindingSlot] = UpdateInfo.ImageInfo.Sampler;
				}
			}

			// ----- Combined Image + Sampler -----
			else if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::COMBINED_IMAGE_SAMPLER)
			{
				uint64_t combined = (uint64_t(UpdateInfo.ImageInfo.Sampler) << 32) |
					uint64_t(UpdateInfo.ImageInfo.Handle);

				if ((CurrentSetCache[BindingSlot] & 0xFFFFFFFF) == UpdateInfo.ImageInfo.Handle &&
					(CurrentSetCache[BindingSlot] >> 32) == UpdateInfo.ImageInfo.Sampler) {
					Skip = true;
				}
				else {
					CurrentSetCache[BindingSlot] = combined;
				}
			}

			if (Skip)
				continue;

			// 6. Build Vulkan Write Structure
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = ActiveMaterial->Sets[_FrameResource.CurrentFrameIndex][SetIdx];
			descriptorWrite.dstBinding = BindingSlot;
			descriptorWrite.dstArrayElement = UpdateInfo.DstArrayIndex;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = ShaderUniformTypeToVk(BindingLocation.BindingInfo.Type);

			// 7. Handle Buffer Types (Uniform / Storage)
			if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::UNIFORM_BUFFER ||
				BindingLocation.BindingInfo.Type == ShaderUniformTypes::STORAGE_BUFFER)
			{
				if (UpdateInfo.BufferInfo.Handle == UINT32_MAX) continue;

				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = _BufferManager.Get(UpdateInfo.BufferInfo.Handle)->Buffer;
				bufferInfo.offset = UpdateInfo.BufferInfo.Offset;
				bufferInfo.range = UpdateInfo.BufferInfo.Range;

				// Push to transient storage and link pointer
				_DescriptorSetsData.WritingBufferInfos.push_back(bufferInfo);
				descriptorWrite.pBufferInfo = &_DescriptorSetsData.WritingBufferInfos.back();
			}

			// 7. Handle Buffer Types (Uniform / Storage)
			if (BindingLocation.BindingInfo.Type == ShaderUniformTypes::SAMPLED_IMAGE ||
				BindingLocation.BindingInfo.Type == ShaderUniformTypes::SAMPLER ||
				BindingLocation.BindingInfo.Type == ShaderUniformTypes::COMBINED_IMAGE_SAMPLER)
			{

				VkDescriptorImageInfo ImageInfo{};
				ImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				ImageInfo.sampler = VK_NULL_HANDLE;
				ImageInfo.imageView = VK_NULL_HANDLE;

				if (UpdateInfo.ImageInfo.Handle != UINT32_MAX) {
					auto ActiveTexture = _ImageDataManager.GetTexture(UpdateInfo.ImageInfo.Handle);
					ImageInfo.imageView = ActiveTexture->GetHandle();
					ImageInfo.imageLayout = _ImageDataManager.GetImage(ActiveTexture->GetImageHandle())->GetImageLayout();
				}
				if (UpdateInfo.ImageInfo.Sampler != UINT32_MAX)
				{
					ImageInfo.sampler = _SamplerSet.Get(UpdateInfo.ImageInfo.Sampler)->GetHandle();
				}
				// Push to transient storage and link pointer
				_DescriptorSetsData.WritingImageInfos.push_back(ImageInfo);
				descriptorWrite.pImageInfo = &_DescriptorSetsData.WritingImageInfos.back();
			}

			// 8. Queue for GPU Update & Mark Cache
			_DescriptorSetsData.WritingSets.push_back(descriptorWrite);
			//CurrentSetCache[BindingSlot] = UpdateInfo.BufferInfo.Handle;
		}
	}

	uint32_t VulkanGraphicsBackend::PrepareMaterialData(uint32_t ShaderProgramHandle)
	{
		// --- 1. Initialize Material Info Structure ---
		VulkanBackendMaterialInfo Info;
		const uint32_t framesInFlight = _Spec.MaxFrameInFlight;
		Info.Sets.resize(framesInFlight);
		Info.ProgramID = ShaderProgramHandle;

		// Retrieve all set layouts defined by the shader program
		// This returns a map/vector where the key/index is the set number (0, 1, 2...)
		const auto& programSetLayouts = _ShaderManager.GetProgramSetLayouts(ShaderProgramHandle);

		// If the shader uses no sets (e.g., uses push constants only), there's nothing to allocate
		if (programSetLayouts.empty()) {
			_MaterialManager.Create(Info);
			return UINT32_MAX;
		}

		// --- 2. Nested Loop for Allocation (Frame-in-Flight & Set Type) ---
		// Outer loop: Iterates over each frame-in-flight index (0, 1, 2...)

		Info.SetBindingsStates.resize(_Spec.MaxFrameInFlight);
		for (size_t frameIndex = 0; frameIndex < framesInFlight; ++frameIndex)
		{
			int idx = 0;
			// Inner loop: Iterates over each SET defined by the shader program
			// (e.g., Set 0, Set 1, Set 2, which map to the indices of your VulkanBackendMaterialInfo::Sets array)
			for (const auto& layoutHandle : programSetLayouts)
			{
				if (layoutHandle == VK_NULL_HANDLE) {
					idx++;
					continue;
				}

				// SetLayoutIndex is the 'Set' number (e.g., 0, 1, 2)
				// layoutHandle is the VkDescriptorSetLayout for that set

				// 3. Prepare Allocation Info
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

				// Use your general descriptor pool (must be large enough for all descriptors)
				allocInfo.descriptorPool = this->_GeneralDescriptorPool;

				// We allocate one set at a time
				allocInfo.descriptorSetCount = 1;

				// Use the correct VkDescriptorSetLayout from the program's layouts
				allocInfo.pSetLayouts = &layoutHandle;

				// NOTE: pNext is generally NOT used here unless you are dealing 
				// with variable descriptor count bindings (which would require the extension struct).
				allocInfo.pNext = nullptr;

				// 4. Allocate Set
				VkDescriptorSet newSet;

				// Check for success
				if (vkAllocateDescriptorSets(_Data.Device.GetHandle(), &allocInfo, &newSet) != VK_SUCCESS) {
					// You should probably use a logger here, not throw std::runtime_error
					std::string error = "Failed to allocate descriptor set for frame " + std::to_string(frameIndex) + " and set index " + std::to_string(idx) + "!";
					throw std::runtime_error(error);
				}

				// 5. Store the Allocated Set in the Material Structure
				// Map the program's set number (setLayoutIndex) to your internal array index.
				// Assuming BindlessSetTypes::USER_0 is the offset for the first user set.

				if (idx < Info.Sets[frameIndex].size()) {
					Info.Sets[frameIndex][idx] = newSet;
				}
				else {
					// Handle case where program set index exceeds the size of your fixed array
				}
				idx++;
			}
		}

		// 6. Insert the fully prepared material data into the manager
		uint32_t ID = _MaterialManager.Create(Info);
		_BindlessManager.PrepareForMaterial(ID);
		return ID;
	}

	void VulkanGraphicsBackend::ClearMaterialData(uint32_t RawMaterialHandle)
	{
		auto Mat = _MaterialManager.Get(RawMaterialHandle);

		std::vector<VkDescriptorSet> ToClearSets;
		for (auto& Sets : Mat->Sets)
			for (auto& EachSet : Sets)
				if (EachSet != nullptr)
					ToClearSets.push_back(EachSet);

		vkFreeDescriptorSets(_Data.Device.GetHandle(), _GeneralDescriptorPool,
			(uint32_t)ToClearSets.size(), ToClearSets.data());

		_MaterialManager.Destroy(RawMaterialHandle);
	}

	uint32_t VulkanGraphicsBackend::CreateSampler(const SamplerSpec& Spec)
	{
		VulkanSampler Sampler;
		Sampler.Init(_Data.Device.GetHandle(), _Data.Device.GetPhysicalDevice()->Info.properties.limits.maxSamplerAnisotropy,
			Spec);

		auto Handle = _SamplerSet.Create(Sampler);
		_BindlessManager.PrepareForSampler(Handle);
		return Handle;
	}

	void VulkanGraphicsBackend::DestroySampler(uint32_t SamplerHandle)
	{
		_SamplerSet.Get(SamplerHandle)->Destroy(_Data.Device.GetHandle());
		_SamplerSet.Destroy(SamplerHandle);
	}

	uint32_t VulkanGraphicsBackend::GetTextureShaderIndex(uint32_t RawTextureHandle)
	{
		return _BindlessManager.UpdateTexture(_Data.Device.GetHandle(), RawTextureHandle,
			_ImageDataManager.GetTexture(RawTextureHandle)->GetHandle(),
			_ImageDataManager.GetImage(_ImageDataManager.GetTexture(RawTextureHandle)->GetImageHandle())->GetImageLayout());
	}

	uint32_t VulkanGraphicsBackend::GetSamplerShaderIndex(uint32_t RawSamplerHandle)
	{
		return _BindlessManager.UpdateSampler(_Data.Device.GetHandle(), RawSamplerHandle
			, _SamplerSet.Get(RawSamplerHandle)->GetHandle());
	}

	uint32_t VulkanGraphicsBackend::GetMaterialShaderIndex(uint32_t RawMaterialHandle)
	{
		return _BindlessManager.GetMaterialShaderIndex(RawMaterialHandle);
	}

	void VulkanGraphicsBackend::SetColorWriteEnable(VkCommandBuffer CmdBuffer, size_t Count, ChBool8* States)
	{
		//if (cachedValue != value)
		{
			VkBool32* Enables = (VkBool32*)alloca(sizeof(VkBool32) * Count);
			for (int i = 0; i < Count; i++)
			{
				Enables[i] = ChBoolToBool(States[i]);
			}

			// Requires VK_EXT_color_write_enable
			pfn_vkCmdSetColorWriteEnableEXT(
				CmdBuffer,
				Count,
				Enables
			);
		}
	}

	uint32_t VulkanGraphicsBackend::AllocateBuffer(const BufferCreateInfo& Info)
	{
		BufferCreateInfo CreateInfo = Info;
		// Inside your Buffer Allocation logic
		uint32_t finalSize = Info.SizeInBytes;

		if (Info.Type & BUFFER_TYPE_UNIFORM)
		{
			// Fetch this from DeviceProperties.limits.minUniformBufferOffsetAlignment
			uint32_t minAlignment = _Data.Device.GetPhysicalDevice()->Info.properties.limits.minUniformBufferOffsetAlignment;
			finalSize = (finalSize + minAlignment - 1) & ~(minAlignment - 1);
		}
		else if (Info.Type & BUFFER_TYPE_STORAGE)
		{
			// Fetch this from DeviceProperties.limits.minStorageBufferOffsetAlignment
			uint32_t minAlignment = _Data.Device.GetPhysicalDevice()->Info.properties.limits.minStorageBufferOffsetAlignment;
			finalSize = (finalSize + minAlignment - 1) & ~(minAlignment - 1);
		}
		CreateInfo.SizeInBytes = finalSize;

		return _BufferManager.Create(_Data.Device, _Data.Allocator, CreateInfo);
	}

	void VulkanGraphicsBackend::MapBufferData(uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset)
	{
		_BufferManager.MapBufferData(_Data.Device, BufferHandle, Data, Size, Offset);
	}

	void VulkanGraphicsBackend::FreeBuffer(uint32_t BufferHandle)
	{
		_BufferManager.Destroy(_Data.Allocator, BufferHandle);
	}

#pragma region CoreInitials
	void VulkanGraphicsBackend::_CreateInstance()
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

	void Chilli::VulkanGraphicsBackend::_DestroyInstance()
	{
		vkDestroyInstance(_Data.Instance, nullptr);
		VULKAN_PRINTLN("Instance ShutDown!");
	}

	void Chilli::VulkanGraphicsBackend::_CreateDebugMessenger()
	{
		if (!_Spec.EnableValidation)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		_PopulateDebugMessengerCreateInfo(createInfo);

		VULKAN_SUCCESS_ASSERT(CreateDebugUtilsMessengerEXT(_Data.Instance, &createInfo, nullptr, &_Data.DebugMessenger), "Debug Messenger not loaded!");
		VULKAN_PRINTLN("Debug Messenger Loaded!");
	}

	void Chilli::VulkanGraphicsBackend::_PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional
		createInfo.pNext = nullptr;     // Optional
		createInfo.flags = 0;           // Must be 0
	}

	void Chilli::VulkanGraphicsBackend::_DestroyDebugMessenger()
	{
		if (_Spec.EnableValidation)
		{
			DestroyDebugUtilsMessengerEXT(_Data.Instance, _Data.DebugMessenger, nullptr);
			VULKAN_PRINTLN("Debug Messenger ShutDown!");
		}
	}

	void Chilli::VulkanGraphicsBackend::_CreateSurfaceKHR()
	{
		glfwCreateWindowSurface(_Data.Instance, (GLFWwindow*)_Spec.WindowSurfaceHandle, nullptr, &_Data.SurfaceKHR);
		VULKAN_PRINTLN("Window Surface KHR Created!");
	}

	void Chilli::VulkanGraphicsBackend::_DestroySurfaceKHR()
	{
		vkDestroySurfaceKHR(_Data.Instance, _Data.SurfaceKHR, nullptr);
		VULKAN_PRINTLN("Surface KHR ShutDown!");
	}

	void VulkanGraphicsBackend::_CreatePhysicalDevice(std::vector<const char*>& DeviceExtensions)
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

		DeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
		DeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
		DeviceExtensions.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
		DeviceExtensions.push_back(VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);
		DeviceExtensions.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);

		for (auto& device : _Data.PhysicalDevices)
		{
			if (CheckVulkanPhysicalDeviceExtensions(device, DeviceExtensions) == false)
				device.Info.SupportsExtensions = false;
		}

		_FindSuitablePhysicalDevice();
	}

	void VulkanGraphicsBackend::_FindSuitablePhysicalDevice()
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
			if (deviceinfo.SupportsExtensions)
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

	void VulkanGraphicsBackend::_CreateLogicalDevice(std::vector<const char*>& DeviceExtensions)
	{
		_Data.Device.Init(&_Data.PhysicalDevices[_Data.ActivePhysicalDeviceIndex], DeviceExtensions);
	}

	void VulkanGraphicsBackend::_DestroyLogicalDevice()
	{
		_Data.Device.Destroy();
		VULKAN_PRINTLN("Device ShutDown!");
	}

	void VulkanGraphicsBackend::_CreateSwapChainKHR()
	{
		_Data.SwapChainKHR.Init(_Data.Device, _Data.SurfaceKHR, _Spec.ViewPortSize.x, _Spec.ViewPortSize.y, _Spec.VSync);
		_Spec.ViewPortResized = false;
	}

	void VulkanGraphicsBackend::_ReCreateSwapChainKHR()
	{
		if (_Spec.ViewPortResized)
			_Data.SwapChainKHR.Recreate(_Data.Device, _Data.SurfaceKHR, _Spec.ViewPortSize.x, _Spec.ViewPortSize.y, _Spec.VSync);

		_Spec.ViewPortResized = false;
	}

	void VulkanGraphicsBackend::_DestroySwapChainKHR()
	{
		_Data.SwapChainKHR.Destroy(_Data.Device);
		VULKAN_PRINTLN("Swap Chain Destroy!");

	}

	void VulkanGraphicsBackend::_CreateVMAAllocator()
	{
		VmaAllocatorCreateInfo Info{};
		Info.device = _Data.Device.GetHandle();
		Info.physicalDevice = _Data.Device.GetPhysicalDevice()->PhysicalDevice;
		Info.instance = _Data.Instance;
		vmaCreateAllocator(&Info, &_Data.Allocator);
		VULKAN_PRINTLN("VMA Allocator Created!");
	}

	void VulkanGraphicsBackend::_DestroyVMAAllocator()
	{
		vmaDestroyAllocator(_Data.Allocator);
		VULKAN_PRINTLN("VMA Allocator Destroyed!");
	}

	void VulkanGraphicsBackend::_CreateFrameResources()
	{
		// Create Synchronization Resoources
		_FrameResource.ImageAvailableSemaphores.resize(_Spec.MaxFrameInFlight);
		_FrameResource.RenderFinishedSemaphores.resize(_Spec.MaxFrameInFlight);
		_FrameResource.InFlightFences.resize(_Spec.MaxFrameInFlight);
		auto device = _Data.Device.GetHandle();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < _Spec.MaxFrameInFlight; i++) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_FrameResource.ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_FrameResource.RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &_FrameResource.InFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}

		// Create Command Buffers
		_FrameResource.CommandBuffers = _CommandManager.AllocateCommandBuffers(device, CommandBufferPurpose::GRAPHICS, _Spec.MaxFrameInFlight);
	}

	void VulkanGraphicsBackend::_DestroyFrameResources()
	{
		auto device = _Data.Device.GetHandle();
		for (size_t i = 0; i < _Spec.MaxFrameInFlight; i++) {
			vkDestroySemaphore(device, _FrameResource.RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, _FrameResource.ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, _FrameResource.InFlightFences[i], nullptr);
		}
	}

	void VulkanGraphicsBackend::_BeginDynamicRendering(VkCommandBuffer CmdBuffer, const RenderPassInfo& Pass)
	{
		VkRenderingInfo RenderingInfo{};
		RenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.layerCount = 1;

		std::vector< VkRenderingAttachmentInfo> ColorAttachments;
		ColorAttachments.reserve(Pass.ColorAttachments.size());

		RenderingInfo.renderArea = { {0, 0}, {(unsigned int)Pass.RenderArea.x
			,(unsigned int)Pass.RenderArea.y} };

		if (Pass.ColorAttachments.size() > 0)
		{
			for (int i = 0; i < Pass.ColorAttachments.size(); i++)
			{
				auto& ActiveColorAttachmentInfo = Pass.ColorAttachments[i];

				VkRenderingAttachmentInfo colorAttachment{};
				colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //  Now correct
				colorAttachment.storeOp = StoreOpToVk(ActiveColorAttachmentInfo.StoreOp);
				colorAttachment.loadOp = LoadOpToVk(ActiveColorAttachmentInfo.LoadOp);
				colorAttachment.clearValue = { {ActiveColorAttachmentInfo.ClearColor.x,
					ActiveColorAttachmentInfo.ClearColor.y, ActiveColorAttachmentInfo.ClearColor.z,
					ActiveColorAttachmentInfo.ClearColor.w} };

				// Render To SwapChain Image
				if (ActiveColorAttachmentInfo.UseSwapChainImage)
				{
					RenderingInfo.renderArea.extent = {
						min((uint32_t)Pass.RenderArea.x, _Data.SwapChainKHR.GetExtent().width),
						min((uint32_t)Pass.RenderArea.y, _Data.SwapChainKHR.GetExtent().height)
					};
					colorAttachment.imageView = _Data.SwapChainKHR.GetImageViews()[_FrameResource.CurrentImageIndex];
				}
				ColorAttachments.push_back(colorAttachment);
			}
		}
		RenderingInfo.pDepthAttachment = nullptr;

		RenderingInfo.colorAttachmentCount = static_cast<uint32_t>(ColorAttachments.size());
		RenderingInfo.pColorAttachments = ColorAttachments.data();

		vkCmdBeginRendering(CmdBuffer, &RenderingInfo);
	}

	void VulkanGraphicsBackend::_PreparePassBarriers(VkCommandBuffer CmdBuffer, const std::vector<PipelineBarrier>& PrePassBarriers)
	{
		// Use a local vector or clear the static one to prevent data leaking between calls
		static std::vector<VkImageMemoryBarrier2> vkBarriers;
		vkBarriers.clear();
		vkBarriers.reserve(PrePassBarriers.size());

		for (const auto& b : PrePassBarriers) {
			VkImageMemoryBarrier2 vkb = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

			auto sync = GetVulkanState(b.NewState);
			auto prevSync = GetVulkanState(b.OldState);

			// Fix: Use the image at CurrentImageIndex for swapchain barriers
			if (b.IsSwapchain) {
				vkb.image = _Data.SwapChainKHR.GetImages()[_FrameResource.CurrentImageIndex];
				_Data.SwapChainKHR.SetImageLayout(_FrameResource.CurrentImageIndex, sync.layout);
			}
			else {
				auto Image = _ImageDataManager.GetImage(_ImageDataManager.GetTexture(b.RawTextureHandle)->GetImageHandle());
				vkb.image = Image->GetHandle();
				Image->SetImageLayout(sync.layout);
			}

			vkb.srcStageMask = prevSync.stage;
			vkb.srcAccessMask = prevSync.access;
			vkb.dstStageMask = sync.stage;
			vkb.dstAccessMask = sync.access;
			vkb.oldLayout = prevSync.layout;
			vkb.newLayout = sync.layout;

			// Handle Aspect Mask (Depth vs Color)
			vkb.subresourceRange.aspectMask = (b.NewState == ResourceState::DepthWrite) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			vkb.subresourceRange.baseMipLevel = 0;
			vkb.subresourceRange.levelCount = 1;
			vkb.subresourceRange.baseArrayLayer = 0;
			vkb.subresourceRange.layerCount = 1;

			vkBarriers.push_back(vkb);
		}

		if (!vkBarriers.empty()) {
			VkDependencyInfo depInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
			depInfo.imageMemoryBarrierCount = (uint32_t)vkBarriers.size();
			depInfo.pImageMemoryBarriers = vkBarriers.data();
			vkCmdPipelineBarrier2(CmdBuffer, &depInfo);
		}
	}

	void VulkanGraphicsBackend::_SetViewPortSize(VkCommandBuffer CmdBuffer, int Width, int Height)
	{
		// Set dynamic viewport (matches pipeline dynamic state)
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(Height);
		viewport.width = static_cast<float>(Width);
		viewport.height = -static_cast<float>(Height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewportWithCount(CmdBuffer, 1, &viewport);
	}

	void VulkanGraphicsBackend::_SetScissorSize(VkCommandBuffer CmdBuffer, int Width, int Height)
	{
		// Set dynamic scissor
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent.width = Width;
		scissor.extent.height = Height;
		vkCmdSetScissorWithCount(CmdBuffer, 1, &scissor);
	}

	void VulkanGraphicsBackend::_CreateGeneralDescriptorPool()
	{
		// --- 1. Define Pool Sizes (Capacity Planning) ---
		std::vector<VkDescriptorPoolSize> poolSizes = {
			// Uniform Buffer (UBO) Descriptors
			// We assume an average of 1 UBO per user set.
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _Data.MaxSets * 1 },

			// Storage Buffer (SSBO) Descriptors
			// We assume an average of 1 SSBO per user set.
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, _Data.MaxSets * 1 },

			// Combined Image Sampler (Texture/Sampler) Descriptors
			// We assume an average of 4 texture bindings per material/set.
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, _Data.MaxSets * 4 }

			// Add other types (e.g., Input Attachments, Separate Images, etc.) if needed
		};

		// --- 2. Define Pool Creation Info ---
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr; // No extensions used here

		// Set the flag to allow freeing individual sets (useful for runtime material destruction)
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		// The total number of sets the pool can allocate
		poolInfo.maxSets = _Data.MaxSets;

		// The types and total count for each descriptor type
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		vkCreateDescriptorPool(_Data.Device.GetHandle(), &poolInfo, nullptr, &_GeneralDescriptorPool);
	}

	void VulkanGraphicsBackend::_DestroyGeneralDescriptorPool()
	{
		if (_GeneralDescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(_Data.Device.GetHandle(), _GeneralDescriptorPool, nullptr);
		}
	}

	void VulkanGraphicsBackend::_CreateVulkanDataUploader()
	{
		auto StagingBufferCmd = _CommandManager.AllocateCommandBuffer(_Data.Device.GetHandle(), CommandBufferPurpose::TRANSFER);
		auto GraphicsBufferCmd = _CommandManager.AllocateCommandBuffer(_Data.Device.GetHandle(), CommandBufferPurpose::GRAPHICS);
		_Uploader.Init(&_Data.Device, _Data.Allocator,
			_CommandManager.Get(StagingBufferCmd.CommandBufferHandle),
			_CommandManager.Get(GraphicsBufferCmd.CommandBufferHandle));
	}

	void VulkanGraphicsBackend::_DestroyVulkanDataUploader()
	{
		_Uploader.Destroy();
	}

	void VulkanGraphicsBackend::_CreateVulkanImageDataManager()
	{
		VulkanImageDataManagerSpec ImageManagerSpec;
		ImageManagerSpec.Device = &_Data.Device;
		ImageManagerSpec.Uploader = &_Uploader;
		ImageManagerSpec.GetBuffer = [&](uint32_t id) {
			return this->_BufferManager.Get(id)->Buffer;
			};
		ImageManagerSpec.AllocateBuffer = [&](const BufferCreateInfo& info) {
			return this->AllocateBuffer(info);
			};

		ImageManagerSpec.FreeBuffer = [&](uint32_t id) {
			this->FreeBuffer(id);
			};

		ImageManagerSpec.MapBufferData = [&](uint32_t BufferHandle, void* Data, uint32_t Size, uint32_t Offset) {
			this->MapBufferData(BufferHandle, Data, Size, Offset);
			};
		_ImageDataManager.Init(ImageManagerSpec);
	}

	void VulkanGraphicsBackend::_DestroyVulkanImageDataManager()
	{
		_ImageDataManager.Destroy();
	}

#pragma endregion

	void _ShaderReadFile(const char* FilePath, std::vector<char>& CharData)
	{
		std::ifstream file(FilePath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open file!");

		size_t fileSize = (size_t)file.tellg();
		CharData.resize(fileSize);

		file.seekg(0);
		file.read(CharData.data(), fileSize);
		file.close();
	}

	inline uint64_t HashSPIRV(const std::vector<char>& code)
	{
		uint64_t hash = 14695981039346656037ull; // FNV offset
		for (auto c : code)
		{
			hash ^= static_cast<uint64_t>(c);
			hash *= 1099511628211ull; // FNV prime
		}
		return hash;
	}

	// Optional: combine with file path for safety
	inline uint64_t HashShaderModule(const std::string& path, const std::vector<char>& code)
	{
		uint64_t h = HashSPIRV(code);
		for (auto c : path)
		{
			h ^= static_cast<uint64_t>(c);
			h *= 1099511628211ull;
		}
		return h;
	}

	ShaderModule VulkanGraphicsBackend::CreateShaderModule(const char* FilePath, ShaderStageType Type)
	{
		ShaderModule Module;
		Module.Path = std::string(FilePath);
		Module.Stage = Type;

		std::vector<char> Code;
		_ShaderReadFile(FilePath, Code);

		Module.RawModuleHandle = _ShaderManager.CreateShaderModule(_Data.Device.GetHandle(), FilePath, Type);
		Module.BinaryHash = HashShaderModule(Module.Path, Code);

		// 3️ Extract the file name if valid
		std::filesystem::path p(Module.Path);

		if (p.has_extension()) // valid shader path
		{
			Module.Name = p.stem().string(); // filename without extension
			// optional: convert to lowercase if you want uniform names
			std::transform(Module.Name.begin(), Module.Name.end(), Module.Name.begin(),
				[](unsigned char c) { return std::tolower(c); });
		}
		else
		{
			// fallback if no extension
			Module.Name = Module.Path;
		}

		return Module;
	}

	void Chilli::VulkanGraphicsBackend::DestroyShaderModule(const ShaderModule& Module)
	{
		if (Module.RawModuleHandle == BackBone::npos)
			VULKAN_ERROR("No Valid Raw Module Handle Given!");
		_ShaderManager.DestroyShaderModule(_Data.Device.GetHandle(), Module.RawModuleHandle);
	}

	void VulkanGraphicsBackend::PrepareForShutDown()
	{
		vkDeviceWaitIdle(_Data.Device.GetHandle());
	}

#pragma region VulkanCommandManager
	VkCommandPool __CreateVkCommandPool(VkDevice device, uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags Flags)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = Flags;

		VkCommandPool commandPool;
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool!");

		return commandPool;
	}

	VkCommandPool __CreateVkDeviceCommandPool(const VulkanDevice& Device, QueueFamilies Family)
	{
		VkCommandPool commandPool;
		commandPool = __CreateVkCommandPool(Device.GetHandle(),
			Device.GetPhysicalDevice()->Info.QueueIndicies.Queues[int(Family)].value(),
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		return commandPool;
	}

	void VulkanCommandManager::Init(const VulkanDevice& Device)
	{
		_Pools[int(QueueFamilies::GRAPHICS)] = __CreateVkDeviceCommandPool(Device, QueueFamilies::GRAPHICS);
		_Pools[int(QueueFamilies::PRESENT)] = __CreateVkDeviceCommandPool(Device, QueueFamilies::PRESENT);
		_Pools[int(QueueFamilies::COMPUTE)] = __CreateVkDeviceCommandPool(Device, QueueFamilies::COMPUTE);
		_Pools[int(QueueFamilies::TRANSFER)] = __CreateVkDeviceCommandPool(Device, QueueFamilies::TRANSFER);
	}

	void VulkanCommandManager::Free(VkDevice Device)
	{
		vkDestroyCommandPool(Device, _Pools[int(QueueFamilies::GRAPHICS)], nullptr);
		vkDestroyCommandPool(Device, _Pools[int(QueueFamilies::PRESENT)], nullptr);
		vkDestroyCommandPool(Device, _Pools[int(QueueFamilies::COMPUTE)], nullptr);
		vkDestroyCommandPool(Device, _Pools[int(QueueFamilies::TRANSFER)], nullptr);
	}

	CommandBufferAllocInfo VulkanCommandManager::AllocateCommandBuffer(VkDevice Device, CommandBufferPurpose Purpose)
	{
		return AllocateCommandBuffers(Device, Purpose, 1)[0];
	}

	std::vector<CommandBufferAllocInfo> VulkanCommandManager::AllocateCommandBuffers(VkDevice Device, CommandBufferPurpose Purpose, uint32_t Count)
	{
		// Allocate a temporary command buffer
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (Purpose == CommandBufferPurpose::GRAPHICS)
			allocInfo.commandPool = _Pools[int(QueueFamilies::GRAPHICS)];
		if (Purpose == CommandBufferPurpose::TRANSFER)
			allocInfo.commandPool = _Pools[int(QueueFamilies::TRANSFER)];
		if (Purpose == CommandBufferPurpose::PRESENT)
			allocInfo.commandPool = _Pools[int(QueueFamilies::PRESENT)];
		if (Purpose == CommandBufferPurpose::COMPUTE)
			allocInfo.commandPool = _Pools[int(QueueFamilies::COMPUTE)];

		allocInfo.commandBufferCount = Count;

		std::vector<VkCommandBuffer> Cmds;
		Cmds.resize(Count);
		vkAllocateCommandBuffers(Device, &allocInfo, Cmds.data());

		std::vector<CommandBufferAllocInfo> AllocInfos;

		for (int i = 0; i < Count; i++)
		{
			CommandBufferAllocInfo AllocInfo;
			AllocInfo.Purpose = Purpose;
			AllocInfo.CommandBufferHandle = _Buffers.Create(Cmds[i]);

			AllocInfos.push_back(AllocInfo);
		}

		return AllocInfos;
	}
#pragma endregion

	void VulkanDataUploader::Init(VulkanDevice* Device, VmaAllocator Allocator, VkCommandBuffer CommandBufferHandle, VkCommandBuffer GraphicsCmdBuffer)
	{
		_Device = Device;
		_Allocator = Allocator;
		_CmdBuffer = CommandBufferHandle;
		_GraphicsCmdBuffer = GraphicsCmdBuffer;
		// Create Fence
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VULKAN_SUCCESS_ASSERT(vkCreateFence(Device->GetHandle(), &fenceInfo, nullptr, &_TransferFence),
			"Failed To Create Fence");
		VULKAN_SUCCESS_ASSERT(vkCreateFence(Device->GetHandle(), &fenceInfo, nullptr, &_GraphicsFence),
			"Failed To Create Fence");
	}

	void VulkanDataUploader::Destroy()
	{
		vkDestroyFence(_Device->GetHandle(), _TransferFence, nullptr);
		vkDestroyFence(_Device->GetHandle(), _GraphicsFence, nullptr);
		_Device = VK_NULL_HANDLE;
	}

	void VulkanDataUploader::BeginBatching(CommandBufferPurpose Purpose)
	{
		auto VkCmdHandle = _CmdBuffer;

		if (Purpose == CommandBufferPurpose::GRAPHICS)
			VkCmdHandle = _GraphicsCmdBuffer;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkResetCommandBuffer(VkCmdHandle, 0);
		if (vkBeginCommandBuffer(VkCmdHandle, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		_BatchRecording = CH_TRUE;
		_ActiveCmdBuffer = VkCmdHandle;
	}

	void VulkanDataUploader::EndBatching()
	{
		auto device = _Device->GetHandle();

		vkEndCommandBuffer(_ActiveCmdBuffer);
		vkResetFences(device, 1, &_TransferFence);

		VkSubmitInfo Submit{};
		Submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		Submit.commandBufferCount = 1;
		Submit.pCommandBuffers = &_ActiveCmdBuffer;

		auto Queue = _Device->GetQueue(QueueFamilies::TRANSFER);
		if (_ActiveCmdBuffer == _GraphicsCmdBuffer)
			Queue = _Device->GetQueue(QueueFamilies::GRAPHICS);

		VULKAN_SUCCESS_ASSERT(vkQueueSubmit(Queue, 1, &Submit, _TransferFence),
			"TRANSFER QUEUE FAILED TO SUBMIT!");
		vkWaitForFences(device, 1, &_TransferFence, VK_TRUE, UINT64_MAX);
		_BatchRecording = CH_NONE;
		_ActiveCmdBuffer = VK_NULL_HANDLE;
	}

	void VulkanDataUploader::CopyBufferToBuffer(VkBuffer Src, VkBuffer Dst, const BufferCopyInfo& Info)
	{
		auto VkCmdHandle = _CmdBuffer;

		bool DoOneTimeBatching = true;
		if (_BatchRecording != ChBool8(CH_NONE))
			DoOneTimeBatching = false;

		if (DoOneTimeBatching)
			BeginBatching();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = Info.SrcOffset; // Offset in bytes from the start of srcBuffer
		copyRegion.dstOffset = Info.DstOffset; // Offset in bytes from the start of dstBuffer
		copyRegion.size = Info.Size; // Size of the data to copy in bytes

		vkCmdCopyBuffer(VkCmdHandle, Src,
			Dst, 1, &copyRegion);

		if (DoOneTimeBatching)
			EndBatching();
	}

	void VulkanDataUploader::CopyBufferToImage(VkBuffer Src, VkImage Dst, const VkBufferImageCopy& Copy)
	{
		auto VkCmdHandle = _CmdBuffer;

		bool DoOneTimeBatching = true;
		if (_BatchRecording != CH_NONE)
			DoOneTimeBatching = false;

		if (DoOneTimeBatching)
			BeginBatching();

		vkCmdCopyBufferToImage(
			VkCmdHandle,
			Src,
			Dst,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&Copy
		);

		if (DoOneTimeBatching)
			EndBatching();
	}

	void VulkanDataUploader::CopyImageToBuffer(VkImage Src, VkBuffer Dst, const VkBufferImageCopy& Copy)
	{
		auto VkCmdHandle = _CmdBuffer;

		bool DoOneTimeBatching = true;
		if (_BatchRecording != CH_NONE)
			DoOneTimeBatching = false;

		if (DoOneTimeBatching)
			BeginBatching();

		vkCmdCopyImageToBuffer(
			VkCmdHandle,
			Src,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			Dst,
			1,
			&Copy
		);

		if (DoOneTimeBatching)
			EndBatching();
	}

	void VulkanDataUploader::CopyImageToImage(VkImage Src, VkImage Dst, const VkImageCopy& Copy)
	{
		auto VkCmdHandle = _CmdBuffer;

		bool DoOneTimeBatching = true;
		if (_BatchRecording != CH_NONE)
			DoOneTimeBatching = false;

		if (DoOneTimeBatching)
			BeginBatching();

		vkCmdCopyImage(
			VkCmdHandle,
			Src,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			Dst,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&Copy
		);

		if (DoOneTimeBatching)
			EndBatching();
	}

	void VulkanDataUploader::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectFlags)
	{
		bool doOneTime = (_BatchRecording == CH_NONE);
		if (doOneTime)
			BeginBatching(CommandBufferPurpose::GRAPHICS);

		// Call your generic helper that records the barrier
		_TransitionImageLayout(
			_ActiveCmdBuffer,
			image,
			oldLayout,
			newLayout,
			aspectFlags
		);

		if (doOneTime)
			EndBatching();
	}
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
	VkPipelineStageFlags2 srcStage = 0;
	VkPipelineStageFlags2 dstStage = 0;
	VkAccessFlags2 srcAccess = 0;
	VkAccessFlags2 dstAccess = 0;

	// Auto-detect stages based on layout transition
	// --- UNDEFINED to various layouts ---
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
	{
		srcStage = VK_PIPELINE_STAGE_2_NONE;
		srcAccess = 0;

		if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			dstAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			dstAccess = VK_ACCESS_2_SHADER_READ_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			dstStage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
			dstAccess = VK_ACCESS_2_MEMORY_READ_BIT;
		}
		else if (newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			dstStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			dstAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		}
	}
	// --- TRANSFER_DST to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		srcAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		dstAccess = VK_ACCESS_2_SHADER_READ_BIT;
	}
	// --- COLOR_ATTACHMENT to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		dstAccess = VK_ACCESS_2_SHADER_READ_BIT;
	}
	// --- DEPTH_STENCIL_ATTACHMENT to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		srcAccess = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		dstAccess = VK_ACCESS_2_SHADER_READ_BIT;
	}
	// --- SHADER_READ_ONLY to COLOR_ATTACHMENT ---
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		srcAccess = VK_ACCESS_2_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	}
	// --- COLOR_ATTACHMENT to PRESENT_SRC ---
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		srcStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		dstAccess = VK_ACCESS_2_MEMORY_READ_BIT;
	}
	// --- PRESENT_SRC to COLOR_ATTACHMENT ---
	else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		srcAccess = 0;
		dstStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	}
	// --- SHADER_READ_ONLY to TRANSFER_SRC ---
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		srcAccess = VK_ACCESS_2_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		dstAccess = VK_ACCESS_2_TRANSFER_READ_BIT;
	}
	// --- SHADER_READ_ONLY to TRANSFER_DST ---
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		srcAccess = VK_ACCESS_2_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		dstAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	}
	// --- GENERAL to SHADER_READ_ONLY ---
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		srcAccess = VK_ACCESS_2_MEMORY_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		dstAccess = VK_ACCESS_2_SHADER_READ_BIT;
	}
	// --- Add more as needed ---
	else
	{
		// Fallback: safe but conservative
		srcStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		srcAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		dstAccess = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
	}

	// Use Vulkan 1.3 barrier if available
	if (vkCmdPipelineBarrier2)
	{
		VkImageMemoryBarrier2 barrier2{};
		barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier2.srcStageMask = srcStage;
		barrier2.srcAccessMask = srcAccess;
		barrier2.dstStageMask = dstStage;
		barrier2.dstAccessMask = dstAccess;
		barrier2.oldLayout = oldLayout;
		barrier2.newLayout = newLayout;
		barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier2.image = image;
		barrier2.subresourceRange.aspectMask = aspectFlags;
		barrier2.subresourceRange.baseMipLevel = 0;
		barrier2.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier2.subresourceRange.baseArrayLayer = 0;
		barrier2.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier2;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}
	else
	{
		// Fallback to Vulkan 1.0-1.2 style (convert flags)
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = aspectFlags;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		// Convert Vulkan 1.3 flags to 1.0-1.2 (simplified)
		barrier.srcAccessMask = static_cast<VkAccessFlags>(srcAccess);
		barrier.dstAccessMask = static_cast<VkAccessFlags>(dstAccess);

		VkPipelineStageFlags srcStageOld = static_cast<VkPipelineStageFlags>(srcStage);
		VkPipelineStageFlags dstStageOld = static_cast<VkPipelineStageFlags>(dstStage);

		// If flags are too new, use safe defaults
		if (srcStageOld == 0) srcStageOld = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		if (dstStageOld == 0) dstStageOld = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStageOld, dstStageOld,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
}
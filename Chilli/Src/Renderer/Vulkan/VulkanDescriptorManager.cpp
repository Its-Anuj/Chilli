#include "ChV_PCH.h"
#include "vulkan\vulkan.h"

#include "vk_mem_alloc.h"
#include "VulkanBackend.h"
#include "VulkanDescriptorManager.h"
#include "VulkanConversions.h"

namespace Chilli
{
	VkDescriptorPoolSize __CreatePoolSize(ShaderUniformTypes Type, uint32_t Count)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = UniformTypeToVkDescriptorType(Type);
		poolSize.descriptorCount = static_cast<uint32_t>(Count);
		return poolSize;
	}

	VkDescriptorSetLayoutBinding __CreateSetLayoutBinding(uint32_t Binding, ShaderUniformTypes Type,
		uint32_t DesCount, VkShaderStageFlags ShaderFlags, VkSampler* pImmutableSamplers)
	{
		// Create Layout
		VkDescriptorSetLayoutBinding BindingInfo{};
		BindingInfo.binding = Binding;
		BindingInfo.descriptorType = UniformTypeToVkDescriptorType(Type);
		BindingInfo.descriptorCount = DesCount;
		BindingInfo.stageFlags = ShaderFlags;
		BindingInfo.pImmutableSamplers = pImmutableSamplers; // Optional

		return BindingInfo;
	}

	VkDescriptorSetLayout __CreateSetLayout(VkDevice Device, uint32_t BindingCount, VkDescriptorSetLayoutBinding* pBindings, void* pNext = nullptr)
	{
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = BindingCount;
		layoutInfo.pBindings = pBindings;
		layoutInfo.pNext = pNext;
		VkDescriptorSetLayout Layout;

		if (vkCreateDescriptorSetLayout(Device, &layoutInfo, nullptr, &Layout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}

		return Layout;
	}

	void VulkanBindlessSetManager::Init(VulkanBindlessSetManagerCreateInfo& Info)
	{
		_SetupManagerSetLayouts(Info);
		_SetupBindlessSetManagerSets(Info);

		for (auto& BufferHandle : _SetBuffers)
			BufferHandle = SparseSet<uint32_t>::npos;
		_SetupManagerBuffers(Info);
		_WriteBindlessSetManagerSets(Info);

		_CreateInfo = Info;
	}

	void VulkanBindlessSetManager::Destroy(VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto device = Info.Device->GetHandle();
			vkDestroyDescriptorPool(device, _BindlessPool, nullptr);

		for (auto& BufferHandle : _SetBuffers)
			if (BufferHandle != SparseSet<uint32_t>::npos)
				Info.FreeBuffer(BufferHandle);

		for (auto Layout : _SetLayouts)
			if (Layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(device, Layout, nullptr);
	}

	void VulkanBindlessSetManager::WriteBindlessTextures(VkDevice device, VkImageView Tex, uint32_t Index)
	{
		for (size_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++) {
			{
				VkDescriptorImageInfo ImageInfo{};
				ImageInfo.imageView = Tex;
				ImageInfo.sampler = VK_NULL_HANDLE;
				ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::TEX_SAMPLERS)];
				descriptorWrite.dstBinding = 1;
				descriptorWrite.dstArrayElement = Index;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::SAMPLED_IMAGE);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = nullptr;
				descriptorWrite.pImageInfo = &ImageInfo; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
		}
	}

	void VulkanBindlessSetManager::WriteBindlessSamplers(VkDevice device, VkSampler Sampler, uint32_t Index)
	{
		for (size_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++) {
			{
				VkDescriptorImageInfo ImageInfo{};
				ImageInfo.imageView = nullptr;
				ImageInfo.sampler = Sampler;

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::TEX_SAMPLERS)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = Index;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::SAMPLER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = nullptr;
				descriptorWrite.pImageInfo = &ImageInfo; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
		}
	}

	void VulkanBindlessSetManager::_SetupManagerSetLayouts(VulkanBindlessSetManagerCreateInfo& Info)
	{
		std::vector<VkDescriptorPoolSize> PoolSizes = {
			__CreatePoolSize(ShaderUniformTypes::UNIFORM_BUFFER,
				BindlessRenderingLimits::MAX_UNIFORM_BUFFERS),
			__CreatePoolSize(ShaderUniformTypes::SAMPLER,
				int(BindlessRenderingLimits::MAX_SAMPLERS)),

			__CreatePoolSize(ShaderUniformTypes::STORAGE_BUFFER,
				BindlessRenderingLimits::MAX_STORAGE_BUFFERS),

			__CreatePoolSize(ShaderUniformTypes::SAMPLED_IMAGE,
				BindlessRenderingLimits::MAX_TEXTURES),
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
		poolInfo.pPoolSizes = PoolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(BindlessSetTypes::COUNT) * Info.MaxFrameInFlight;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; // Important for bindless!

		auto device = Info.Device->GetHandle();

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr
			, &_BindlessPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		_Sets.resize(Info.MaxFrameInFlight);
		for (auto& SetArray : _Sets)
			for (auto& Set : SetArray)
				Set = VK_NULL_HANDLE;
		for (auto& Layout : _SetLayouts)
			Layout = VK_NULL_HANDLE;

		// Create Layouts and Sets
		{
			// Global Set - 0 Binding 0
			auto GlobalUBOLayoutBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::UNIFORM_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			// Global Set - 0 Binding 1
			auto SceneUBOLayoutBinding = __CreateSetLayoutBinding(1, ShaderUniformTypes::UNIFORM_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			VkDescriptorSetLayoutBinding Bindings[] = { GlobalUBOLayoutBinding, SceneUBOLayoutBinding };

			_SetLayouts[int(BindlessSetTypes::GLOBAL_SCENE)] = __CreateSetLayout(
				device, 2, Bindings);
		} {
			// Tex Sampler Set - CORRECT ORDER for bindless
			// Binding 0: Samplers (fixed count)
			auto SamplerBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::SAMPLER,
				int(BindlessRenderingLimits::MAX_SAMPLERS), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			// Binding 1: Textures (variable count - MUST BE HIGHEST BINDING)
			auto TextureBinding = __CreateSetLayoutBinding(1, ShaderUniformTypes::SAMPLED_IMAGE,
				int(BindlessRenderingLimits::MAX_TEXTURES), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			VkDescriptorSetLayoutBinding Bindings[] = { SamplerBinding, TextureBinding }; // REVERSED ORDER!

			// ---- Binding flags ----
			VkDescriptorBindingFlags bindingFlags[2] = {};
			bindingFlags[0] = 0; // No flags for samplers (binding 0)
			bindingFlags[1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |  // for bindless textures (binding 1)
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

			VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
			bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
			bindingFlagsInfo.bindingCount = 2;
			bindingFlagsInfo.pBindingFlags = bindingFlags;

			_SetLayouts[int(BindlessSetTypes::TEX_SAMPLERS)] = __CreateSetLayout(
				device, 2, Bindings, &bindingFlagsInfo);
		}

		{
			// Material Set - 0 Binding 0
			auto MaterialBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::STORAGE_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			_SetLayouts[int(BindlessSetTypes::MATERIAl)] = __CreateSetLayout(
				device, 1, &MaterialBinding, nullptr);
		}
		{
			// Object Set - 0 Binding 0
			auto ObjectBinding = __CreateSetLayoutBinding(0, ShaderUniformTypes::STORAGE_BUFFER,
				1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr);

			_SetLayouts[int(BindlessSetTypes::PER_OBJECT)] = __CreateSetLayout(
				device, 1, &ObjectBinding, nullptr);

		}
	}

	void VulkanBindlessSetManager::_SetupManagerBuffers(VulkanBindlessSetManagerCreateInfo& Info)
	{
		// Global Buffers
		size_t alignment = Info.Device->GetPhysicalDevice()->Info.properties.limits.minUniformBufferOffsetAlignment;
		{
			size_t rawSize = sizeof(GlobalShaderData);
			_GlobalBufferAlignedSize = (rawSize + alignment - 1) & ~(alignment - 1);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = _GlobalBufferAlignedSize * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_STREAM;
			BufferInfo.Type = BUFFER_TYPE_UNIFORM;

			_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Scene Buffers
			size_t rawSize = sizeof(SceneShaderData);
			_SceneBufferAlignedSize = (rawSize + alignment - 1) & ~(alignment - 1);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = _SceneBufferAlignedSize * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_STREAM;
			BufferInfo.Type = BUFFER_TYPE_UNIFORM;

			_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Material Buffers
			constexpr size_t rawSize = sizeof(MaterialShaderData);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = rawSize * CH_MATERIAL_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_STREAM;
			BufferInfo.Type = BUFFER_TYPE_STORAGE;

			_SetBuffers[int(BindlessSetTypes::MATERIAl)] = Info.AllocateBuffer(BufferInfo);
		}
		{
			// Material Buffers
			constexpr size_t rawSize = sizeof(ObjectShaderData);

			BufferCreateInfo BufferInfo{};
			BufferInfo.Data = nullptr;
			BufferInfo.SizeInBytes = rawSize * CH_OBJECT_SHADER_DATA_AMOUNT * Info.MaxFrameInFlight;
			BufferInfo.State = BufferState::DYNAMIC_STREAM;
			BufferInfo.Type = BUFFER_TYPE_STORAGE;

			_SetBuffers[int(BindlessSetTypes::PER_OBJECT)] = Info.AllocateBuffer(BufferInfo);
		}
	}
	void VulkanBindlessSetManager::_SetupBindlessSetManagerSets(VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto device = Info.Device->GetHandle();
		{
			// Global Scene Set - 0
			std::vector<VkDescriptorSetLayout> layouts(Info.MaxFrameInFlight, _SetLayouts[int(BindlessSetTypes::GLOBAL_SCENE)]);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _BindlessPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(Info.MaxFrameInFlight);
			allocInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> DescSets;
			DescSets.resize(Info.MaxFrameInFlight);
			if (vkAllocateDescriptorSets(device, &allocInfo, DescSets.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor sets!");
			}
			int idx = 0;
			for (auto& Set : _Sets)
				Set[int(BindlessSetTypes::GLOBAL_SCENE)] = DescSets[idx++];
		}
		{
			// Global Scene Set - 0
			std::vector<uint32_t> variableCounts(Info.MaxFrameInFlight, BindlessRenderingLimits::MAX_TEXTURES);

			VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
			variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
			variableCountInfo.descriptorSetCount = Info.MaxFrameInFlight;
			variableCountInfo.pDescriptorCounts = variableCounts.data();

			std::vector<VkDescriptorSetLayout> layouts(Info.MaxFrameInFlight, _SetLayouts[int(BindlessSetTypes::TEX_SAMPLERS)]);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _BindlessPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(Info.MaxFrameInFlight);
			allocInfo.pSetLayouts = layouts.data();
			allocInfo.pNext = &variableCountInfo;

			std::vector<VkDescriptorSet> DescSets;
			DescSets.resize(Info.MaxFrameInFlight);
			if (vkAllocateDescriptorSets(device, &allocInfo, DescSets.data()) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate descriptor sets!");
			int idx = 0;
			for (auto& Set : _Sets)
				Set[int(BindlessSetTypes::TEX_SAMPLERS)] = DescSets[idx++];
		}
		{
			// Material Scene Set - 0
			std::vector<VkDescriptorSetLayout> layouts(Info.MaxFrameInFlight, _SetLayouts[int(BindlessSetTypes::MATERIAl)]);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _BindlessPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(Info.MaxFrameInFlight);
			allocInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> DescSets;
			DescSets.resize(Info.MaxFrameInFlight);
			if (vkAllocateDescriptorSets(device, &allocInfo, DescSets.data()) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate descriptor sets!");
			int idx = 0;
			for (auto& Set : _Sets)
				Set[int(BindlessSetTypes::MATERIAl)] = DescSets[idx++];
		}
		{
			// Material Scene Set - 0
			std::vector<VkDescriptorSetLayout> layouts(Info.MaxFrameInFlight, _SetLayouts[int(BindlessSetTypes::PER_OBJECT)]);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _BindlessPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(Info.MaxFrameInFlight);
			allocInfo.pSetLayouts = layouts.data();

			std::vector<VkDescriptorSet> DescSets;
			DescSets.resize(Info.MaxFrameInFlight);
			if (vkAllocateDescriptorSets(device, &allocInfo, DescSets.data()) != VK_SUCCESS)
				throw std::runtime_error("failed to allocate descriptor sets!");
			int idx = 0;
			for (auto& Set : _Sets)
				Set[int(BindlessSetTypes::PER_OBJECT)] = DescSets[idx++];
		}
	}

	void VulkanBindlessSetManager::_WriteBindlessSetManagerSets(VulkanBindlessSetManagerCreateInfo& Info)
	{
		auto device = Info.Device->GetHandle();
		for (size_t i = 0; i < Info.MaxFrameInFlight; i++) {
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)]);
				bufferInfo.offset = i * _GlobalBufferAlignedSize;
				bufferInfo.range = sizeof(GlobalShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::GLOBAL_SCENE)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::UNIFORM_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)]);
				bufferInfo.offset = i * _SceneBufferAlignedSize;
				bufferInfo.range = sizeof(SceneShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::GLOBAL_SCENE)];
				descriptorWrite.dstBinding = 1;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::UNIFORM_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::MATERIAl)]);
				bufferInfo.offset = i * CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);
				bufferInfo.range = CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::MATERIAl)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::STORAGE_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = Info.GetBuffer(_SetBuffers[int(BindlessSetTypes::PER_OBJECT)]);
				bufferInfo.offset = i * CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);
				bufferInfo.range = CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = _Sets[i][int(BindlessSetTypes::PER_OBJECT)];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;

				descriptorWrite.descriptorType = UniformTypeToVkDescriptorType(ShaderUniformTypes::STORAGE_BUFFER);
				descriptorWrite.descriptorCount = 1;

				descriptorWrite.pBufferInfo = &bufferInfo;
				descriptorWrite.pImageInfo = nullptr; // Optional
				descriptorWrite.pTexelBufferView = nullptr; // Optional

				vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
			}
		}
	}

	SparseSet<uint32_t>& VulkanBindlessSetManager::GetMap(BindlessSetTypes Type)
	{
		if (int(Type) >= int(BindlessSetTypes::COUNT_NON_USER))
			VULKAN_ERROR("Given Type Must be below COUNT_NON_USER");
		return _Maps[int(Type)];
	}

	void VulkanBindlessSetManager::UpdateGlobalShaderData(const GlobalShaderData& Data)
	{
		for (int i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			_CreateInfo.MapBufferData(_SetBuffers[int(BindlessSetTypes::GLOBAL_SCENE)],
				(void*)&Data,
				sizeof(GlobalShaderData),
				i * _GlobalBufferAlignedSize);
		}
	}

	void VulkanBindlessSetManager::UpdateSceneShaderData(const SceneShaderData& Data)
	{
		for (int i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			_CreateInfo.MapBufferData(_SetBuffers[int(BindlessSetTypes::SCENE_BUFFER_USAGE)],
				(void*)&Data,
				sizeof(SceneShaderData),
				i * _SceneBufferAlignedSize);
		}
	}

	void VulkanBindlessSetManager::UpdateMaterialShaderData(const Material& Mat, int MaterialID)
	{
		MaterialShaderData Data;
		Data.Indicies.x = *GetTextureMap().Get(Mat.AlbedoTextureHandle.Handle);
		Data.Indicies.y = *GetSamplerMap().Get(Mat.AlbedoSamplerHandle.Handle);
		Data.AlbedoColor = Mat.AlbedoColor;

		const uint32_t materialOffset = MaterialID * sizeof(MaterialShaderData);
		for (uint32_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			const uint32_t frameOffset =
				i * CH_MATERIAL_SHADER_DATA_AMOUNT * sizeof(MaterialShaderData);

			_CreateInfo.MapBufferData(
				_SetBuffers[int(BindlessSetTypes::MATERIAl)],
				&Data,
				sizeof(MaterialShaderData),
				frameOffset + materialOffset
			);
		}
	}

	// USe Entity Handle for Objects
	void VulkanBindlessSetManager::UpdateObjectShaderData(const glm::mat4& Transformation, int ObjectID)
	{
		ObjectShaderData Data;
		Data.TransformationMat = Transformation;
		Data.InverseTransformationMat = glm::inverse(Transformation);

		const uint32_t ObjectOffset = ObjectID * sizeof(ObjectShaderData);
		for (uint32_t i = 0; i < _CreateInfo.MaxFrameInFlight; i++)
		{
			const uint32_t frameOffset =
				i * CH_OBJECT_SHADER_DATA_AMOUNT * sizeof(ObjectShaderData);

			_CreateInfo.MapBufferData(
				_SetBuffers[int(BindlessSetTypes::PER_OBJECT)],
				&Data,
				sizeof(ObjectShaderData),
				frameOffset + ObjectOffset
			);
		}
	}
}

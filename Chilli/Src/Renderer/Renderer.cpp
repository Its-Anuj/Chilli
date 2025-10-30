#include "Material.h"
#include "Ch_PCH.h"

#include "Renderer.h"

#include "vulkan/vulkan.h"

#include "vk_mem_alloc.h"
#include "Vulkan/VulkanRenderer.h"

namespace Chilli
{
	bool Renderer::Init(const RendererInitSpec& Spec)
	{
		if (Spec.Type == RenderAPIType::VULKAN1_3)
		{
			Get()._Api = new VulkanRenderer();

			VulkanRenderInitSpec VulkanSpec{};
			VulkanSpec.GlfwWindow = Spec.GlfwWindow;
			VulkanSpec.InFrameFlightCount = Spec.InFrameFlightCount;
			VulkanSpec.InitialWindowSize.x = Spec.InitialWindowSize.x;
			VulkanSpec.InitialWindowSize.y = Spec.InitialWindowSize.y;
			VulkanSpec.InitialFrameBufferSize.x = Spec.InitialFrameBufferSize.x;
			VulkanSpec.InitialFrameBufferSize.y = Spec.InitialFrameBufferSize.y;
			VulkanSpec.VSync = Spec.VSync;
			VulkanSpec.EnableValidationLayer = Spec.EnableValidation;
			VulkanSpec.Name = Spec.Name;

			Get()._Api->Init((void*)&VulkanSpec);
		}
		return true;
	}

	void Renderer::ShutDown()
	{
		Get()._Api->ShutDown();
		delete Get()._Api;
	}

	bool Renderer::BeginFrame()
	{
		return Get()._Api->BeginFrame();
	}

	void Renderer::BeginScene()
	{
		Get()._Api->BeginScene();;
	}

	void Renderer::BeginRenderPass()
	{
		Get()._Api->BeginRenderPass();;
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB)
	{
		Get()._Api->Submit(Shader, VB);
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB, const std::shared_ptr<IndexBuffer>& IB)
	{
		Get()._Api->Submit(Shader, VB, IB);
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Shader, const std::shared_ptr<VertexBuffer>& VB,
		const std::shared_ptr<IndexBuffer>& IB, const RenderCommandSpec& CommandSpec)
	{
		Get()._Api->Submit(Shader, VB, IB, CommandSpec);
	}

	void Renderer::EndRenderPass()
	{
		Get()._Api->EndRenderPass();;
	}

	void Renderer::EndScene()
	{
		Get()._Api->EndScene();;
	}

	void Renderer::Render()
	{
		Get()._Api->Render();;
	}

	void Renderer::Present()
	{
		Get()._Api->Present();;
	}

	void Renderer::EndFrame()
	{
		Get()._Api->EndFrame();;
	}

	void Renderer::FinishRendering()
	{
		Get()._Api->FinishRendering();
	}

	void Renderer::FrameBufferReSized(int Width, int Height)
	{
		Get()._Api->FrameBufferReSized(Width, Height);
	}

	BindlessSetManager& Renderer::GetBindlessSetManager()
	{
		return Get()._Api->GetBindlessSetManager();
	}

	std::shared_ptr<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto Shader = std::make_shared<VulkanGraphiscPipeline>(Spec);
			return Shader;
		}
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const VertexBufferSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto VB = std::make_shared<VulkanVertexBuffer>(Spec);
			return VB;
		}
	}

	std::shared_ptr<IndexBuffer> Chilli::IndexBuffer::Create(const IndexBufferSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto IB = std::make_shared<VulkanIndexBuffer>(Spec);
			return IB;
		}
	}

	std::shared_ptr<UniformBuffer> Chilli::UniformBuffer::Create(size_t Size)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto Buffer = std::make_shared<VulkanUniformBuffer>(Size);
			return Buffer;
		}
	}

	std::shared_ptr<StorageBuffer> StorageBuffer::Create(size_t Size)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto Buffer = std::make_shared<VulkanStorageBuffer>(Size);
			return Buffer;
		}
	}

	std::shared_ptr<Image> Image::Create(const ImageSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto VImage = std::make_shared<VulkanImage>(Spec);
			return VImage;
		}
	}

	std::shared_ptr<Texture> Texture::Create(const TextureSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto VTexture = std::make_shared<VulkanTexture>(Spec);
			return VTexture;
		}
	}

	std::shared_ptr<Sampler> Sampler::Create(const SamplerSpec& Spec)
	{
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
		{
			auto VSampler = std::make_shared<VulkanSampler>(Spec);
			return VSampler;
		}
	}
	// Mesh Manager

	const UUID& MeshManager::AddMesh(const VertexBufferSpec& VBSpec, const IndexBufferSpec& IBSpec)
	{
		UUID NewID;

		Mesh NewMesh;
		NewMesh.ID = NewID;
		NewMesh.IndexBuffer = IndexBuffer::Create(IBSpec);
		NewMesh.VertexBuffers.push_back(VertexBuffer::Create(VBSpec));

		_Meshes[NewID] = NewMesh;
		return NewID;
	}

	bool MeshManager::DoesMeshExist(UUID ID)
	{
		return _Meshes.find(ID) != _Meshes.end();
	}

	const Mesh& MeshManager::GetMesh(UUID ID)
	{
		if (DoesMeshExist(ID))
			return _Meshes[ID];
	}


	void MeshManager::Flush()
	{
		// Store all ids in a array
		std::vector<UUID> AllIDs;
		AllIDs.reserve(_Meshes.size());

		for (auto& [ID, Sampler] : _Meshes)
			AllIDs.push_back(ID);

		for (auto& ID : AllIDs)
			DestroyMesh(ID);
	}

	void MeshManager::DestroyMesh(const UUID& ID)
	{
		auto it = _Meshes.find(ID);
		if (it != _Meshes.end())
		{
			if (it->second.IndexBuffer)
			{
				it->second.IndexBuffer->Destroy();
				for (auto& VB : it->second.VertexBuffers)
					VB->Destroy();
			}
			_Meshes.erase(it);
		}
	}

	// Texture Manager
	const UUID& TextureManager::Add(const TextureSpec& Spec)
	{
		TextureHandle NewTexture = Texture::Create(Spec);

		_Textures[NewTexture->ID()] = NewTexture;;
		return NewTexture->ID();
	}

	const TextureHandle& Chilli::TextureManager::Get(UUID ID)
	{
		if (Exist(ID))
			return _Textures[ID];
	}

	bool Chilli::TextureManager::Exist(UUID ID)
	{
		return _Textures.find(ID) != _Textures.end();
	}

	void TextureManager::Flush()
	{
		// Store all ids in a array
		std::vector<UUID> AllIDs;
		AllIDs.reserve(_Textures.size());

		for (auto& [ID, Sampler] : _Textures)
			AllIDs.push_back(ID);

		for (auto& ID : AllIDs)
			Destroy(ID);
	}

	void TextureManager::Destroy(const UUID& ID)
	{
		auto it = _Textures.find(ID);
		if (it != _Textures.end())
		{
			if (it->second)
				it->second->Destroy();
			_Textures.erase(it);
		}
	}

	// Sampler Manager
	const UUID& SamplerManager::Add(const SamplerSpec& Spec)
	{
		SamplerHandle NewSampler = Sampler::Create(Spec);

		_Samplers[NewSampler->ID()] = NewSampler;;
		return NewSampler->ID();
	}

	const SamplerHandle& Chilli::SamplerManager::Get(UUID ID)
	{
		if (Exist(ID))
			return _Samplers[ID];
	}

	bool Chilli::SamplerManager::Exist(UUID ID)
	{
		return _Samplers.find(ID) != _Samplers.end();
	}

	void SamplerManager::Flush()
	{
		// Store all ids in a array
		std::vector<UUID> AllIDs;
		AllIDs.reserve(_Samplers.size());
		
		for (auto& [ID, Sampler] : _Samplers)
			AllIDs.push_back(ID);

		for (auto& ID: AllIDs)
			Destroy(ID);
	}

	void SamplerManager::Destroy(const UUID& ID)
	{
		auto it = _Samplers.find(ID);
		if (it != _Samplers.end())
		{
			if (it->second)
				it->second->Destroy();
			_Samplers.erase(it);
		}
	}

	// Bindless set managers 
	uint32_t BindlessSetTextureManager::Add(const UUID& ID)
	{
		uint32_t ShaderIndex = GetCount();
		_Textures[ID] = ShaderIndex;
		return ShaderIndex;
	}

	uint32_t BindlessSetTextureManager::GetIndex(const UUID& ID)
	{
		if (IsTexPresent(ID))
			return _Textures[ID];
	}

	void BindlessSetTextureManager::Remove(const UUID& ID)
	{
		auto it = _Textures.find(ID);
		if (it != _Textures.end())
			_Textures.erase(it);
	}

	bool BindlessSetTextureManager::IsTexPresent(const UUID& ID) const
	{
		return _Textures.find(ID) != _Textures.end();
	}

	uint32_t BindlessSetSamplerManager::Add(const UUID& ID)
	{
		uint32_t ShaderIndex = GetCount();
		_Samplers[ID] = ShaderIndex;
		return ShaderIndex;
	}

	uint32_t BindlessSetSamplerManager::GetIndex(const UUID& ID)
	{
		if (IsPresent(ID))
			return _Samplers[ID];
	}

	void BindlessSetSamplerManager::Remove(const UUID& ID)
	{
		auto it = _Samplers.find(ID);
		if (it != _Samplers.end())
			_Samplers.erase(it);
	}

	bool BindlessSetSamplerManager::IsPresent(const UUID& ID) const
	{
		return _Samplers.find(ID) != _Samplers.end();
	}

	uint32_t BindlessSetMaterialManager::Add(const UUID& ID)
	{
		uint32_t ShaderIndex = GetCount();
		_Materials[ID] = ShaderIndex;
		return ShaderIndex;
	}

	void BindlessSetMaterialManager::Add(const UUID& ID, uint32_t Index)
	{
		_Materials[ID] = Index;
	}

	uint32_t BindlessSetMaterialManager::GetIndex(const UUID& ID)
	{
		if (IsPresent(ID))
			return _Materials[ID];
	}

	void BindlessSetMaterialManager::Remove(const UUID& ID)
	{
		auto it = _Materials.find(ID);
		if (it != _Materials.end())
			_Materials.erase(it);
	}

	bool BindlessSetMaterialManager::IsPresent(const UUID& ID) const
	{
		return _Materials.find(ID) != _Materials.end();
	}
	void BindlessSetObjectManager::Add(const UUID& ID, uint32_t Index)
	{
		_Objects[ID] = Index;
	}

	uint32_t BindlessSetObjectManager::GetIndex(const UUID& ID)
	{
		if (IsPresent(ID))
			return _Objects[ID];
	}

	void BindlessSetObjectManager::Remove(const UUID& ID)
	{
		auto it = _Objects.find(ID);
		if (it != _Objects.end())
			_Objects.erase(it);
	}

	bool BindlessSetObjectManager::IsPresent(const UUID& ID) const
	{
		return _Objects.find(ID) != _Objects.end();
	}
}

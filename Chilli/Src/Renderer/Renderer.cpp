#include "Renderer.h"
#include "Ch_PCH.h"

#include "Renderer.h"

#include "vulkan/vulkan.h"

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

	void Renderer::BeginRenderPass()
	{
		Get()._Api->BeginRenderPass();;
	}

	void Renderer::Submit(const std::shared_ptr<GraphicsPipeline>& Pipeline, const std::shared_ptr<VertexBuffer>& VertexBuffer
		, const std::shared_ptr<IndexBuffer>& IndexBuffer)
	{
		Get()._Api->Submit(Pipeline, VertexBuffer, IndexBuffer);
	}

	void Renderer::Submit(const Material& Mat, const std::shared_ptr<VertexBuffer>& VertexBuffer, const std::shared_ptr<IndexBuffer>& IndexBuffer)
	{
		Get()._Api->Submit(Mat, VertexBuffer, IndexBuffer);
	}

	void Renderer::EndRenderPass()
	{
		Get()._Api->EndRenderPass();;
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

	ResourceFactory& Renderer::GetResourceFactory()
	{
		return Get()._Api->GetResourceFactory();
	}

	void Renderer::FrameBufferReSized(int Width, int Height)
	{
		Get()._Api->FrameBufferReSized(Width, Height);
	}

	Material::Material(const std::shared_ptr<GraphicsPipeline>& Shader)
	{
		SetShader(_Shader);
	}

	void Material::SetShader(const std::shared_ptr<GraphicsPipeline>& Shader)
	{
		_Shader = Shader;
		// Make Vulkan Backend
		if (Renderer::GetType() == RenderAPIType::VULKAN1_3)
			_Backend = std::make_shared<VulkanMaterialBackend>(Shader);
	}

	void Material::SetTexture(const char* Name, const std::shared_ptr<Texture>& Texture)
	{
		_Textures[Name] = Texture;
		_Dirty = true;
	}

	void Material::Update()
	{
		if(_Dirty)
			_Backend->Update(_Shader, _Uniforms, _Textures);
		_Dirty = false;
	}

	void Material::SetUniformBuffer(const char* Name, const std::shared_ptr<UniformBuffer>& BufferHandle)
	{
		_Uniforms[Name] = BufferHandle;
		_Dirty = true;
	}

	Material::Material()
	{

	}
}

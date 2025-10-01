#include "Ch_PCH.h"
#include "Chilli/Chilli.h"

#define GLM_FORCE_RADIANS
#include "Chilli/Libs/glm/glm/glm.hpp"
#include "Chilli/Libs/glm/glm/gtc/matrix_transform.hpp"
#include <chrono>

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	// Uniform buffer object structure
	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct Vertex
	{
		glm::vec3 Position;

		Vertex(const glm::vec3& pos) :Position(pos) {}
	};

	virtual void Init() override
	{
		Chilli::UUID id;
		CH_INFO((uint64_t)id);
			
		{
			Chilli::GraphicsPipelineSpec Spec{};
			Spec.Paths[0] = "vert.spv";
			Spec.Paths[1] = "frag.spv";
			Spec.Attribs = { {"position", Chilli::ShaderVertexTypes::FLOAT3, 0,0} };

			Shader = Chilli::Renderer::GetResourceFactory()->CreateGraphicsPipeline(Spec);
		}
		{
			std::vector<Vertex> vertices;
			vertices.reserve(3);
			vertices.push_back(glm::vec3(0.0f, -0.5f, 0.0f));
			vertices.push_back(glm::vec3(0.5f, 0.5f, 0.0f));
			vertices.push_back(glm::vec3(-0.5f, 0.5f, 0.0f));

			Chilli::VertexBufferSpec Spec{};
			Spec.Data = vertices.data();
			Spec.Size = vertices.size() * sizeof(Vertex);
			Spec.Count = vertices.size();
			Spec.Type = Chilli::BufferType::STATIC_DRAW;

			VertexBuffer = Chilli::Renderer::GetResourceFactory()->CreateVertexBuffer(Spec);
		}
		{
			const std::vector<uint16_t> indices = {
				0, 1, 2 };
			Chilli::IndexBufferSpec Spec{};
			Spec.Data = (void*)indices.data();
			Spec.Size = indices.size() * sizeof(uint16_t);
			Spec.Count = indices.size();
			Spec.Type = Chilli::BufferType::STATIC_DRAW;

			IndexBuffer = Chilli::Renderer::GetResourceFactory()->CreateIndexBuffer(Spec);
		} 
		{
			Chilli::UniformBufferSpec Spec{};
			Spec.Size = sizeof(UniformBufferObject);
			Spec.Count = 1;

			UniformBuffer = Chilli::Renderer::GetResourceFactory()->CreateUniformBuffer(Spec);
		}
		Shader->LinkUniformBuffer(UniformBuffer);
	}

	void UpdateUBO()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), 1280/ (float)720, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1; // Flip Y for Vulkan
		_UBO = ubo;
	}

	virtual void Update() override
	{
		Chilli::Renderer::BeginFrame();

		Chilli::Renderer::BeginRenderPass();

		UpdateUBO();
		UniformBuffer->StreamData((void*)&_UBO, sizeof(_UBO));
		
		Chilli::Renderer::Submit(Shader, VertexBuffer, IndexBuffer);
		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::RenderFrame();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
	}

	virtual void Terminate() override {
		Chilli::Renderer::FinishRendering();
		Chilli::Renderer::GetResourceFactory()->DestroyIndexBuffer(IndexBuffer);
		Chilli::Renderer::GetResourceFactory()->DestroyUniformBuffer(UniformBuffer);
		Chilli::Renderer::GetResourceFactory()->DestroyVertexBuffer(VertexBuffer);
		Chilli::Renderer::GetResourceFactory()->DestroyGraphicsPipeline(Shader);
	}

	virtual void OnEvent(Chilli::Event& e) override {
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> Shader;
	std::shared_ptr<Chilli::IndexBuffer> IndexBuffer;
	std::shared_ptr<Chilli::VertexBuffer> VertexBuffer;
	std::shared_ptr<Chilli::UniformBuffer> UniformBuffer;
	UniformBufferObject _UBO;
};

int main()
{
	Chilli::ApplicationSpec Spec{};
	Spec.Name = "Chilli Editor";
	Spec.Dimensions = { 1280, 720 };
	Spec.VSync = true;

	Chilli::Application App;
	App.Init(Spec);
	App.PushLayer(std::make_shared<Editor>());
	App.Run();
	App.ShutDown();

	std::cin.get();
}

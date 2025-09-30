#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "Chilli/Libs/glm/glm/glm.hpp"

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

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
	}

	virtual void Update() override
	{
		Chilli::Renderer::BeginFrame();

		Chilli::Renderer::BeginRenderPass();
		Chilli::Renderer::Submit(Shader, VertexBuffer, IndexBuffer);
		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::RenderFrame();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
	}

	virtual void Terminate() override {
		Chilli::Renderer::FinishRendering();
		Chilli::Renderer::GetResourceFactory()->DestroyIndexBuffer(IndexBuffer);
		Chilli::Renderer::GetResourceFactory()->DestroyVertexBuffer(VertexBuffer);
		Chilli::Renderer::GetResourceFactory()->DestroyGraphicsPipeline(Shader);
	}

	virtual void OnEvent(Chilli::Event& e) override {
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> Shader;
	std::shared_ptr<Chilli::IndexBuffer> IndexBuffer;
	std::shared_ptr<Chilli::VertexBuffer> VertexBuffer;
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

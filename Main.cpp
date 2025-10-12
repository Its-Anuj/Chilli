#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#include "glm.hpp"

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	struct UBO
	{
		glm::mat4 View, Proj, Transform;
	};

	virtual void Init() override
	{
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec2 uv;
		};

		{
			const std::vector<Vertex> Vertices = {
				//   Position              //    UV
				{{ 0.5f, -0.5f, 0.0f },   { 1.0f, 1.0f }},  // Bottom-right
				{{ 0.5f,  0.5f, 0.0f },   { 1.0f, 0.0f }},  // Top-right
				{{-0.5f,  0.5f, 0.0f },   { 0.0f, 0.0f }},  // Top-left
				{{-0.5f, -0.5f, 0.0f },   { 0.0f, 1.0f }}   // Bottom-left
			};

			Chilli::VertexBufferSpec Spec{};
			Spec.Count = (uint32_t)Vertices.size();
			Spec.Data = (void*)Vertices.data();
			Spec.Size = sizeof(Vertex) * Vertices.size();
			Spec.State = Chilli::BufferState::STATIC_DRAW;
			VertexBuffer = Chilli::Renderer::GetResourceFactory().CreateVertexBuffer(Spec);
		}
		{
			const std::vector<uint16_t> Indicies = {
			   0, 1, 2,   // First triangle
			   2, 3, 0    // Second triangle
			};

			Chilli::IndexBufferSpec Spec{};
			Spec.Count = (uint32_t)Indicies.size();
			Spec.Data = (void*)Indicies.data();
			Spec.Size = sizeof(uint16_t) * Indicies.size();
			Spec.State = Chilli::BufferState::STATIC_DRAW;
			IndexBuffer = Chilli::Renderer::GetResourceFactory().CreateIndexBuffer(Spec);
		}
		UniformBuffer = Chilli::Renderer::GetResourceFactory().CreateUniformBuffer(sizeof(UBO));
		{
			Chilli::PipelineSpec Spec{};
			Spec.Paths[0] = "vert.spv";
			Spec.Paths[1] = "frag.spv";
			Spec.ShaderCullMode = Chilli::CullMode::Back;

			Shader = Chilli::Renderer::GetResourceFactory().CreateGraphicsPipeline(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "A.png";
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIONAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;

			Texture = Chilli::Renderer::GetResourceFactory().CreateTexture(Spec);
		}
		{
			Mat.SetShader(Shader);
			Mat.SetUniformBuffer("ubo", UniformBuffer);
			Mat.SetTexture("Tex", Texture);

			Mat.Update();
		}
	}

	void UpdateUBO()
	{
		UBO ubo;
		ubo.Transform = glm::mat4(1.0f);
		ubo.View = glm::mat4(1.0f);
		ubo.Proj = glm::mat4(1.0f);
		UniformBuffer->MapData((void*)&ubo, sizeof(ubo));
	}

	virtual void Update() override
	{
		bool Continue = Chilli::Renderer::BeginFrame();
		if (!Continue)
			return;

		Chilli::Renderer::BeginRenderPass();

		UpdateUBO();

		Chilli::Renderer::Submit(Mat, VertexBuffer, IndexBuffer);
		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::Render();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
	}

	virtual void Terminate() override {
		Chilli::Renderer::FinishRendering();
		Chilli::Renderer::GetResourceFactory().DestroyGraphicsPipeline(Shader);
		Chilli::Renderer::GetResourceFactory().DestroyVertexBuffer(VertexBuffer);
		Chilli::Renderer::GetResourceFactory().DestroyIndexBuffer(IndexBuffer);
		Chilli::Renderer::GetResourceFactory().DestroyUniformBuffer(UniformBuffer);
		Chilli::Renderer::GetResourceFactory().DestroyTexture(Texture);
	}

	virtual void OnEvent(Chilli::Event& e) override {
		if (e.GetType() == Chilli::FrameBufferResizeEvent::GetStaticType())
		{
			auto Fb = static_cast<Chilli::FrameBufferResizeEvent&>(e);
			Chilli::Renderer::FrameBufferReSized(Fb.GetX(), Fb.GetY());
		}
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> Shader;
	std::shared_ptr<Chilli::VertexBuffer> VertexBuffer;
	std::shared_ptr<Chilli::IndexBuffer> IndexBuffer;
	std::shared_ptr<Chilli::UniformBuffer> UniformBuffer;
	std::shared_ptr<Chilli::Texture> Texture;
	Chilli::Material Mat;
};

int main()
{
	Chilli::ApplicationSpec Spec{};
	Spec.Name = "Chilli Editor";
	Spec.Dimensions = { 800, 600 };
	Spec.VSync = true;

	Chilli::Application App;
	App.Init(Spec);
	App.PushLayer(std::make_shared<Editor>());
	App.Run();
	App.ShutDown();

	std::cin.get();
}

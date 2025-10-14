#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>	

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	struct UBO
	{
		glm::mat4 Transform;
	};

	struct GlobalUBO
	{
		glm::mat4 ViewProjMatrix;
	};

	struct BirdData
	{
		glm::vec3 Position{ 0.0f };
		glm::vec3 Scale{ 0.2f };
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
				// Front face
				{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
				{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
				{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},

				// Back face
				{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
				{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},
				{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

				// Left face
				{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
				{{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}},
				{{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}},
				{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},

				// Right face
				{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
				{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}},
				{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},

				// Top face
				{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}},
				{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}},
				{{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}},
				{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}},

				// Bottom face
				{{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}},
				{{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}},
				{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
				{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}}
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
	0, 1, 2, 2, 3, 0,       // Front
	4, 5, 6, 6, 7, 4,       // Back
	8, 9,10,10,11, 8,       // Left
   12,13,14,14,15,12,       // Right
   16,17,18,18,19,16,       // Top
   20,21,22,22,23,20        // Bottom
			};

			Chilli::IndexBufferSpec Spec{};
			Spec.Count = (uint32_t)Indicies.size();
			Spec.Data = (void*)Indicies.data();
			Spec.Size = sizeof(uint16_t) * Indicies.size();
			Spec.State = Chilli::BufferState::STATIC_DRAW;
			IndexBuffer = Chilli::Renderer::GetResourceFactory().CreateIndexBuffer(Spec);
		}
		GlobalUB = Chilli::Renderer::GetResourceFactory().CreateUniformBuffer(sizeof(GlobalUBO));
		BirdUBO = Chilli::Renderer::GetResourceFactory().CreateUniformBuffer(sizeof(UBO));
		{
			Chilli::PipelineSpec Spec{};
			Spec.Paths[0] = "vert.spv";
			Spec.Paths[1] = "frag.spv";
			Spec.ShaderCullMode = Chilli::CullMode::Back;
			Spec.EnableDepthStencil = true;
			Spec.EnableDepthTest = true;
			Spec.EnableDepthWrite = true;
			Spec.EnableStencilTest= true;
			Spec.DepthFormat = Chilli::ImageFormat::D24_S8;

			Shader = Chilli::Renderer::GetResourceFactory().CreateGraphicsPipeline(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "flappy-bird.png";
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIOMAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Mode = Chilli::SamplerMode::REPEAT;
			Spec.Filter = Chilli::SamplerFilter::LINEAR;

			Texture = Chilli::Renderer::GetResourceFactory().CreateTexture(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = nullptr;
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIOMAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Format = Chilli::ImageFormat::D24_S8;
			Spec.Resolution = { 800, 600 };
			Spec.Usage = Chilli::ImageUsage::DEPTH;

			DepthTexture = Chilli::Renderer::GetResourceFactory().CreateTexture(Spec);
		}
		{
			Mat1.SetShader(Shader);
			Mat1.SetUniformBuffer("GlobalUBO", GlobalUB);
			Mat1.SetUniformBuffer("ubo", BirdUBO);
			Mat1.SetTexture("Tex", Texture);

			Mat1.Update();
		}
		// Projection matrix (800x600 resolution, 45° FOV)
		glm::mat4 projection = glm::perspective(
			glm::radians(45.0f),      // Field of view
			800.0f / 600.0f,          // Aspect ratio (800x600)
			0.1f, 100.0f              // Near and far planes
		);

		// View matrix (camera at Z = -3, looking at origin)
		glm::mat4 view = glm::lookAt(
			glm::vec3(0.0f, 0.0f, 2.0f),  // Camera position (back 3 units)
			glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
			glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
		);

		globalobject.ViewProjMatrix = projection * view;  // View-projection matrix
		GlobalUB->MapData((void*)&globalobject, sizeof(globalobject));
	}

	virtual void Update() override
	{
		bool Continue = Chilli::Renderer::BeginFrame();
		if (!Continue)
			return;

		Chilli::ColorAttachment ColorAttachment{};
		ColorAttachment.UseSwapChainImage = true;
		ColorAttachment.ClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };

		Chilli::DepthAttachment DepthAttachment{};
		DepthAttachment.TextureAttachment = DepthTexture;

		RenderPass.ColorAttachmentCount = 1;
		RenderPass.ColorAttachments = &ColorAttachment;
		RenderPass.DepthAttachment = &DepthAttachment;

		Chilli::Renderer::BeginRenderPass(RenderPass);

		static float decrease = 0.0f;
		UBO ubo;
		ubo.Transform = glm::translate(glm::mat4(1.0f), BirdData.Position);
		ubo.Transform = glm::rotate(ubo.Transform, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		BirdUBO->MapData((void*)&ubo, sizeof(ubo));

		Chilli::Renderer::Submit(Mat1, VertexBuffer, IndexBuffer);

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
		Chilli::Renderer::GetResourceFactory().DestroyUniformBuffer(GlobalUB);
		Chilli::Renderer::GetResourceFactory().DestroyUniformBuffer(BirdUBO);
		Chilli::Renderer::GetResourceFactory().DestroyTexture(Texture);
		Chilli::Renderer::GetResourceFactory().DestroyTexture(DepthTexture);
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
	std::shared_ptr<Chilli::UniformBuffer> GlobalUB;
	std::shared_ptr<Chilli::UniformBuffer> BirdUBO;
	std::shared_ptr<Chilli::Texture> Texture, DepthTexture;
	GlobalUBO globalobject;
	Chilli::Material Mat1;
	Chilli::RenderPass RenderPass{};
	BirdData BirdData;
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

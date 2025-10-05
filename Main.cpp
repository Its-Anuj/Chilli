#include "Ch_PCH.h"
#include "Chilli/Chilli.h"

#define GLM_FORCE_RADIANS
#include "Chilli/Libs/glm/glm/glm.hpp"
#include "Chilli/Libs/glm/glm/gtc/matrix_transform.hpp"
#include <chrono>

class Camera
{
public:
	Camera(const glm::vec3& pPos = glm::vec3(0.0f, 0.0f, 3.0f), const glm::vec3& p_Target = glm::vec3(0.0f, 0.0f, 0.0f),
		const glm::vec3& pUp = glm::vec3(0.0f, 1.0f, 3.0f))
		:_Pos(pPos), _Target(p_Target), _Up(pUp)
	{
	}

	// Calculate matrices
	glm::mat4 getViewMatrix() const {
		return glm::lookAt(_Pos, _Target, _Up);
	}

	glm::mat4 getProjectionMatrix(float aspectRatio) const {
		return glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
	}

	// Simple movement
	void moveForward(float amount) {
		glm::vec3 direction = glm::normalize(_Target - _Pos);
		_Pos += direction * amount;
		_Target += direction * amount;
	}

	void moveRight(float amount) {
		glm::vec3 direction = glm::normalize(_Target - _Pos);
		glm::vec3 right = glm::normalize(glm::cross(direction, _Up));
		_Pos += right * amount;
		_Target += right * amount;
	}

	void moveUp(float amount) {
		_Pos += _Up * amount;
		_Target += _Up * amount;
	}

private:
	glm::vec3 _Pos;
	glm::vec3 _Target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 _Up = glm::vec3(0.0f, 1.0f, 0.0f);
};

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
		glm::vec3 _Pos;
		glm::vec2 Uv;

		Vertex(const glm::vec3& pos, const glm::vec2& uv) :_Pos(pos), Uv(uv) {}
	};

	virtual void Init() override
	{
		{
			std::vector<Vertex> vertices = {
				{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}},
				{{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}},
				{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
				{{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}},

				{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
				{{0.5f, -0.5f, -0.5f},  {1.0f, 0.0f}},
				{{0.5f, 0.5f, -0.5f},  {1.0f, 1.0f}},
				{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}
			};

			Chilli::VertexBufferSpec Spec{};
			Spec.Data = vertices.data();
			Spec.Size = vertices.size() * sizeof(Vertex);
			Spec.Count = vertices.size();
			Spec.Type = Chilli::BufferType::STATIC_DRAW;

			VertexBuffer = Chilli::Renderer::GetResourceFactory()->CreateVertexBuffer(Spec);
		}
		{
			const std::vector<uint16_t> indices = {
				0, 1, 2, 2, 3, 0,
				4, 5, 6, 6, 7, 4
			};

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
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "A.png";
			Spec.Format = Chilli::ImageFormat::RGBA8;
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIONAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Aspect = Chilli::ImageAspect::COLOR;

			Texture = Chilli::Renderer::GetResourceFactory()->CreateTexture(Spec);
		}
		{
			Chilli::TextureSpec Spec{};

			Spec.Format = Chilli::ImageFormat::RGBA8;
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIONAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Resolution.Width = 800;
			Spec.Resolution.Height = 800;
			Spec.FilePath = nullptr;
			Spec.Aspect = Chilli::ImageAspect::COLOR;

			uint32_t* ImageData = new uint32_t[800 * 800];
			for (int y = 0; y < 800; y++)
			{
				for (int x = 0; x < 800; x++)
				{
					if ((x / 50 + y / 50) % 2 == 0) {
						ImageData[y * 800 + x] = 0xFFFF0000;
					}
					else {
						ImageData[y * 800 + x] = 0xFF0000FF;
					}
				}
			}

			Spec.ImageData = ImageData;

			WhiteTexture = Chilli::Renderer::GetResourceFactory()->CreateTexture(Spec);
			delete[] ImageData;
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.Resolution.Width = Chilli::Renderer::GetFrameBufferSize().x;
			Spec.Resolution.Height = Chilli::Renderer::GetFrameBufferSize().y;
			Spec.Format = Chilli::ImageFormat::D32_FLOAT;
			Spec.Tiling = Chilli::ImageTiling::IMAGE_TILING_OPTIONAL;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.ImageData = nullptr;
			Spec.FilePath = nullptr;
			Spec.Aspect = Chilli::ImageAspect::DEPTH;

			DepthTexture = Chilli::Renderer::GetResourceFactory()->CreateTexture(Spec);
		}
		{
			Chilli::GraphicsPipelineSpec Spec{};
			Spec.Paths[0] = "vert.spv";
			Spec.Paths[1] = "frag.spv";
			Spec.Attribs = { {"_Pos", Chilli::ShaderVertexTypes::FLOAT3, 0,0},
				{"tecoord", Chilli::ShaderVertexTypes::FLOAT2, 0,1} };
			Spec.UniformAttribs = {
				{Chilli::ShaderUniformTypes::UNIFORM, Chilli::ShaderStageType::VERTEX, "UBO", 0},
				{Chilli::ShaderUniformTypes::SAMPLED_IMAGE, Chilli::ShaderStageType::FRAGMENT, "Texture", 1}
			};
			Spec.UBs.push_back(UniformBuffer);
			Spec.Texs = WhiteTexture;

			Shader = Chilli::Renderer::GetResourceFactory()->CreateGraphicsPipeline(Spec);
		}
	}

	void UpdateUBO()
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = _Camera.getViewMatrix();
		ubo.proj = _Camera.getProjectionMatrix(1280 / 720);
		ubo.proj[1][1] *= -1; // Flip Y for Vulkan
		_UBO = ubo;
	}

	virtual void Update() override
	{
		bool Continue = Chilli::Renderer::BeginFrame();
		if (!Continue)
			return;

		Chilli::ColorAttachment ColorAttachment{};
		ColorAttachment.UseSwapChainTexture = true;
		ColorAttachment.ClearColor = { 0.4f, 0.8f, 0.4f, 1.0f };

		Chilli::DepthAttachment DepthAttachment{};
		DepthAttachment.DepthTexture = DepthTexture;
		DepthAttachment.Planes.Far = 1.0f;
		DepthAttachment.Planes.Near = 0;
		DepthAttachment.UseSwapChainTexture = false;

		NormalPassInfo.ColorAttachments = &ColorAttachment;
		NormalPassInfo.ColorAttachmentCount = 1;
		NormalPassInfo.DepthAttachment = DepthAttachment;

		Chilli::Renderer::BeginRenderPass(NormalPassInfo);

		if (Chilli::Input::IsKeyPressed(Chilli::Input_key_W) == Chilli::InputResult::INPUT_PRESS ||
			Chilli::Input::IsKeyPressed(Chilli::Input_key_W) == Chilli::InputResult::INPUT_REPEAT)
		{
			_Camera.moveForward(0.02f);
		}
		else if (Chilli::Input::IsKeyPressed(Chilli::Input_key_S) == Chilli::InputResult::INPUT_PRESS ||
			Chilli::Input::IsKeyPressed(Chilli::Input_key_S) == Chilli::InputResult::INPUT_REPEAT)
		{
			_Camera.moveForward(-0.02f);
		}

		if (Chilli::Input::IsKeyPressed(Chilli::Input_key_A) == Chilli::InputResult::INPUT_PRESS ||
			Chilli::Input::IsKeyPressed(Chilli::Input_key_A) == Chilli::InputResult::INPUT_REPEAT)
		{
			_Camera.moveRight(-0.02f);
		}
		else if (Chilli::Input::IsKeyPressed(Chilli::Input_key_D) == Chilli::InputResult::INPUT_PRESS ||
			Chilli::Input::IsKeyPressed(Chilli::Input_key_D) == Chilli::InputResult::INPUT_REPEAT)
		{
			_Camera.moveRight(0.02f);
		}

		if(Chilli::Input::IsKeyPressed(Chilli::Input_key_Space) != Chilli::InputResult::INPUT_REPEAT)
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
		Chilli::Renderer::GetResourceFactory()->DestroyTexture(Texture);
		Chilli::Renderer::GetResourceFactory()->DestroyTexture(WhiteTexture);
		Chilli::Renderer::GetResourceFactory()->DestroyTexture(DepthTexture);
	}

	virtual void OnEvent(Chilli::Event& e) override {
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> Shader;
	std::shared_ptr<Chilli::IndexBuffer> IndexBuffer;
	std::shared_ptr<Chilli::VertexBuffer> VertexBuffer;
	std::shared_ptr<Chilli::UniformBuffer> UniformBuffer;
	std::shared_ptr<Chilli::Texture> Texture;
	std::shared_ptr<Chilli::Texture> WhiteTexture;
	std::shared_ptr<Chilli::Texture> DepthTexture;
	UniformBufferObject _UBO;
	Camera _Camera;
	Chilli::BeginRenderPassInfo NormalPassInfo;
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

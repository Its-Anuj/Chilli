#include "Ch_PCH.h"
#include "Chilli/Chilli.h"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>	

#include "Extensions/Camera.h"

#define MAX_MATERIAL 1000
#define MAX_OBJECT 10000

struct GlobalUBO
{
	glm::vec4 ResolutionTime; // x,y for resolution , z for time, w null
};

struct SceneUBO
{
	glm::mat4 ViewProjMatrix;
	glm::vec4 CameraPos;
};

struct Vertex {
	glm::vec3 Position;
	glm::vec2 TexCoord;
};

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	virtual void Init() override
	{
		{
			const std::vector<Vertex> Vertices = {
				// First triangle
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f } }, // bottom-left
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f } }, // bottom-right
				{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f } }, // top-right

				// Second triangle
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f } }, // bottom-left
				{ {  0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f } }, // top-right
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f } }  // top-left
			};

			Chilli::VertexBufferSpec VBSpec{};
			VBSpec.Count = (uint32_t)Vertices.size();
			VBSpec.Data = (void*)Vertices.data();
			VBSpec.Size = sizeof(Vertex) * Vertices.size();
			VBSpec.State = Chilli::BufferState::STATIC_DRAW;

			const std::vector<uint16_t> Indicies = {
				0, 1, 2, 3, 4, 5 };

			Chilli::IndexBufferSpec IBSpec{};
			IBSpec.Count = (uint32_t)Indicies.size();
			IBSpec.Data = (void*)Indicies.data();
			IBSpec.Size = sizeof(uint16_t) * Indicies.size();
			IBSpec.State = Chilli::BufferState::STATIC_DRAW;

			SquareMeshID = MeshManager.AddMesh(VBSpec, IBSpec);
		}
		{
			Chilli::GraphicsPipelineSpec Spec{};
			Spec.VertPath = "vert.spv";
			Spec.FragPath = "frag.spv";

			DeafultShader = Chilli::GraphicsPipeline::Create(Spec);
		}
		{
			Chilli::SamplerSpec Spec{};
			Spec.Filter = Chilli::SamplerFilter::LINEAR;
			Spec.Mode = Chilli::SamplerMode::REPEAT;;

			DeafultSamplerID = SamplerManager.Add(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "Deafult.png";
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.YFlip = true;

			DeafultTextureID = TextureManager.Add(Spec);
		}

		{
			Chilli::Renderer::GetBindlessSetManager().SetGlobalUBO(sizeof(GlobalUBO));
			Chilli::Renderer::GetBindlessSetManager().SetSceneUBO(sizeof(SceneUBO));

			Chilli::Renderer::GetBindlessSetManager().CreateMaterialSBO(sizeof(Chilli::MaterialShaderData) * MAX_MATERIAL,
				MAX_MATERIAL);

			Chilli::Renderer::GetBindlessSetManager().CreateObjectSBO(sizeof(Chilli::ObjectShaderData) * MAX_OBJECT,
				MAX_OBJECT);

			Chilli::Renderer::GetBindlessSetManager().AddSampler(SamplerManager.Get(DeafultSamplerID));
			Chilli::Renderer::GetBindlessSetManager().AddTexture(TextureManager.Get(DeafultTextureID));
		}

		DeafultMaterial.AlbedoSampler = DeafultSamplerID;
		DeafultMaterial.AlbedoTexture = DeafultTextureID;
		DeafultMaterial.AlbedoColor = { 0.4f, 0.6f, 0.2f, 1.0f };

		DeafultObject.MaterialIndex = DeafultMaterial.ID;
		DeafultObject.MeshIndex = SquareMeshID;
		DeafultObject.Transform.TransformationMat = glm::mat4(1.0f);

		Chilli::Renderer::GetBindlessSetManager().UpdateMaterial(DeafultMaterial, 0);
		Chilli::Renderer::GetBindlessSetManager().UpdateObject(DeafultObject, 0);

		// Camera setup
		_Camera.SetProjection(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
	}

	void UpdateGlobalData()
	{
		GlobalData = GlobalUBO();
		GlobalData.ResolutionTime = { WindowSize.x, WindowSize.y, Chilli::GetWindowTime(), 0.0f };
		Chilli::Renderer::GetBindlessSetManager().UpdateGlobalUBO(&GlobalData);
	}

	void UpdateSceneData()
	{
		SceneData = SceneUBO();

		SceneData.ViewProjMatrix = _Camera.GetProjectionMatrix() * _Camera.GetViewMatrix();
		SceneData.CameraPos.x = _Camera.GetPosition().x;
		SceneData.CameraPos.y = _Camera.GetPosition().y;
		SceneData.CameraPos.z = _Camera.GetPosition().z;
		SceneData.CameraPos.w = 0;
		Chilli::Renderer::GetBindlessSetManager().UpdateSceneUBO(&SceneData);
	}

	void Draw(const Chilli::RenderCommand& Command)
	{
		auto& CurrentMesh = MeshManager.GetMesh(Command.RenderObject.MeshIndex);
		Chilli::RenderCommandSpec Spec{};
		Spec.MaterialIndex = Command.RenderObject.MaterialIndex;
		Spec.ObjectIndex = Command.RenderObject.ID;

		Chilli::Renderer::Submit(DeafultShader, CurrentMesh.VertexBuffers[0], CurrentMesh.IndexBuffer, Spec);
		// Take in Shader from Material
		//Chilli::Renderer::Submit(DeafultShader, CurrentMesh.VertexBuffers[0], CurrentMesh.IndexBuffer);

	}

	virtual void Update() override
	{
		if (_Minimized)
		{
			CH_CORE_INFO("Minized!");
		}
		_Camera.Process(0.1f);

		UpdateGlobalData();
		Chilli::Renderer::BeginFrame();

		UpdateSceneData();
		Chilli::Renderer::BeginScene();

		Chilli::Renderer::BeginRenderPass();

		Chilli::RenderCommand Command{ .RenderObject = DeafultObject };
		Draw(Command);
		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::EndScene();

		Chilli::Renderer::Render();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
	}

	virtual void Terminate() override {
		Chilli::Renderer::FinishRendering();
		MeshManager.Flush();
		TextureManager.Flush();
		SamplerManager.Flush();
		DeafultShader->Destroy();
	}

	void OnKeyPressed(Chilli::KeyPressedEvent& e)
	{
		CH_CORE_INFO("Pressed: {0}", Chilli::InputKeyToString(e.GetKeyCode()));
	}

	void OnFrameBufferResizeEvent(Chilli::FrameBufferResizeEvent& e) override
	{
		Chilli::Renderer::FrameBufferReSized(e.GetX(), e.GetY());
		_Camera.SetProjection(45.0f, e.GetX() / e.GetY(), 0.1f, 100.0f);
	}

	void OnWindowResizeEvent(Chilli::WindowResizeEvent& e)
	{
		WindowSize = (e.GetX(), e.GetY());

		if (WindowSize.x != 0 && WindowSize.y != 0)
			_Minimized = false;
	}

	void OnWindowMinimizedEvent(Chilli::WindowMinimizedEvent& e)
	{
		_Minimized = true;
	};

	virtual void OnCursorPosEvent(Chilli::CursorPosEvent& e)
	{
		static float lastX = 400;
		static float lastY = 300;
		static bool firstMouse = true;

		if (firstMouse) {
			lastX = e.GetX();
			lastY = e.GetY();
			firstMouse = false;
		}

		float xOffset = e.GetX() - lastX;
		float yOffset = lastY - e.GetY(); // Reversed since y-coordinates go from bottom to top

		lastX = e.GetX();
		lastY = e.GetY();

		if (Chilli::Input::IsMouseButtonPressed(Chilli::Input_mouse_Right) == Chilli::InputResult::INPUT_RELEASE)
			return;

		_Camera.ProcessMouseMovement(xOffset, yOffset);
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> DeafultShader;
	bool _Minimized = false;

	GlobalUBO GlobalData;
	SceneUBO SceneData;
	Chilli::Vec2 WindowSize = { 800.0f, 600.0f };

	Chilli::Material DeafultMaterial;
	Chilli::Object DeafultObject;

	Chilli::MeshManager MeshManager;
	Chilli::SamplerManager SamplerManager;
	Chilli::TextureManager TextureManager;
	//Chilli::ShaderManager ShaderManager;
	Chilli::UUID SquareMeshID, DeafultTextureID, DeafultSamplerID;

	Camera _Camera;
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

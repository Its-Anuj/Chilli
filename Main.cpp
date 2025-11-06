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

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	virtual void Init() override
	{
		{
			auto MeshDataSpec = Chilli::GetCubeMeshSpec();
			std::vector<Chilli::Vertex> Vertices;
			Vertices.resize(MeshDataSpec.first);
			std::vector<uint16_t> Indicies;
			Indicies.resize(MeshDataSpec.second);
			Chilli::FillCubeMeshData(Vertices.data(), Indicies.data());

			Chilli::VertexBufferSpec VBSpec{};
			VBSpec.Count = (uint32_t)Vertices.size();
			VBSpec.Data = (void*)Vertices.data();
			VBSpec.Size = sizeof(Chilli::Vertex) * Vertices.size();
			VBSpec.State = Chilli::BufferState::STATIC_DRAW;


			Chilli::IndexBufferSpec IBSpec{};
			IBSpec.Count = (uint32_t)Indicies.size();
			IBSpec.Data = (void*)Indicies.data();
			IBSpec.Size = sizeof(uint16_t) * Indicies.size();
			IBSpec.State = Chilli::BufferState::STATIC_DRAW;

			CubeMeshID = MeshManager.AddMesh(VBSpec, IBSpec);
		}
		{
			auto MeshDataSpec = Chilli::GetSquareMeshSpec();
			std::vector<Chilli::Vertex> Vertices;
			Vertices.resize(MeshDataSpec.first);
			std::vector<uint16_t> Indicies;
			Indicies.resize(MeshDataSpec.second);
			Chilli::FillSquareMeshData(Vertices.data(), Indicies.data());

			Chilli::VertexBufferSpec VBSpec{};
			VBSpec.Count = (uint32_t)Vertices.size();
			VBSpec.Data = (void*)Vertices.data();
			VBSpec.Size = sizeof(Chilli::Vertex) * Vertices.size();
			VBSpec.State = Chilli::BufferState::STATIC_DRAW;


			Chilli::IndexBufferSpec IBSpec{};
			IBSpec.Count = (uint32_t)Indicies.size();
			IBSpec.Data = (void*)Indicies.data();
			IBSpec.Size = sizeof(uint16_t) * Indicies.size();
			IBSpec.State = Chilli::BufferState::STATIC_DRAW;

			SquareMeshID = MeshManager.AddMesh(VBSpec, IBSpec);
		}
		{
			Chilli::GraphicsPipelineSpec Spec{};
			Spec.VertPath = "Assets/Shaders/shader_vert.spv";
			Spec.FragPath = "Assets/Shaders/shader_frag.spv";
			Spec.EnableDepthStencil = true;
			Spec.DepthFormat = Chilli::ImageFormat::D24_S8;
			Spec.UseSwapChainColorFormat = false;
			Spec.ColorFormat = Chilli::ImageFormat::RGBA8;

			GeometryShaderID = ShaderManager.Add(Spec);
		}
		{
			Chilli::GraphicsPipelineSpec Spec{};
			Spec.VertPath = "Assets/Shaders/screen_vert.spv";
			Spec.FragPath = "Assets/Shaders/screen_frag.spv";
			Spec.UseSwapChainColorFormat = true;

			ScreenShaderID = ShaderManager.Add(Spec);
		}
		{
			Chilli::SamplerSpec Spec{};
			Spec.Filter = Chilli::SamplerFilter::LINEAR;
			Spec.Mode = Chilli::SamplerMode::MIRRORED_REPEAT;

			DeafultSamplerID = SamplerManager.Add(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "Assets/Textures/Deafult.png";
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Usage = Chilli::ImageUsage::SAMPLED_IMAGE | Chilli::ImageUsage::TRANSFER_DST;
			Spec.YFlip = true;

			DeafultTextureID = TextureManager.Add(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "Assets/Textures/download.jpg";
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Usage = Chilli::ImageUsage::SAMPLED_IMAGE | Chilli::ImageUsage::TRANSFER_DST;
			Spec.YFlip = true;

			BrickTextureID = TextureManager.Add(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "";
			Spec.Resolution = { GeometryPassResolution.x, GeometryPassResolution.y };
			Spec.Format = Chilli::ImageFormat::RGBA8;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Usage = Chilli::ImageUsage::COLOR_ATTACHMENT | Chilli::ImageUsage::SAMPLED_IMAGE;
			Spec.UseFilePath = false;

			GeometryPassColorTexture = TextureManager.Add(Spec);
		}
		{
			Chilli::TextureSpec Spec{};
			Spec.FilePath = "";
			Spec.Resolution = { GeometryPassResolution.x,GeometryPassResolution.y };
			Spec.Format = Chilli::ImageFormat::D24_S8;
			Spec.Type = Chilli::ImageType::IMAGE_TYPE_2D;
			Spec.Usage = Chilli::ImageUsage::DEPTH_STENCIL_ATTACHMENT;
			Spec.UseFilePath = false;

			GeometryPassDepthTexture = TextureManager.Add(Spec);
		} {
			Chilli::Renderer::GetBindlessSetManager().SetGlobalUBO(sizeof(GlobalUBO));
			Chilli::Renderer::GetBindlessSetManager().SetSceneUBO(sizeof(Chilli::SceneShaderData));

			Chilli::Renderer::GetBindlessSetManager().CreateMaterialSBO(sizeof(Chilli::MaterialShaderData) * MAX_MATERIAL,
				MAX_MATERIAL);

			Chilli::Renderer::GetBindlessSetManager().CreateObjectSBO(sizeof(Chilli::ObjectShaderData) * MAX_OBJECT,
				MAX_OBJECT);

			Chilli::Renderer::GetBindlessSetManager().AddSampler(SamplerManager.Get(DeafultSamplerID));

			Chilli::Renderer::GetBindlessSetManager().AddTexture(TextureManager.Get(BrickTextureID));
			Chilli::Renderer::GetBindlessSetManager().AddTexture(TextureManager.Get(DeafultTextureID));
			Chilli::Renderer::GetBindlessSetManager().AddTexture(TextureManager.Get(GeometryPassColorTexture));
		}
		{
			auto A = Chilli::Material();
			A.AlbedoColor = { 1.0, 1.0f, 1.0f, 1.0f };
			A.AlbedoTexture = DeafultTextureID;
			A.AlbedoSampler = DeafultSamplerID;
			A.UsingPipelineID = GeometryShaderID;
			DeafultMaterialID = MaterialManager.Add(A);
			Chilli::Renderer::GetBindlessSetManager().UpdateMaterial(MaterialManager.Get(DeafultMaterialID), 0);
		}
		{
			auto A = Chilli::Material();
			A.AlbedoColor = { 1.0, 1.0f, 1.0f, 1.0f };
			A.AlbedoTexture = BrickTextureID;
			A.AlbedoSampler = DeafultSamplerID;
			A.UsingPipelineID = GeometryShaderID;
			CubeMaterialID = MaterialManager.Add(A);
			Chilli::Renderer::GetBindlessSetManager().UpdateMaterial(MaterialManager.Get(CubeMaterialID), 1);
		}
		{
			auto A = Chilli::Material();
			A.AlbedoColor = { 1.0, 1.0f, 1.0f, 1.0f };
			A.AlbedoTexture = GeometryPassColorTexture;
			A.AlbedoSampler = DeafultSamplerID;
			A.UsingPipelineID = ScreenShaderID;
			ScreenMaterialID = MaterialManager.Add(A);
			Chilli::Renderer::GetBindlessSetManager().UpdateMaterial(MaterialManager.Get(ScreenMaterialID), 2);
		}


		ScreenQuadObject.MaterialIndex = ScreenMaterialID;
		ScreenQuadObject.MeshIndex = SquareMeshID;
		ScreenQuadObject.Transform.Scale(2.0f);
		Chilli::Renderer::GetBindlessSetManager().UpdateObject(ScreenQuadObject, 0);

		CubeObject.MaterialIndex = CubeMaterialID;
		CubeObject.MeshIndex = CubeMeshID;
		Chilli::Renderer::GetBindlessSetManager().UpdateObject(CubeObject, 1);

		// Camera setup
		_Camera.SetProjection(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);

	}

	void UpdateGlobalData()
	{
		GlobalData = GlobalUBO();
		GlobalData.ResolutionTime = { WindowSize.x, WindowSize.y, Chilli::GetWindowTime(), 0.0f };
		Chilli::Renderer::GetBindlessSetManager().UpdateGlobalUBO(&GlobalData);
	}

	void UpdateSceneData(const Chilli::Scene& Scene)
	{
		auto SceneData = Chilli::SceneShaderData();

		SceneData.CameraPos.x = Scene.CameraPos.x;
		SceneData.CameraPos.y = Scene.CameraPos.y;
		SceneData.CameraPos.z = Scene.CameraPos.z;
		SceneData.CameraPos.w = 0.0f;
		SceneData.ViewProjMatrix = Scene.ViewProjMatrix;
		Chilli::Renderer::GetBindlessSetManager().UpdateSceneUBO(&SceneData);
	}

	void Draw(const Chilli::Object& Command)
	{
		auto& CurrentMesh = MeshManager.GetMesh(Command.MeshIndex);

		Chilli::RenderCommandSpec Spec{};
		Spec.MaterialIndex = Command.MaterialIndex;
		Spec.ObjectIndex = Command.ID;
		auto Shader = ShaderManager.Get(MaterialManager.Get(Command.MaterialIndex).UsingPipelineID);
		Chilli::Renderer::Submit(Shader, CurrentMesh.VertexBuffers[0], CurrentMesh.IndexBuffer, Spec);
	}

	virtual void Update(Chilli::TimeStep Ts) override
	{
		if (_Minimized)
		{
			CH_CORE_INFO("Minized!");
		}

		UpdateGlobalData();
		Chilli::Renderer::BeginFrame();

		_Camera.Process(Ts.GetSecond());

		DeafultScene.ViewProjMatrix = _Camera.GetProjectionMatrix() * _Camera.GetViewMatrix();
		DeafultScene.CameraPos.x = _Camera.GetPosition().x;
		DeafultScene.CameraPos.y = _Camera.GetPosition().y;
		DeafultScene.CameraPos.z = _Camera.GetPosition().z;
		UpdateSceneData(DeafultScene);

		Chilli::Renderer::BeginScene();
		_Camera.SetProjection(45.0f, (float)GeometryPassResolution.x / (float)GeometryPassResolution.y, 0.1f, 100.0f);

		{
			// Geometry + ZBuffer Pass
			Chilli::ColorAttachment ColorAttachment;
			ColorAttachment.ClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
			ColorAttachment.UseSwapChainImage = false;
			ColorAttachment.ColorTexture = TextureManager.Get(GeometryPassColorTexture);

			Chilli::DepthAttachment DepthAttachment;
			DepthAttachment.DepthTexture = TextureManager.Get(GeometryPassDepthTexture);

			GeoZBufferPass.ColorAttachments = &ColorAttachment;
			GeoZBufferPass.ColorAttachmentCount = 1;
			GeoZBufferPass.DepthAttachment = &DepthAttachment;

			Chilli::Renderer::BeginRenderPass(GeoZBufferPass);
			Draw(CubeObject);
			Chilli::Renderer::EndRenderPass();
		}
		Chilli::Renderer::EndScene();

		Chilli::Renderer::BeginScene();
		{
			// Screen Quad Pass
			Chilli::ColorAttachment ColorAttachment;
			ColorAttachment.ClearColor = { 0.2f, 0.2f, 0.2f, 1.0f };
			ColorAttachment.UseSwapChainImage = true;

			ScreenPass.ColorAttachments = &ColorAttachment;
			ScreenPass.ColorAttachmentCount = 1;

			Chilli::Renderer::BeginRenderPass(ScreenPass);
			Draw(ScreenQuadObject);
			Chilli::Renderer::EndRenderPass();
		}
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
		ShaderManager.Flush();
		MaterialManager.Flush();
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
	bool _Minimized = false;

	GlobalUBO GlobalData;
	Chilli::IVec2 GeometryPassResolution = { 1920, 1080};
	Chilli::Scene DeafultScene, ScreenQuadScene;
	Chilli::Vec2 WindowSize = { 800.0f, 600.0f };

	Chilli::UUID CubeMeshID, SquareMeshID;
	Chilli::UUID GeometryPassDepthTexture, GeometryPassColorTexture;
	Chilli::UUID BrickTextureID, DeafultTextureID;
	Chilli::UUID DeafultSamplerID;
	Chilli::UUID GeometryShaderID, ScreenShaderID;
	Chilli::UUID DeafultMaterialID, CubeMaterialID, ScreenMaterialID;

	Chilli::Object CubeObject, ScreenQuadObject;

	Chilli::MeshManager MeshManager;
	Chilli::SamplerManager SamplerManager;
	Chilli::TextureManager TextureManager;
	Chilli::GraphicsPipelineManager ShaderManager;
	Chilli::MaterialManager MaterialManager;

	Chilli::RenderPassInfo GeoZBufferPass, ScreenPass;

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

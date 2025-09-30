#include "Ch_PCH.h"
#include "Chilli/Chilli.h"

class Editor : public Chilli::Layer
{
public:
	Editor() :Layer("Editor") {}
	~Editor() {}

	virtual void Init() override 
	{
		Chilli::UUID id;
		CH_INFO((uint64_t)id);

		Chilli::GraphicsPipelineSpec Spec{};
		Spec.Paths[0] = "vert.spv";
		Spec.Paths[1] = "frag.spv";
		Shader = Chilli::Renderer::GetResourceFactory()->CreateGraphicsPipeline(Spec);
	}

	virtual void Update() override
	{
		Chilli::Renderer::BeginFrame();
		
		Chilli::Renderer::BeginRenderPass();
		Chilli::Renderer::Submit(Shader);
		Chilli::Renderer::EndRenderPass();

		Chilli::Renderer::RenderFrame();
		Chilli::Renderer::Present();

		Chilli::Renderer::EndFrame();
	}

	virtual void Terminate() override {
		Chilli::Renderer::FinishRendering();
		Chilli::Renderer::GetResourceFactory()->DestroyGraphicsPipeline(Shader);
	}

	virtual void OnEvent(Chilli::Event& e) override {
	}

private:
	std::shared_ptr<Chilli::GraphicsPipeline> Shader;
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

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
	}

	virtual void Terminate() override {
	}

	virtual void Update() override
	{
		if (Chilli::Input::IsKeyPressed(Chilli::Input_key_A) == Chilli::InputResult::INPUT_PRESS)
		{
			CH_INFO(Chilli::Input::KeyToString(Chilli::Input_key_A));
		}
	}

	virtual void OnEvent(Chilli::Event& e) override {
	}

private:
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

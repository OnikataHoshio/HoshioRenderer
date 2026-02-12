#include "KH_Editor.h"
uint32_t KH_Editor::Width = 1920;
uint32_t KH_Editor::Height = 1080;
std::string KH_Editor::Title = "KH_Renderer";

KH_Editor& KH_Editor::Instance()
{
	static KH_Editor instance; 
	return instance;
}

GLFWwindow* KH_Editor::GLFWwindow()
{
	return Window.Window;
}

void KH_Editor::BeginRender()
{
	Window.BeginRender();
}

void KH_Editor::EndRender()
{
	Window.EndRender();
}

KH_Editor::KH_Editor()
	:Camera(Width, Height), Window(Width, Height, Title)
{
	Window.SetCamera(&Camera);

	Initialize();
}

KH_Editor::~KH_Editor()
{
	DeInitialize();
}

void KH_Editor::Initialize()
{

}

void KH_Editor::DeInitialize()
{
	//TODO
}

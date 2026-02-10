#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Object/KH_Object.h"

int main()
{
	KH_Editor::Width = 1920;
	KH_Editor::Height = 1080;
	KH_Editor::Title = "KH_Renderer";
	KH_Editor::Instance();

	KH_Shader TestShader("Assert/Shaders/test.vert", "Assert/Shaders/test.frag");

	while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	{
		KH_Editor::Instance().BeginRender();

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		KH_DefaultModels::Get().Cube.Render(TestShader);

		KH_Editor::Instance().EndRender();
	}

	return 0;
}
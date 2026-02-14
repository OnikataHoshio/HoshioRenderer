#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Object/KH_Object.h"
#include "Hit/KH_BVH.h"

int main()
{
	KH_Editor::Width = 1920;
	KH_Editor::Height = 1080;
	KH_Editor::Title = "KH_Renderer";
	KH_Editor::Instance();

	KH_Shader TestShader("Assert/Shaders/test.vert", "Assert/Shaders/test.frag");
	KH_Shader AABBShader("Assert/Shaders/DrawAABBs.vert", "Assert/Shaders/DrawAABBs.frag");

	KH_Model Bunny;
	Bunny.LoadOBJ("Assert/Models/bunny.obj");

	KH_BVH BVH(1, 8);
	BVH.LoadObj("Assert/Models/bunny.obj");

	while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	{
		KH_Editor::Instance().BeginRender();

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		//KH_DefaultModels::Get().Cube.Render(TestShader);
		//KH_DefaultModels::Get().EmptyCube.Render(TestShader);
		Bunny.Render(TestShader);
		BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));

		KH_Editor::Instance().EndRender();
	}

	return 0;
}
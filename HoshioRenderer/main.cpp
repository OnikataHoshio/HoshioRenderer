#include "Editor/KH_Editor.h"
#include "Pipeline/KH_Shader.h"
#include "Scene/KH_Object.h"
#include "Hit/KH_BVH.h"
#include "Renderer/KH_Renderer.h"
#include "Scene/KH_Scene.h"
#include "Utils/KH_DebugUtils.h"

#include "Scene/KH_Scene.h"

int main()
{
	KH_Editor::Width = 1920;
	KH_Editor::Height = 1080;
	KH_Editor::Title = "KH_Renderer";
	KH_Editor::Instance();

	KH_Shader TestShader("Assert/Shaders/test.vert", "Assert/Shaders/test.frag");
	KH_Shader AABBShader("Assert/Shaders/DrawAABBs.vert", "Assert/Shaders/DrawAABBs.frag");

	//KH_Model Bunny;
	//Bunny.LoadOBJ("Assert/Models/bunny.obj");

	//KH_BVH BVH(11, 1);
	//BVH.BuildMode = KH_BVH_BUILD_MODE::SAH;
	//BVH.LoadObj("Assert/Models/cube.obj");
	//BVH.LoadObj("Assert/Models/bunny.obj");

	KH_RendererBase BaseRenderer;
	BaseRenderer.TraversalMode = KH_PRIMITIVE_TRAVERSAL_MODE::BASE_BVH;
	BaseRenderer.Render(KH_ExampleScene::Instance().ExampleScene1);

	//while (!glfwWindowShouldClose(KH_Editor::Instance().GLFWwindow()))
	//{
	//	KH_Editor::Instance().BeginRender();

	//	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	//	glClear(GL_COLOR_BUFFER_BIT);
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//	//KH_DefaultModels::Get().Cube.Render(TestShader);
	//	//KH_DefaultModels::Get().EmptyCube.Render(TestShader);
	//	Bunny.Render(TestShader);
	//	KH_ExampleScene::Instance().ExampleScene1.BVH.RenderAABB(AABBShader, glm::vec3(1.0f, 1.0f, 1.0f));

	//	ImGui_ImplOpenGL3_NewFrame();
	//	ImGui_ImplGlfw_NewFrame();
	//	ImGui::NewFrame();

	//	{
	//		ImGui::Begin("KH Canvas Editor");
	//		ImGui::BeginChild("CanvasArea", ImVec2((float)KH_Editor::Width/2, (float)KH_Editor::Height/2), 1);

	//		ImVec2 origin = ImGui::GetCursorScreenPos();
	//		ImDrawList* draw_list = ImGui::GetWindowDrawList();
	//	
	//		ImGui::EndChild();
	//		ImGui::End();
	//	}


	//	ImGui::Render();
	//	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	//	KH_Editor::Instance().EndRender();
	//}

	return 0;
}
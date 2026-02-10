#pragma once

#include "KH_Common.h"

class KH_Camera;

class GLFWwindow;

class KH_Window
{
public:
	using InputCallback = std::function<void(GLFWwindow*)>;
	using ResizeCallback = std::function<void(GLFWwindow*, int, int)>;

public:
	KH_Window(uint32_t Width = 1920, uint32_t Height = 1080, std::string Title = "KH_Renderer");
	~KH_Window();

	void AddInputCallback(const InputCallback& callback);
	void AddResizeCallback(const ResizeCallback& callback);

	void SetCamera(KH_Camera* Camera);

	void BeginRender();

	void EndRender();

	uint32_t Width;
	uint32_t Height;
	std::string Title;

	GLFWwindow* Window;

private:
	void Initialize();
	void DeInitialize();

	void ProcessData();
	void ProcessInput(GLFWwindow* window);


	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

	static void MouseMovementCallback(GLFWwindow* window, double xposIn, double yposIn);

	static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
private:
	float LastFrame = 0.0f;
	float DeltaTime = 0.0f;

	static KH_Camera* Camera;


	std::vector<InputCallback> M_InputCallbacks;
	std::vector<ResizeCallback> M_ResizeCallbacks;
};

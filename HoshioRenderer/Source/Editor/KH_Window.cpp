#include "KH_Window.h"
#include "KH_Camera.h"

KH_Camera* KH_Window::Camera = nullptr;

KH_Window::KH_Window(uint32_t Width, uint32_t Height, std::string Title)
	:Width(Width), Height(Height), Title(Title)
{
	Initialize();
}

KH_Window::~KH_Window()
{
	DeInitialize();
}

void KH_Window::AddInputCallback(const InputCallback& callback)
{
    M_InputCallbacks.push_back(callback);
}

void KH_Window::AddResizeCallback(const ResizeCallback& callback)
{
    M_ResizeCallbacks.push_back(callback);
}

void KH_Window::BeginRender()
{
    ProcessData();
    ProcessInput(Window);
}

void KH_Window::EndRender()
{
    glfwSwapBuffers(Window);
    glfwPollEvents();
}

void KH_Window::Initialize()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    Window = glfwCreateWindow(
        Width, Height,
        Title.c_str(),
        nullptr, nullptr
    );

    if (Window == nullptr)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
    }

    glfwMakeContextCurrent(Window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    glViewport(0, 0, Width, Height);
    glfwSetWindowUserPointer(Window, this);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(Window, MouseMovementCallback);
    glfwSetScrollCallback(Window, MouseScrollCallback);

    glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);
}

void KH_Window::DeInitialize()
{
    glfwDestroyWindow(Window);
    glfwTerminate();
}

void KH_Window::ProcessData()
{
    float CurrentFrame = static_cast<float>(glfwGetTime());
    DeltaTime = CurrentFrame - LastFrame;
    LastFrame = CurrentFrame;
}

void KH_Window::ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Camera->ProcessKeyboard(CameraMovement::Forward, DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Camera->ProcessKeyboard(CameraMovement::Left, DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Camera->ProcessKeyboard(CameraMovement::Backward, DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Camera->ProcessKeyboard(CameraMovement::Right, DeltaTime);

    auto* self = static_cast<KH_Window*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    for (const auto& callback : self->M_InputCallbacks)
    {
        callback(window);
    }
}

void KH_Window::SetCamera(KH_Camera* Camera)
{
    this->Camera = Camera;
}



void KH_Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    auto* self = static_cast<KH_Window*>(glfwGetWindowUserPointer(window));
    if (!self) return;

    for (const auto& callback : self->M_ResizeCallbacks)
    {
        callback(window, width, height);
    }
}

void KH_Window::MouseMovementCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    static float LastMouseX = 0.0f;
    static float LastMouseY = 0.0f;
    static bool bFirstMouse = true;

    if (!Camera) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (bFirstMouse)
    {
        LastMouseX = xpos;
        LastMouseY = ypos;
        bFirstMouse = false;
        return; 
    }

    float OffsetX = xpos - LastMouseX;
    float OffsetY = LastMouseY - ypos; // 注意这里反过来

    LastMouseX = xpos;
    LastMouseY = ypos;

    Camera->ProcessMouseMovement(OffsetX, OffsetY);
}

void KH_Window::MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Camera->ProcessMouseScroll(yoffset);
}

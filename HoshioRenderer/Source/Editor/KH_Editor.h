#pragma once
#include "KH_Camera.h"
#include "KH_Window.h"


class KH_Editor
{
public:
    static KH_Editor& Instance();

    KH_Editor(const KH_Editor&) = delete;
    KH_Editor& operator=(const KH_Editor&) = delete;

    void BeginRender();
    void EndRender();

    GLFWwindow* GLFWwindow();

    KH_Camera Camera;
    KH_Window Window;

    static uint32_t Width;
    static uint32_t Height;
    static std::string Title;

private:
    KH_Editor();  
    ~KH_Editor();

    void Initialize();
    void DeInitialize();
};




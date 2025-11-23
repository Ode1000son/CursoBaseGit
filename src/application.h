#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "input_controller.h"
#include "renderer.h"
#include "renderer_controller.h"
#include "scene.h"

struct ApplicationConfig
{
    int width = 1280;
    int height = 720;
    const char* title = "Aula 8.4 - Application Host";
};

class Application
{
public:
    explicit Application(const ApplicationConfig& config);
    ~Application();

    int Run();

private:
    bool Initialize();
    void Shutdown();

    ApplicationConfig m_config;
    GLFWwindow* m_window = nullptr;
    Camera m_camera;
    Scene m_scene;
    Renderer m_renderer;
    InputController m_inputController;
    RendererController m_rendererController;
    float m_lastFrame = 0.0f;
};


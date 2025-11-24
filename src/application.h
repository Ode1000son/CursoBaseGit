#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"
#include "input_controller.h"
#include "renderer.h"
#include "renderer_controller.h"
#include "scene.h"
#include "physics_system.h"

struct ApplicationConfig
{
    int width = 1280;
    int height = 720;
    const char* title = "Aula 10.1 - Engine Completa";
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
    void SetupDebugOutput();
    void ForwardDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, const std::string& message);
    static void APIENTRY OpenGLDebugCallback(GLenum source,
                                             GLenum type,
                                             GLuint id,
                                             GLenum severity,
                                             GLsizei length,
                                             const GLchar* message,
                                             const void* userParam);
    void ProcessHotkeys();
    bool ReloadSceneKeepingCamera();

    ApplicationConfig m_config;
    GLFWwindow* m_window = nullptr;
    Camera m_camera;
    Scene m_scene;
    Renderer m_renderer;
    PhysicsSystem m_physicsSystem;
    InputController m_inputController;
    RendererController m_rendererController;
    float m_lastFrame = 0.0f;
    bool m_debugOutputEnabled = false;
    bool m_f4Held = false;
    bool m_f5Held = false;
};


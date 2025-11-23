#include "application.h"

#include <iostream>

Application::Application(const ApplicationConfig& config)
    : m_config(config)
    , m_camera(glm::vec3(0.0f, 2.0f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f)
{
}

Application::~Application()
{
    Shutdown();
}

int Application::Run()
{
    if (!Initialize())
    {
        Shutdown();
        return -1;
    }

    while (!glfwWindowShouldClose(m_window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        m_inputController.ProcessInput(deltaTime);
        m_rendererController.ProcessShortcuts(m_window);
        m_renderer.RenderFrame(m_window, m_camera, currentFrame);

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    Shutdown();
    return 0;
}

bool Application::Initialize()
{
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW." << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_config.width, m_config.height, m_config.title, nullptr, nullptr);
    if (!m_window)
    {
        std::cerr << "Falha ao criar janela GLFW." << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Falha ao inicializar GLAD." << std::endl;
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
        return false;
    }

    if (!m_scene.Initialize())
    {
        std::cerr << "Falha ao inicializar a cena." << std::endl;
        return false;
    }

    if (!m_renderer.Initialize(&m_scene))
    {
        std::cerr << "Falha ao inicializar o renderer." << std::endl;
        return false;
    }

    m_inputController.Initialize(&m_camera);
    m_inputController.AttachWindow(m_window);

    m_rendererController.Initialize(&m_renderer);
    m_lastFrame = static_cast<float>(glfwGetTime());

    return true;
}

void Application::Shutdown()
{
    m_renderer.Shutdown();

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}


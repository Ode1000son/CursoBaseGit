#include "application.h"

#include <iostream>
#include <cstring>

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
        ProcessHotkeys();
        m_physicsSystem.Simulate(deltaTime, m_scene);
        m_renderer.RenderFrame(m_window, m_camera, currentFrame, deltaTime);

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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

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

    SetupDebugOutput();

    if (!m_scene.Initialize())
    {
        std::cerr << "Falha ao inicializar a cena." << std::endl;
        return false;
    }

    if (!m_physicsSystem.Initialize())
    {
        std::cerr << "Falha ao inicializar o PhysX." << std::endl;
        return false;
    }

    if (!m_physicsSystem.BuildFromScene(m_scene))
    {
        std::cerr << "Falha ao configurar o mundo físico." << std::endl;
        return false;
    }

    if (!m_renderer.Initialize(&m_scene, &m_physicsSystem))
    {
        std::cerr << "Falha ao inicializar o renderer." << std::endl;
        return false;
    }
    m_renderer.SetWindowTitleBase(m_config.title);

    const SceneCameraSettings& cameraSettings = m_scene.GetCameraSettings();
    m_camera.SetPosition(cameraSettings.position);
    m_camera.SetUp(cameraSettings.up);
    m_camera.SetOrientation(cameraSettings.yaw, cameraSettings.pitch);
    m_camera.SetMovementSpeed(cameraSettings.movementSpeed);
    m_camera.SetMouseSensitivity(cameraSettings.mouseSensitivity);
    m_camera.SetZoom(cameraSettings.zoom);

    m_inputController.Initialize(&m_camera);
    m_inputController.AttachWindow(m_window);

    m_rendererController.Initialize(&m_renderer);
    m_lastFrame = static_cast<float>(glfwGetTime());

    return true;
}

void Application::Shutdown()
{
    m_physicsSystem.Shutdown();
    m_renderer.Shutdown();

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

void Application::SetupDebugOutput()
{
    if (m_debugOutputEnabled)
    {
        return;
    }

    if (glDebugMessageCallback == nullptr)
    {
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(&Application::OpenGLDebugCallback, this);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    m_debugOutputEnabled = true;
}

void Application::ForwardDebugMessage(GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      const std::string& message)
{
    m_renderer.PushDebugMessage(source, type, id, severity, message);
}

void APIENTRY Application::OpenGLDebugCallback(GLenum source,
                                              GLenum type,
                                              GLuint id,
                                              GLenum severity,
                                              GLsizei length,
                                              const GLchar* message,
                                              const void* userParam)
{
    auto* app = reinterpret_cast<Application*>(const_cast<void*>(userParam));
    if (!app)
    {
        return;
    }

    std::string text;
    if (message != nullptr)
    {
        if (length > 0)
        {
            text.assign(message, static_cast<std::size_t>(length));
        }
        else
        {
            text.assign(message);
        }
    }

    app->ForwardDebugMessage(source, type, id, severity, text);
}

void Application::ProcessHotkeys()
{
    if (!m_window)
    {
        return;
    }

    auto handleToggle = [&](int key, bool& heldState, auto&& action) {
        const bool pressed = glfwGetKey(m_window, key) == GLFW_PRESS;
        if (pressed && !heldState)
        {
            action();
        }
        heldState = pressed;
    };

    handleToggle(GLFW_KEY_F4, m_f4Held, [&]() {
        const bool enabled = !m_physicsSystem.IsDebugRenderingEnabled();
        m_physicsSystem.SetDebugRenderingEnabled(enabled);
        m_renderer.PushOverlayStatus(enabled ? "Debug de colisão ativado (F4)" : "Debug de colisão desativado (F4)");
    });

    handleToggle(GLFW_KEY_F5, m_f5Held, [&]() {
        if (ReloadSceneKeepingCamera())
        {
            m_renderer.PushOverlayStatus("Cena recarregada (F5)");
        }
        else
        {
            m_renderer.PushOverlayStatus("Falha ao recarregar a cena (F5)");
        }
    });
}

bool Application::ReloadSceneKeepingCamera()
{
    const glm::vec3 savedPosition = m_camera.GetPosition();
    const glm::vec3 savedUp = m_camera.GetUpVector();
    const float savedYaw = m_camera.GetYaw();
    const float savedPitch = m_camera.GetPitch();
    const float savedSpeed = m_camera.GetMovementSpeed();
    const float savedSensitivity = m_camera.GetMouseSensitivity();
    const float savedZoom = m_camera.GetZoom();

    if (!m_scene.Reload())
    {
        std::cerr << "Falha ao recarregar definição da cena." << std::endl;
        return false;
    }

    if (!m_physicsSystem.BuildFromScene(m_scene))
    {
        std::cerr << "Falha ao reconstruir o mundo físico após recarregar a cena." << std::endl;
        return false;
    }

    m_camera.SetPosition(savedPosition);
    m_camera.SetUp(savedUp);
    m_camera.SetOrientation(savedYaw, savedPitch);
    m_camera.SetMovementSpeed(savedSpeed);
    m_camera.SetMouseSensitivity(savedSensitivity);
    m_camera.SetZoom(savedZoom);

    return true;
}


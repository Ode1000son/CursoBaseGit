// Implementação da classe Application - gerencia o ciclo de vida completo
// da aplicação, incluindo inicialização de sistemas, loop principal e shutdown.

#include "application.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

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
        m_renderer.RenderFrame(m_window, m_camera, currentFrame, deltaTime);
        UpdateAudio(deltaTime);

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
    m_characterAudioObject = m_scene.GetCharacterObject();
    m_vehicleAudioObject = m_scene.GetCarObject();

    if (!m_renderer.Initialize(&m_scene))
    {
        std::cerr << "Falha ao inicializar o renderer." << std::endl;
        return false;
    }
    m_renderer.SetWindowTitleBase(m_config.title);

    if (!InitializeAudioSystem())
    {
        std::cerr << "Falha ao inicializar o sistema de áudio." << std::endl;
        return false;
    }
    if (m_characterAudioObject != nullptr)
    {
        m_audioSystem.UpdateEmitterPosition("hero_beacon", m_characterAudioObject->GetWorldCenter());
    }
    if (m_vehicleAudioObject != nullptr)
    {
        m_audioSystem.UpdateEmitterPosition("car_engine_loop", m_vehicleAudioObject->GetWorldCenter());
    }

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
    if (m_shutdownPerformed)
    {
        return;
    }
    m_shutdownPerformed = true;

    m_renderer.Shutdown();

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
    m_audioSystem.Shutdown();
}

bool Application::InitializeAudioSystem()
{
    AudioSystemConfig audioConfig;
    audioConfig.assetsRoot = "assets";
    audioConfig.configPath = "assets/scenes/audio_config.json";
    audioConfig.globalVolume = 0.75f;
    return m_audioSystem.Initialize(audioConfig);
}

void Application::UpdateAudio(float)
{
    if (!m_audioSystem.IsInitialized())
    {
        return;
    }

    m_audioSystem.UpdateListener(m_camera.GetPosition(), m_camera.GetFront(), m_camera.GetUp());

    if (m_characterAudioObject != nullptr)
    {
        m_audioSystem.UpdateEmitterPosition("hero_beacon", m_characterAudioObject->GetWorldCenter());
    }
    if (m_vehicleAudioObject != nullptr)
    {
        m_audioSystem.UpdateEmitterPosition("car_engine_loop", m_vehicleAudioObject->GetWorldCenter());
    }

    HandleAudioShortcuts();
    HandleOneShotTrigger();
}

void Application::HandleAudioShortcuts()
{
    if (!m_audioSystem.IsInitialized() || !m_window)
    {
        return;
    }

    auto notify = [this](float volume)
    {
        std::ostringstream stream;
        stream << "Volume global de áudio: " << static_cast<int>(volume * 100.0f) << "%";
        m_renderer.PushDebugMessage(GL_DEBUG_SOURCE_APPLICATION,
                                    GL_DEBUG_TYPE_MARKER,
                                    0,
                                    GL_DEBUG_SEVERITY_NOTIFICATION,
                                    stream.str());
    };

    const float step = 0.05f;
    const int increaseState = glfwGetKey(m_window, GLFW_KEY_RIGHT_BRACKET);
    if (increaseState == GLFW_PRESS)
    {
        if (!m_volumeUpPressed)
        {
            m_volumeUpPressed = true;
            const float volume = std::clamp(m_audioSystem.GetGlobalVolume() + step, 0.0f, 1.0f);
            m_audioSystem.SetGlobalVolume(volume);
            notify(volume);
        }
    }
    else
    {
        m_volumeUpPressed = false;
    }

    const int decreaseState = glfwGetKey(m_window, GLFW_KEY_LEFT_BRACKET);
    if (decreaseState == GLFW_PRESS)
    {
        if (!m_volumeDownPressed)
        {
            m_volumeDownPressed = true;
            const float volume = std::clamp(m_audioSystem.GetGlobalVolume() - step, 0.0f, 1.0f);
            m_audioSystem.SetGlobalVolume(volume);
            notify(volume);
        }
    }
    else
    {
        m_volumeDownPressed = false;
    }
}

void Application::HandleOneShotTrigger()
{
    if (!m_audioSystem.IsInitialized() || !m_window)
    {
        return;
    }

    const int triggerState = glfwGetKey(m_window, GLFW_KEY_SPACE);
    if (triggerState == GLFW_PRESS)
    {
        if (!m_triggerBonusPressed)
        {
            m_audioSystem.PlayOneShot("hero_beacon");
            m_triggerBonusPressed = true;
        }
    }
    else
    {
        m_triggerBonusPressed = false;
    }
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


#pragma once

// Classe principal da aplicação que gerencia o loop de renderização,
// inicialização de sistemas (GLFW, OpenGL, áudio) e coordenação entre
// câmera, cena, renderer e sistema de áudio 3D.

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "audio_system.h"
#include "camera.h"
#include "input_controller.h"
#include "renderer.h"
#include "renderer_controller.h"
#include "scene.h"

// Configuração inicial da aplicação (resolução, título da janela)
struct ApplicationConfig
{
    int width = 1280;
    int height = 720;
    const char* title = "Aula 10.2 - Sistema de Áudio com MiniAudio";
};

// Classe principal que gerencia o ciclo de vida da aplicação
// Responsável por inicializar GLFW, OpenGL, sistemas de renderização e áudio,
// executar o loop principal e coordenar atualizações entre sistemas.
class Application
{
public:
    explicit Application(const ApplicationConfig& config);
    ~Application();

    // Executa o loop principal da aplicação até o fechamento da janela
    // Retorna código de saída (0 = sucesso, -1 = erro na inicialização)
    int Run();

private:
    // Inicializa todos os subsistemas (GLFW, OpenGL, cena, renderer, áudio)
    bool Initialize();
    // Inicializa o sistema de áudio com configuração do arquivo JSON
    bool InitializeAudioSystem();
    // Atualiza posições do listener e emissores de áudio a cada frame
    void UpdateAudio(float deltaTime);
    // Processa atalhos de teclado para controle de volume ([ e ])
    void HandleAudioShortcuts();
    // Processa trigger de som one-shot (tecla Space)
    void HandleOneShotTrigger();
    // Libera recursos e finaliza todos os sistemas
    void Shutdown();
    // Configura callback de debug do OpenGL para capturar mensagens
    void SetupDebugOutput();
    // Encaminha mensagem de debug do OpenGL para o renderer
    void ForwardDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, const std::string& message);
    // Callback estático do OpenGL para capturar mensagens de debug
    static void APIENTRY OpenGLDebugCallback(GLenum source,
                                             GLenum type,
                                             GLuint id,
                                             GLenum severity,
                                             GLsizei length,
                                             const GLchar* message,
                                             const void* userParam);

    ApplicationConfig m_config;
    GLFWwindow* m_window = nullptr;
    Camera m_camera;
    Scene m_scene;
    Renderer m_renderer;
    InputController m_inputController;
    RendererController m_rendererController;
    AudioSystem m_audioSystem;
    SceneObject* m_characterAudioObject = nullptr;
    SceneObject* m_vehicleAudioObject = nullptr;
    bool m_triggerBonusPressed = false;
    bool m_volumeUpPressed = false;
    bool m_volumeDownPressed = false;
    bool m_shutdownPerformed = false;
    float m_lastFrame = 0.0f;
    bool m_debugOutputEnabled = false;
};


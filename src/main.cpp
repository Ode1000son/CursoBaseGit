#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

/// @brief Callback chamado quando a janela é redimensionada
/// @param window Ponteiro para a janela GLFW
/// @param width Nova largura da janela
/// @param height Nova altura da janela
void FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

/// @brief Processa entrada do teclado
/// @param window Ponteiro para a janela GLFW
void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

/// @brief Função principal da Aula 1.1 - Configuração e Janela OpenGL
/// Esta função demonstra o bootstrap básico para criar uma janela OpenGL 3.3 Core
/// usando GLFW e GLAD no Windows. O programa cria uma janela, inicializa o contexto
/// OpenGL e mantém um loop principal até que ESC seja pressionado.
/// @return 0 em caso de sucesso, -1 em caso de erro
int main()
{
    // === INICIALIZAÇÃO DO GLFW ===
    // GLFW é a biblioteca responsável por criar janelas e gerenciar entrada
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    // === CONFIGURAÇÃO DO CONTEXTO OPENGL ===
    // Define que queremos usar OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // === CRIAÇÃO DA JANELA ===
    // Cria uma janela de 1280x720 pixels com título específico
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aula 1.1 - Janela OpenGL", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // === CONFIGURAÇÃO DO CONTEXTO ===
    // Torna o contexto da janela atual no thread atual
    glfwMakeContextCurrent(window);
    // Registra callback para quando a janela for redimensionada
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // === INICIALIZAÇÃO DO GLAD ===
    // GLAD carrega os ponteiros para as funções OpenGL
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // === CONFIGURAÇÃO DO ESTADO OPENGL ===
    // Define a área de renderização (viewport) inicial
    glViewport(0, 0, 1280, 720);
    // Define a cor de limpeza da tela (azul escuro)
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // === LOOP PRINCIPAL DA APLICAÇÃO ===
    // Mantém a aplicação rodando até que a janela seja fechada
    while (!glfwWindowShouldClose(window))
    {
        // Processa entrada do usuário (teclado, mouse, etc.)
        ProcessInput(window);

        // Limpa o buffer de cor com a cor definida
        glClear(GL_COLOR_BUFFER_BIT);

        // Troca os buffers (double buffering) para evitar flickering
        glfwSwapBuffers(window);

        // Processa eventos da janela (redimensionamento, fechamento, etc.)
        glfwPollEvents();
    }

    // === LIMPEZA ===
    // Destroi a janela e finaliza o GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


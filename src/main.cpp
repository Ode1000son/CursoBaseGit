#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "camera.h"
#include "texture.h"
#include "model.h"

// === VARIÁVEIS GLOBAIS PARA CONTROLE DA CÂMERA ===
Camera camera;  // Instância da câmera
bool firstMouse = true;  // Controle para primeira captura do mouse
float lastX = 400.0f;    // Última posição X do mouse
float lastY = 300.0f;    // Última posição Y do mouse

// === ESTRUTURAS DE SHADER ===
struct ShaderProgram {
    GLuint program = 0;

    void Create(const std::string& vertexPath, const std::string& fragmentPath) {
        // Carrega e compila os shaders
        GLuint vertexShader = LoadAndCompileShader(vertexPath, GL_VERTEX_SHADER);
        GLuint fragmentShader = LoadAndCompileShader(fragmentPath, GL_FRAGMENT_SHADER);

        // Cria o programa e linka
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        // Verifica erros de linkagem
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Erro ao linkar programa shader: " << infoLog << std::endl;
        }

        // Limpa shaders temporários
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Use() const {
        glUseProgram(program);
    }

    void Delete() {
        if (program) {
            glDeleteProgram(program);
            program = 0;
        }
    }

private:
    GLuint LoadAndCompileShader(const std::string& path, GLenum type) {
        // Carrega o código fonte do shader
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir arquivo shader: " << path << std::endl;
            return 0;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string source = buffer.str();

        // Cria e compila o shader
        GLuint shader = glCreateShader(type);
        const char* sourcePtr = source.c_str();
        glShaderSource(shader, 1, &sourcePtr, nullptr);
        glCompileShader(shader);

        // Verifica erros de compilação
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Erro ao compilar shader " << path << ": " << infoLog << std::endl;
        }

        return shader;
    }
};

/// @brief Callback chamado quando a janela é redimensionada
/// @param window Ponteiro para a janela GLFW
/// @param width Nova largura da janela
/// @param height Nova altura da janela
void FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

/// @brief Processa entrada do teclado (WASD + ESC)
/// @param window Ponteiro para a janela GLFW
/// @param deltaTime Tempo decorrido desde o último frame
void ProcessInput(GLFWwindow* window, float deltaTime)
{
    // ESC para fechar
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Movimentação WASD + QE (cima/baixo)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
}

/// @brief Callback para movimento do mouse
/// @param window Ponteiro para a janela GLFW
/// @param xpos Posição X atual do mouse
/// @param ypos Posição Y atual do mouse
void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    static_cast<void>(window); // Evita warning de parâmetro não usado

    // Verificar se o botão direito está pressionado
    bool rightButtonPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    // Capturar ou liberar o cursor baseado no botão direito
    if (rightButtonPressed) {
        // Capturar cursor quando botão direito for pressionado
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (firstMouse) {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }

        // Calcula o deslocamento do mouse
        float xoffset = static_cast<float>(xpos) - lastX;
        float yoffset = lastY - static_cast<float>(ypos); // Invertido

        // Passa o movimento para a câmera
        camera.ProcessMouseMovement(xoffset, yoffset);
    } else {
        // Liberar cursor quando botão direito for solto
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true; // Reset para evitar saltos na próxima captura
    }

    // Sempre atualizar a posição do mouse para evitar saltos quando o botão for pressionado
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
}

/// @brief Função principal da Aula 3.2 - Modelos 3D Simples
/// Demonstra como carregar um modelo glTF com Assimp e renderizá-lo com texturas.
/// @return 0 em caso de sucesso, -1 em caso de erro
int main()
{
    // === INICIALIZAÇÃO DO GLFW ===
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    // === CONFIGURAÇÃO DO CONTEXTO OPENGL ===
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // === CRIAÇÃO DA JANELA ===
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aula 3.2 - Modelos 3D Simples", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // === CONFIGURAÇÃO DO CONTEXTO ===
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // === CONFIGURAÇÃO DO MOUSE ===
    // Cursor normal inicialmente (só captura com botão direito)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, MouseCallback);

    // === INICIALIZAÇÃO DO GLAD ===
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // === CRIAÇÃO DOS SHADERS ===
    ShaderProgram shaderProgram;
    shaderProgram.Create("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

    // === CARREGAMENTO DO MODELO 3D ===
    Model characterModel;
    if (!characterModel.LoadFromFile("assets/models/scene.gltf") || !characterModel.HasMeshes()) {
        std::cerr << "Falha ao carregar modelo 3D (scene.gltf)." << std::endl;
        return -1;
    }

    // === TEXTURA PADRÃO DE FALLBACK ===
    Texture fallbackTexture;
    if (!fallbackTexture.LoadFromFile("assets/models/Vitalik_edit_2.png")) {
        std::cerr << "Falha ao carregar textura padrão de fallback." << std::endl;
        return -1;
    }

    // === CONFIGURAÇÃO DAS MATRIZES MVP ===
    // Matriz Model: transforma o objeto local para o mundo
    glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                           1280.0f / 720.0f,
                                           0.1f,
                                           100.0f);

    // Habilitar depth testing para renderização 3D
    glEnable(GL_DEPTH_TEST);

    // === LOOP PRINCIPAL DA APLICAÇÃO ===
    // Variáveis para controle de tempo
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        // Calcular delta time para movimento suave
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Processar entrada do usuário (WASD + mouse)
        ProcessInput(window, deltaTime);

        // Limpa os buffers de cor e profundidade
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === RENDERIZAÇÃO DO MODELO 3D ===
        shaderProgram.Use();

        glm::mat4 modelMatrix = glm::mat4(1.0f);

        glm::mat4 view = camera.GetViewMatrix();

        const GLuint modelLoc = glGetUniformLocation(shaderProgram.program, "model");
        const GLuint viewLoc = glGetUniformLocation(shaderProgram.program, "view");
        const GLuint projectionLoc = glGetUniformLocation(shaderProgram.program, "projection");
        const GLuint lightDirLoc = glGetUniformLocation(shaderProgram.program, "lightDirection");
        const GLuint viewPosLoc = glGetUniformLocation(shaderProgram.program, "viewPos");
        const GLuint samplerLoc = glGetUniformLocation(shaderProgram.program, "textureSampler");

        glm::vec3 lightDirection = glm::normalize(glm::vec3(-0.45f, -1.0f, -0.35f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.GetPosition()));
        glUniform1i(samplerLoc, 0);

        characterModel.Draw(fallbackTexture.GetID());

        // === GERENCIAMENTO DE BUFFERS ===
        // Trocar os buffers (double buffering) para exibir o frame renderizado
        glfwSwapBuffers(window);
        // Processar eventos da janela (teclado, mouse, redimensionamento, etc.)
        glfwPollEvents();
    }

    shaderProgram.Delete();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

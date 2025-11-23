#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include "camera.h"
#include "texture.h"
#include "model.h"
#include "material.h"
#include "light_manager.h"

constexpr int kMaxDirectionalLights = 4;
constexpr int kMaxPointLights = 4;
constexpr unsigned int kShadowMapWidth = 2048;
constexpr unsigned int kShadowMapHeight = 2048;
constexpr unsigned int kPointShadowSize = 1024;
constexpr float kShadowNearPlane = 1.0f;
constexpr float kShadowFarPlane = 60.0f;
constexpr float kPointShadowNearPlane = 0.1f;
constexpr float kPointShadowFarPlane = 35.0f;

// === VARIÁVEIS GLOBAIS PARA CONTROLE DA CÂMERA ===
Camera camera(glm::vec3(0.0f, 2.0f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f);  // Câmera posicionada mais acima apontando para o centro
bool firstMouse = true;  // Controle para primeira captura do mouse
float lastX = 400.0f;    // Última posição X do mouse
float lastY = 300.0f;    // Última posição Y do mouse

// === ESTRUTURAS DE SHADER ===
struct ShaderProgram {
    GLuint program = 0;

    void Create(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath = "") {
        // Carrega e compila os shaders
        GLuint vertexShader = LoadAndCompileShader(vertexPath, GL_VERTEX_SHADER);
        GLuint fragmentShader = LoadAndCompileShader(fragmentPath, GL_FRAGMENT_SHADER);
        GLuint geometryShader = 0;
        if (!geometryPath.empty()) {
            geometryShader = LoadAndCompileShader(geometryPath, GL_GEOMETRY_SHADER);
        }

        // Cria o programa e linka
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        if (geometryShader != 0) {
            glAttachShader(program, geometryShader);
        }
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
        if (geometryShader != 0) {
            glDeleteShader(geometryShader);
        }
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

GLuint CreateSolidColorTexture(const glm::vec4& color)
{
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    std::array<unsigned char, 4> pixel{
        static_cast<unsigned char>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f)
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

enum class TextureOverrideMode
{
    Imported = 0,
    Checker,
    Highlight
};

void ApplyOverrideMode(TextureOverrideMode mode,
                       Model& characterModel,
                       Model& floorModel,
                       Texture* checkerTexture,
                       Texture* highlightTexture)
{
    characterModel.ClearTextureOverrides();
    floorModel.ClearTextureOverrides();

    auto applyTexture = [&](Texture* texture) {
        if (texture) {
            characterModel.OverrideAllTextures(texture);
            floorModel.OverrideAllTextures(texture);
        }
    };

    switch (mode) {
    case TextureOverrideMode::Checker:
        applyTexture(checkerTexture);
        break;
    case TextureOverrideMode::Highlight:
        applyTexture(highlightTexture);
        break;
    default:
        break;
    }
}

void HandleOverrideShortcuts(GLFWwindow* window,
                             TextureOverrideMode& currentMode,
                             Model& characterModel,
                             Model& floorModel,
                             Texture* checkerTexture,
                             Texture* highlightTexture)
{
    static bool key1Held = false;
    static bool key2Held = false;
    static bool key3Held = false;

    auto handleKey = [&](int key, bool& heldState, TextureOverrideMode mode) {
        bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;
        if (isPressed && !heldState && currentMode != mode) {
            currentMode = mode;
            ApplyOverrideMode(currentMode, characterModel, floorModel, checkerTexture, highlightTexture);
        }
        heldState = isPressed;
    };

    handleKey(GLFW_KEY_1, key1Held, TextureOverrideMode::Imported);
    handleKey(GLFW_KEY_2, key2Held, TextureOverrideMode::Checker);
    handleKey(GLFW_KEY_3, key3Held, TextureOverrideMode::Highlight);
}

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

/// @brief Função principal da Aula 7.1 - Materiais e Texturas
/// Demonstra carregamento de materiais, múltiplas texturas por mesh e sistema de overrides.
/// @return 0 em caso de sucesso, -1 em caso de erro
int main()
{
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aula 7.1 - Materiais e Texturas", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, MouseCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    const GLuint defaultWhiteTexture = CreateSolidColorTexture(glm::vec4(1.0f));

    ShaderProgram shaderProgram;
    shaderProgram.Create("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl");

    ShaderProgram directionalDepthShader;
    directionalDepthShader.Create("assets/shaders/directional_depth_vertex.glsl",
                                  "assets/shaders/directional_depth_fragment.glsl");

    ShaderProgram pointDepthShader;
    pointDepthShader.Create("assets/shaders/depth_vertex.glsl",
                            "assets/shaders/depth_fragment.glsl",
                            "assets/shaders/depth_geometry.glsl");

    Model characterModel;
    if (!characterModel.LoadFromFile("assets/models/scene.gltf") || !characterModel.HasMeshes()) {
        std::cerr << "Falha ao carregar modelo 3D (scene.gltf)." << std::endl;
        return -1;
    }

    Model floorModel;
    if (!floorModel.LoadFromFile("assets/models/cube.gltf") || !floorModel.HasMeshes()) {
        std::cerr << "Falha ao carregar modelo do chão (cube.gltf)." << std::endl;
        return -1;
    }

    Texture characterTexture;
    if (!characterTexture.LoadFromFile("assets/models/Vitalik_edit_2.png")) {
        std::cerr << "Falha ao carregar textura do personagem (Vitalik_edit_2.png)." << std::endl;
    }
    Texture floorTexture;
    if (!floorTexture.LoadFromFile("assets/models/CubeTexture.jpg")) {
        std::cerr << "Falha ao carregar textura do chão (CubeTexture.jpg)." << std::endl;
    }
    Texture checkerTexture;
    if (!checkerTexture.LoadFromFile("assets/texture.png")) {
        std::cerr << "Falha ao carregar textura checker para overrides." << std::endl;
    }
    Texture highlightTexture;
    if (!highlightTexture.LoadFromFile("assets/models/CubeTexture.jpg")) {
        std::cerr << "Falha ao carregar textura de destaque para overrides." << std::endl;
    }

    if (characterTexture.GetID() != 0) {
        characterModel.ApplyTextureIfMissing(&characterTexture);
    }
    if (floorTexture.GetID() != 0) {
        floorModel.ApplyTextureIfMissing(&floorTexture);
    }

    Texture* checkerTexturePtr = checkerTexture.GetID() != 0 ? &checkerTexture : nullptr;
    Texture* highlightTexturePtr = highlightTexture.GetID() != 0 ? &highlightTexture : nullptr;
    TextureOverrideMode overrideMode = TextureOverrideMode::Imported;
    ApplyOverrideMode(overrideMode, characterModel, floorModel, checkerTexturePtr, highlightTexturePtr);

    DirectionalLight mainSun{
        glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)),
        glm::vec3(0.25f, 0.22f, 0.20f),
        glm::vec3(0.9f, 0.85f, 0.8f),
        glm::vec3(1.0f),
        false,
        glm::vec3(0.0f),
        0.0f
    };

    DirectionalLightManager directionalLights(kMaxDirectionalLights);
    directionalLights.AddLight(mainSun);
    directionalLights.AddLight({ glm::normalize(glm::vec3(0.3f, -1.0f, 0.15f)),
                                 glm::vec3(0.02f, 0.02f, 0.03f),
                                 glm::vec3(0.35f, 0.4f, 0.55f),
                                 glm::vec3(0.25f, 0.3f, 0.45f),
                                 false });

    PointLightManager pointLights(kMaxPointLights);
    const int shadowPointIndex = pointLights.AddLight({
        glm::vec3(0.0f, 2.8f, 0.0f),
        glm::vec3(0.03f, 0.03f, 0.03f),
        glm::vec3(1.0f, 0.85f, 0.6f),
        glm::vec3(1.0f, 0.95f, 0.9f),
        1.0f,
        0.09f,
        0.032f,
        18.0f
    });
    pointLights.AddLight({
        glm::vec3(-3.0f, 3.5f, -2.0f),
        glm::vec3(0.04f, 0.05f, 0.06f),
        glm::vec3(0.55f, 0.65f, 1.0f),
        glm::vec3(0.35f, 0.40f, 0.55f),
        1.0f,
        0.14f,
        0.07f,
        12.0f
    });

    if (shadowPointIndex < 0) {
        std::cerr << "Falha ao registrar a luz pontual que gera sombras." << std::endl;
        return -1;
    }

    const glm::vec3 pointLightOrbitCenter(0.0f, 1.8f, 0.0f);
    const float pointLightOrbitRadius = 3.8f;
    const float pointLightVerticalAmplitude = 0.7f;

    GLuint depthMapFBO = 0;
    glGenFramebuffers(1, &depthMapFBO);

    GLuint depthMap = 0;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapWidth, kShadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Erro ao configurar framebuffer de depth para shadow mapping." << std::endl;
        return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GLuint pointDepthMapFBO = 0;
    glGenFramebuffers(1, &pointDepthMapFBO);

    GLuint pointDepthCubemap = 0;
    glGenTextures(1, &pointDepthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0,
                     GL_DEPTH_COMPONENT,
                     kPointShadowSize,
                     kPointShadowSize,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT,
                     nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointDepthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Erro ao configurar framebuffer de depth para point light shadow." << std::endl;
        return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    shaderProgram.Use();
    const GLint modelLoc = glGetUniformLocation(shaderProgram.program, "model");
    const GLint viewLoc = glGetUniformLocation(shaderProgram.program, "view");
    const GLint projectionLoc = glGetUniformLocation(shaderProgram.program, "projection");
    const GLint viewPosLoc = glGetUniformLocation(shaderProgram.program, "viewPos");
    const GLint samplerLoc = glGetUniformLocation(shaderProgram.program, "textureSampler");
    const GLint lightSpaceLoc = glGetUniformLocation(shaderProgram.program, "lightSpaceMatrix");
    const GLint shadowMapLoc = glGetUniformLocation(shaderProgram.program, "shadowMap");
    const GLint pointShadowMapLoc = glGetUniformLocation(shaderProgram.program, "pointShadowMap");
    const GLint pointShadowLightPosLoc = glGetUniformLocation(shaderProgram.program, "pointShadowLightPos");
    const GLint pointShadowFarPlaneLoc = glGetUniformLocation(shaderProgram.program, "shadowFarPlane");
    const GLint shadowPointIndexLoc = glGetUniformLocation(shaderProgram.program, "shadowPointIndex");
    glUniform1i(samplerLoc, 0);
    if (shadowMapLoc >= 0) {
        glUniform1i(shadowMapLoc, 1);
    }
    if (pointShadowMapLoc >= 0) {
        glUniform1i(pointShadowMapLoc, 2);
    }
    if (pointShadowFarPlaneLoc >= 0) {
        glUniform1f(pointShadowFarPlaneLoc, kPointShadowFarPlane);
    }
    if (shadowPointIndexLoc >= 0) {
        glUniform1i(shadowPointIndexLoc, shadowPointIndex);
    }

    directionalDepthShader.Use();
    const GLint dirDepthModelLoc = glGetUniformLocation(directionalDepthShader.program, "model");
    const GLint dirDepthLightSpaceLoc = glGetUniformLocation(directionalDepthShader.program, "lightSpaceMatrix");

    pointDepthShader.Use();
    const GLint pointDepthModelLoc = glGetUniformLocation(pointDepthShader.program, "model");
    const GLint pointDepthLightPosLoc = glGetUniformLocation(pointDepthShader.program, "lightPos");
    const GLint pointDepthFarPlaneLoc = glGetUniformLocation(pointDepthShader.program, "far_plane");
    std::array<GLint, 6> pointDepthShadowMatricesLoc{};
    for (int i = 0; i < 6; ++i) {
        pointDepthShadowMatricesLoc[i] = glGetUniformLocation(pointDepthShader.program, ("shadowMatrices[" + std::to_string(i) + "]").c_str());
    }
    if (pointDepthFarPlaneLoc >= 0) {
        glUniform1f(pointDepthFarPlaneLoc, kPointShadowFarPlane);
    }

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window, deltaTime);
        HandleOverrideShortcuts(window, overrideMode, characterModel, floorModel, checkerTexturePtr, highlightTexturePtr);

        if (PointLight* caster = pointLights.GetLightMutable(shadowPointIndex)) {
            glm::vec3 orbitOffset(
                std::cos(currentFrame) * pointLightOrbitRadius,
                pointLightVerticalAmplitude * std::sin(currentFrame * 0.7f),
                std::sin(currentFrame) * pointLightOrbitRadius);
            caster->position = pointLightOrbitCenter + orbitOffset;
        }
        glm::vec3 shadowLightPos(0.0f);
        if (const PointLight* casterReadOnly = pointLights.GetLight(shadowPointIndex)) {
            shadowLightPos = casterReadOnly->position;
        }

        int viewportWidth = 0;
        int viewportHeight = 0;
        glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
        if (viewportWidth == 0 || viewportHeight == 0) {
            viewportWidth = 1280;
            viewportHeight = 720;
        }

        glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                                static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
                                                0.1f,
                                                100.0f);

        glm::vec3 lightDirection = glm::normalize(mainSun.direction);
        if (glm::length(lightDirection) < 0.0001f) {
            lightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.3f));
        }
        const glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);
        const float lightDistance = 25.0f;
        glm::vec3 lightPos = sceneCenter - lightDirection * lightDistance;
        glm::vec3 upVector = glm::abs(lightDirection.y) > 0.95f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, kShadowNearPlane, kShadowFarPlane);
        glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, upVector);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glm::mat4 floorMatrix = glm::mat4(1.0f);
        floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, -0.15f, 0.0f));
        floorMatrix = glm::scale(floorMatrix, glm::vec3(10.0f, 0.2f, 10.0f));
        glm::mat4 characterMatrix = glm::mat4(1.0f);

        glViewport(0, 0, kShadowMapWidth, kShadowMapHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        directionalDepthShader.Use();
        if (dirDepthLightSpaceLoc >= 0) {
            glUniformMatrix4fv(dirDepthLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        }
        if (dirDepthModelLoc >= 0) {
            glUniformMatrix4fv(dirDepthModelLoc, 1, GL_FALSE, glm::value_ptr(floorMatrix));
        }
        floorModel.Draw(directionalDepthShader.program, 0);
        if (dirDepthModelLoc >= 0) {
            glUniformMatrix4fv(dirDepthModelLoc, 1, GL_FALSE, glm::value_ptr(characterMatrix));
        }
        characterModel.Draw(directionalDepthShader.program, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, kPointShadowNearPlane, kPointShadowFarPlane);
        std::array<glm::mat4, 6> shadowTransforms{
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            shadowProj * glm::lookAt(shadowLightPos, shadowLightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
        };

        glViewport(0, 0, kPointShadowSize, kPointShadowSize);
        glBindFramebuffer(GL_FRAMEBUFFER, pointDepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        pointDepthShader.Use();
        if (pointDepthLightPosLoc >= 0) {
            glUniform3fv(pointDepthLightPosLoc, 1, glm::value_ptr(shadowLightPos));
        }
        if (pointDepthFarPlaneLoc >= 0) {
            glUniform1f(pointDepthFarPlaneLoc, kPointShadowFarPlane);
        }
        for (int i = 0; i < 6; ++i) {
            if (pointDepthShadowMatricesLoc[i] >= 0) {
                glUniformMatrix4fv(pointDepthShadowMatricesLoc[i], 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
            }
        }
        if (pointDepthModelLoc >= 0) {
            glUniformMatrix4fv(pointDepthModelLoc, 1, GL_FALSE, glm::value_ptr(floorMatrix));
        }
        floorModel.Draw(pointDepthShader.program, 0);
        if (pointDepthModelLoc >= 0) {
            glUniformMatrix4fv(pointDepthModelLoc, 1, GL_FALSE, glm::value_ptr(characterMatrix));
        }
        characterModel.Draw(pointDepthShader.program, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, viewportWidth, viewportHeight);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.Use();
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera.GetPosition()));
        if (lightSpaceLoc >= 0) {
            glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        }
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, pointDepthCubemap);
        if (pointShadowLightPosLoc >= 0) {
            glUniform3fv(pointShadowLightPosLoc, 1, glm::value_ptr(shadowLightPos));
        }
        if (pointShadowFarPlaneLoc >= 0) {
            glUniform1f(pointShadowFarPlaneLoc, kPointShadowFarPlane);
        }
        if (shadowPointIndexLoc >= 0) {
            glUniform1i(shadowPointIndexLoc, shadowPointIndex);
        }

        directionalLights.Upload(shaderProgram.program, currentFrame);
        pointLights.Upload(shaderProgram.program);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(floorMatrix));
        floorModel.Draw(shaderProgram.program, defaultWhiteTexture);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(characterMatrix));
        characterModel.Draw(shaderProgram.program, defaultWhiteTexture);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram.Delete();
    directionalDepthShader.Delete();
    pointDepthShader.Delete();
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
    glDeleteFramebuffers(1, &pointDepthMapFBO);
    glDeleteTextures(1, &pointDepthCubemap);
    glDeleteTextures(1, &defaultWhiteTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

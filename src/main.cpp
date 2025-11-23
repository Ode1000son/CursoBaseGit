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
#include <cstddef>
#include <utility>
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
constexpr int kDefaultFramebufferWidth = 1280;
constexpr int kDefaultFramebufferHeight = 720;

struct MultiRenderTargetFramebuffer
{
    GLuint fbo = 0;
    std::array<GLuint, 2> colorAttachments{ 0, 0 };
    GLuint depthBuffer = 0;
    int width = 0;
    int height = 0;
};

constexpr std::array<float, 24> kFullscreenQuadVertices{
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
};

bool EnsureFramebufferSize(MultiRenderTargetFramebuffer& framebuffer, int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);

    if (framebuffer.fbo == 0) {
        glGenFramebuffers(1, &framebuffer.fbo);
    }
    for (GLuint& attachment : framebuffer.colorAttachments) {
        if (attachment == 0) {
            glGenTextures(1, &attachment);
        }
    }
    if (framebuffer.depthBuffer == 0) {
        glGenRenderbuffers(1, &framebuffer.depthBuffer);
    }

    if (width == framebuffer.width && height == framebuffer.height) {
        return true;
    }

    framebuffer.width = width;
    framebuffer.height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
    for (std::size_t i = 0; i < framebuffer.colorAttachments.size(); ++i) {
        glBindTexture(GL_TEXTURE_2D, framebuffer.colorAttachments[i]);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA16F,
                     framebuffer.width,
                     framebuffer.height,
                     0,
                     GL_RGBA,
                     GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 + static_cast<GLint>(i),
                               GL_TEXTURE_2D,
                               framebuffer.colorAttachments[i],
                               0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, framebuffer.width, framebuffer.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    const GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    bool isComplete = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return isComplete;
}

void DestroyFramebuffer(MultiRenderTargetFramebuffer& framebuffer)
{
    if (framebuffer.fbo != 0) {
        glDeleteFramebuffers(1, &framebuffer.fbo);
        framebuffer.fbo = 0;
    }
    for (GLuint& attachment : framebuffer.colorAttachments) {
        if (attachment != 0) {
            glDeleteTextures(1, &attachment);
            attachment = 0;
        }
    }
    if (framebuffer.depthBuffer != 0) {
        glDeleteRenderbuffers(1, &framebuffer.depthBuffer);
        framebuffer.depthBuffer = 0;
    }
    framebuffer.width = 0;
    framebuffer.height = 0;
}

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

struct SceneObjectTransform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct SceneObject
{
    SceneObject() = default;

    SceneObject(std::string objectName,
                Model* objectModel,
                const SceneObjectTransform& initialTransform)
        : name(std::move(objectName))
        , model(objectModel)
        , transform(initialTransform)
        , baseTransform(initialTransform)
    {
    }

    glm::mat4 GetModelMatrix() const
    {
        glm::mat4 modelMatrix(1.0f);
        modelMatrix = glm::translate(modelMatrix, transform.position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::scale(modelMatrix, transform.scale);
        return modelMatrix;
    }

    std::string name;
    Model* model{ nullptr };
    SceneObjectTransform transform{};
    SceneObjectTransform baseTransform{};
};

class Scene
{
public:
    void Reserve(std::size_t count) { m_objects.reserve(count); }

    SceneObject& AddObject(std::string name,
                           Model* model,
                           const SceneObjectTransform& initialTransform)
    {
        m_objects.emplace_back(std::move(name), model, initialTransform);
        return m_objects.back();
    }

    std::vector<SceneObject>& Objects() { return m_objects; }
    const std::vector<SceneObject>& Objects() const { return m_objects; }

private:
    std::vector<SceneObject> m_objects;
};

void DrawSceneObjects(const Scene& scene, GLint modelUniformLocation, GLuint program, GLuint fallbackTexture)
{
    for (const auto& object : scene.Objects()) {
        if (!object.model) {
            continue;
        }
        if (modelUniformLocation >= 0) {
            const glm::mat4 modelMatrix = object.GetModelMatrix();
            glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        }
        object.model->Draw(program, fallbackTexture);
    }
}

void UpdateSceneAnimation(SceneObject& character, SceneObject& car, float currentTime)
{
    character.transform = character.baseTransform;
    character.transform.position.y += 0.05f * std::sin(currentTime * 1.5f);
    character.transform.rotation.y += std::sin(currentTime * 0.3f) * 15.0f;

    car.transform = car.baseTransform;
    car.transform.position.x += std::cos(currentTime * 0.4f) * 0.8f;
    car.transform.position.z += std::sin(currentTime * 0.4f) * 0.6f;
    car.transform.position.y += 0.02f * std::sin(currentTime * 2.2f);
    car.transform.rotation.y += static_cast<float>(std::fmod(currentTime * 45.0f, 360.0f));
}

void ApplyOverrideMode(TextureOverrideMode mode,
                       const std::vector<Model*>& models,
                       Texture* checkerTexture,
                       Texture* highlightTexture)
{
    for (Model* model : models) {
        if (model) {
            model->ClearTextureOverrides();
        }
    }

    auto applyTexture = [&](Texture* texture) {
        if (!texture) {
            return;
        }
        for (Model* model : models) {
            if (model) {
                model->OverrideAllTextures(texture);
            }
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
                             const std::vector<Model*>& models,
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
            ApplyOverrideMode(currentMode, models, checkerTexture, highlightTexture);
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

/// @brief Função principal da Aula 8.1 - Framebuffers e MRT
/// Demonstra renderização off-screen com múltiplos render targets + pipeline de pós-processamento.
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aula 8.1 - Framebuffers e MRT", nullptr, nullptr);
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

    MultiRenderTargetFramebuffer sceneFramebuffer;
    if (!EnsureFramebufferSize(sceneFramebuffer, kDefaultFramebufferWidth, kDefaultFramebufferHeight)) {
        std::cerr << "Falha ao configurar o framebuffer off-screen da Aula 8.1." << std::endl;
        return -1;
    }

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

    ShaderProgram postProcessShader;
    postProcessShader.Create("assets/shaders/postprocess_vertex.glsl",
                             "assets/shaders/postprocess_fragment.glsl");

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kFullscreenQuadVertices.size() * sizeof(float)),
                 kFullscreenQuadVertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);

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

    Model carModel;
    if (!carModel.LoadFromFile("assets/models/car.glb") || !carModel.HasMeshes()) {
        std::cerr << "Falha ao carregar modelo do carro (car.glb)." << std::endl;
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
    std::vector<Model*> sceneModels{ &characterModel, &floorModel, &carModel };
    TextureOverrideMode overrideMode = TextureOverrideMode::Imported;
    ApplyOverrideMode(overrideMode, sceneModels, checkerTexturePtr, highlightTexturePtr);
    carModel.ForEachMaterial([](Material& material) {
        material.SetSpecular(glm::vec3(0.0f));
        material.SetShininess(1.0f);
    });

    Scene scene;
    scene.Reserve(4);
    scene.AddObject("Floor", &floorModel, SceneObjectTransform{
        glm::vec3(0.0f, -0.15f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(10.0f, 0.2f, 10.0f)
    });
    SceneObject& characterObject = scene.AddObject("Character", &characterModel, SceneObjectTransform{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -90.0f, 0.0f),
        glm::vec3(1.0f)
    });
    SceneObject& carObject = scene.AddObject("Car", &carModel, SceneObjectTransform{
        glm::vec3(2.5f, -0.05f, -2.5f),
        glm::vec3(0.0f, 90.0f, 0.0f),
        glm::vec3(0.9f)
    });

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

    postProcessShader.Use();
    const GLint sceneColorLocPP = glGetUniformLocation(postProcessShader.program, "sceneColor");
    const GLint sceneHighlightsLocPP = glGetUniformLocation(postProcessShader.program, "sceneHighlights");
    const GLint exposureLoc = glGetUniformLocation(postProcessShader.program, "exposure");
    const GLint bloomIntensityLoc = glGetUniformLocation(postProcessShader.program, "bloomIntensity");
    if (sceneColorLocPP >= 0) {
        glUniform1i(sceneColorLocPP, 0);
    }
    if (sceneHighlightsLocPP >= 0) {
        glUniform1i(sceneHighlightsLocPP, 1);
    }
    if (exposureLoc >= 0) {
        glUniform1f(exposureLoc, 1.05f);
    }
    if (bloomIntensityLoc >= 0) {
        glUniform1f(bloomIntensityLoc, 0.85f);
    }

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window, deltaTime);
        HandleOverrideShortcuts(window, overrideMode, sceneModels, checkerTexturePtr, highlightTexturePtr);
        UpdateSceneAnimation(characterObject, carObject, currentFrame);

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
            viewportWidth = kDefaultFramebufferWidth;
            viewportHeight = kDefaultFramebufferHeight;
        }
        if (!EnsureFramebufferSize(sceneFramebuffer, viewportWidth, viewportHeight)) {
            std::cerr << "Falha ao redimensionar o framebuffer off-screen para "
                      << viewportWidth << "x" << viewportHeight << std::endl;
            break;
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

        glViewport(0, 0, kShadowMapWidth, kShadowMapHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        directionalDepthShader.Use();
        if (dirDepthLightSpaceLoc >= 0) {
            glUniformMatrix4fv(dirDepthLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        }
        DrawSceneObjects(scene, dirDepthModelLoc, directionalDepthShader.program, 0);
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
        DrawSceneObjects(scene, pointDepthModelLoc, pointDepthShader.program, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, sceneFramebuffer.width, sceneFramebuffer.height);
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer.fbo);
        glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
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

        DrawSceneObjects(scene, modelLoc, shaderProgram.program, defaultWhiteTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, viewportWidth, viewportHeight);
        glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        postProcessShader.Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneFramebuffer.colorAttachments[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneFramebuffer.colorAttachments[1]);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    shaderProgram.Delete();
    directionalDepthShader.Delete();
    pointDepthShader.Delete();
    postProcessShader.Delete();
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteFramebuffers(1, &depthMapFBO);
    glDeleteTextures(1, &depthMap);
    glDeleteFramebuffers(1, &pointDepthMapFBO);
    glDeleteTextures(1, &pointDepthCubemap);
    DestroyFramebuffer(sceneFramebuffer);
    glDeleteTextures(1, &defaultWhiteTexture);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

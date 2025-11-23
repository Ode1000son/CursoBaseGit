#include "renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace
{
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

constexpr std::array<float, 24> kFullscreenQuadVertices{
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,

    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f
};
}

struct Plane
{
    glm::vec3 normal{ 0.0f };
    float distance = 0.0f;
};

struct Frustum
{
    std::array<Plane, 6> planes{};

    bool IsSphereVisible(const glm::vec3& center, float radius) const
    {
        for (const auto& plane : planes)
        {
            if (glm::dot(plane.normal, center) + plane.distance < -radius)
            {
                return false;
            }
        }
        return true;
    }
};

namespace
{
Plane NormalizePlane(const glm::vec4& plane)
{
    const glm::vec3 normal = glm::vec3(plane);
    const float length = glm::length(normal);
    if (length == 0.0f)
    {
        return Plane{};
    }
    return Plane{ normal / length, plane.w / length };
}

Frustum ExtractFrustum(const glm::mat4& matrix)
{
    Frustum frustum;
    const glm::vec4 rowX = glm::row(matrix, 0);
    const glm::vec4 rowY = glm::row(matrix, 1);
    const glm::vec4 rowZ = glm::row(matrix, 2);
    const glm::vec4 rowW = glm::row(matrix, 3);

    frustum.planes[0] = NormalizePlane(rowW + rowX);
    frustum.planes[1] = NormalizePlane(rowW - rowX);
    frustum.planes[2] = NormalizePlane(rowW + rowY);
    frustum.planes[3] = NormalizePlane(rowW - rowY);
    frustum.planes[4] = NormalizePlane(rowW + rowZ);
    frustum.planes[5] = NormalizePlane(rowW - rowZ);

    return frustum;
}

GLuint LoadAndCompileShader(const std::string& path, GLenum type)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Erro ao abrir arquivo shader: " << path << std::endl;
        return 0;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string source = buffer.str();

    GLuint shader = glCreateShader(type);
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Erro ao compilar shader " << path << ": " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

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
} // namespace

// === ShaderProgram ===
bool ShaderProgram::Create(const std::string& vertexPath,
                           const std::string& fragmentPath,
                           const std::string& geometryPath)
{
    GLuint vertexShader = LoadAndCompileShader(vertexPath, GL_VERTEX_SHADER);
    if (vertexShader == 0)
    {
        return false;
    }

    GLuint fragmentShader = LoadAndCompileShader(fragmentPath, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return false;
    }

    GLuint geometryShader = 0;
    if (!geometryPath.empty())
    {
        geometryShader = LoadAndCompileShader(geometryPath, GL_GEOMETRY_SHADER);
        if (geometryShader == 0)
        {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            return false;
        }
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    if (geometryShader != 0)
    {
        glAttachShader(program, geometryShader);
    }
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Erro ao linkar programa shader: " << infoLog << std::endl;
        Destroy();
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader != 0)
        {
            glDeleteShader(geometryShader);
        }
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    if (geometryShader != 0)
    {
        glDeleteShader(geometryShader);
    }

    return true;
}

void ShaderProgram::Use() const
{
    glUseProgram(program);
}

void ShaderProgram::Destroy()
{
    if (program != 0)
    {
        glDeleteProgram(program);
        program = 0;
    }
}

// === Scene ===
// === Renderer ===
Renderer::Renderer()
    : m_directionalLights(kMaxDirectionalLights)
    , m_pointLights(kMaxPointLights)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(Scene* scene)
{
    if (m_initialized)
    {
        return true;
    }

    if (scene == nullptr)
    {
        return true;
    }

    m_scene = scene;
    m_sceneModels = m_scene->GetModelPointers();
    m_characterObject = m_scene->GetCharacterObject();
    m_carObject = m_scene->GetCarObject();

    glEnable(GL_DEPTH_TEST);

    m_defaultWhiteTexture = CreateSolidColorTexture(glm::vec4(1.0f));

    if (!CreateShaders())
    {
        Shutdown();
        return false;
    }

    if (!CreateFullscreenQuad())
    {
        Shutdown();
        return false;
    }

    m_checkerTexture.LoadFromFile("assets/texture.png");
    m_highlightTexture.LoadFromFile("assets/models/CubeTexture.jpg");

    SetupLights();

    if (!SetupShadowResources())
    {
        Shutdown();
        return false;
    }

    if (!EnsureOffscreenSize(kDefaultFramebufferWidth, kDefaultFramebufferHeight))
    {
        Shutdown();
        return false;
    }

    glGenBuffers(1, &m_instanceVBO);
    m_instanceBufferCapacity = 0;

    ApplyOverrideMode(m_overrideMode);
    m_initialized = true;
    return true;
}

void Renderer::Shutdown()
{
    if (!m_initialized)
    {
        DestroyShaders();
        DestroyFullscreenQuad();
        DestroyFramebuffer(m_sceneFramebuffer);
        if (m_depthMapFBO != 0)
        {
            glDeleteFramebuffers(1, &m_depthMapFBO);
            m_depthMapFBO = 0;
        }
        if (m_depthMap != 0)
        {
            glDeleteTextures(1, &m_depthMap);
            m_depthMap = 0;
        }
        if (m_pointDepthMapFBO != 0)
        {
            glDeleteFramebuffers(1, &m_pointDepthMapFBO);
            m_pointDepthMapFBO = 0;
        }
        if (m_pointDepthCubemap != 0)
        {
            glDeleteTextures(1, &m_pointDepthCubemap);
            m_pointDepthCubemap = 0;
        }
        if (m_defaultWhiteTexture != 0)
        {
            glDeleteTextures(1, &m_defaultWhiteTexture);
            m_defaultWhiteTexture = 0;
        }
        if (m_instanceVBO != 0)
        {
            glDeleteBuffers(1, &m_instanceVBO);
            m_instanceVBO = 0;
            m_instanceBufferCapacity = 0;
        }
        return;
    }

    DestroyShaders();
    DestroyFullscreenQuad();
    DestroyFramebuffer(m_sceneFramebuffer);

    if (m_depthMapFBO != 0)
    {
        glDeleteFramebuffers(1, &m_depthMapFBO);
        m_depthMapFBO = 0;
    }
    if (m_depthMap != 0)
    {
        glDeleteTextures(1, &m_depthMap);
        m_depthMap = 0;
    }
    if (m_pointDepthMapFBO != 0)
    {
        glDeleteFramebuffers(1, &m_pointDepthMapFBO);
        m_pointDepthMapFBO = 0;
    }
    if (m_pointDepthCubemap != 0)
    {
        glDeleteTextures(1, &m_pointDepthCubemap);
        m_pointDepthCubemap = 0;
    }
    if (m_defaultWhiteTexture != 0)
    {
        glDeleteTextures(1, &m_defaultWhiteTexture);
        m_defaultWhiteTexture = 0;
    }
    if (m_instanceVBO != 0)
    {
        glDeleteBuffers(1, &m_instanceVBO);
        m_instanceVBO = 0;
        m_instanceBufferCapacity = 0;
    }

    m_sceneModels.clear();
    m_characterObject = nullptr;
    m_carObject = nullptr;
    m_scene = nullptr;
    m_initialized = false;
}

void Renderer::RenderFrame(GLFWwindow* window, const Camera& camera, float currentTime)
{
    if (!m_initialized || m_scene == nullptr)
    {
        return;
    }

    int viewportWidth = 0;
    int viewportHeight = 0;
    if (window != nullptr)
    {
        glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
    }
    if (viewportWidth <= 0 || viewportHeight <= 0)
    {
        viewportWidth = kDefaultFramebufferWidth;
        viewportHeight = kDefaultFramebufferHeight;
    }

    if (!EnsureOffscreenSize(viewportWidth, viewportHeight))
    {
        std::cerr << "Falha ao redimensionar framebuffer off-screen (" << viewportWidth << "x" << viewportHeight << ")." << std::endl;
        return;
    }

    if (m_scene != nullptr)
    {
        m_scene->Update(currentTime);
        m_characterObject = m_scene->GetCharacterObject();
        m_carObject = m_scene->GetCarObject();
    }
    UpdateOrbitingPointLight(currentTime);
    m_lastCameraPos = camera.GetPosition();

    glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()),
                                            static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
                                            0.1f,
                                            100.0f);

    glm::mat4 lightSpaceMatrix = ComputeDirectionalLightMatrix();

    RenderDirectionalShadowPass(lightSpaceMatrix);
    RenderPointShadowPass(m_shadowLightPos);
    RenderScenePass(camera, projection, lightSpaceMatrix, viewportWidth, viewportHeight, currentTime);
    RenderPostProcessPass(viewportWidth, viewportHeight);
}

void Renderer::SetOverrideMode(TextureOverrideMode mode)
{
    if (m_overrideMode == mode)
    {
        return;
    }
    m_overrideMode = mode;
    ApplyOverrideMode(m_overrideMode);
}

bool Renderer::EnsureOffscreenSize(int width, int height)
{
    return EnsureFramebufferSize(m_sceneFramebuffer, width, height);
}

bool Renderer::CreateShaders()
{
    if (!m_sceneShader.Create("assets/shaders/vertex.glsl", "assets/shaders/fragment.glsl"))
    {
        return false;
    }
    if (!m_directionalDepthShader.Create("assets/shaders/directional_depth_vertex.glsl",
                                         "assets/shaders/directional_depth_fragment.glsl"))
    {
        return false;
    }
    if (!m_pointDepthShader.Create("assets/shaders/depth_vertex.glsl",
                                   "assets/shaders/depth_fragment.glsl",
                                   "assets/shaders/depth_geometry.glsl"))
    {
        return false;
    }
    if (!m_postProcessShader.Create("assets/shaders/postprocess_vertex.glsl",
                                    "assets/shaders/postprocess_fragment.glsl"))
    {
        return false;
    }

    m_sceneShader.Use();
    m_modelLoc = glGetUniformLocation(m_sceneShader.program, "model");
    m_viewLoc = glGetUniformLocation(m_sceneShader.program, "view");
    m_projectionLoc = glGetUniformLocation(m_sceneShader.program, "projection");
    m_viewPosLoc = glGetUniformLocation(m_sceneShader.program, "viewPos");
    m_lightSpaceLoc = glGetUniformLocation(m_sceneShader.program, "lightSpaceMatrix");
    m_shadowMapLoc = glGetUniformLocation(m_sceneShader.program, "shadowMap");
    m_pointShadowMapLoc = glGetUniformLocation(m_sceneShader.program, "pointShadowMap");
    m_pointShadowLightPosLoc = glGetUniformLocation(m_sceneShader.program, "pointShadowLightPos");
    m_pointShadowFarPlaneLoc = glGetUniformLocation(m_sceneShader.program, "shadowFarPlane");
    m_shadowPointIndexLoc = glGetUniformLocation(m_sceneShader.program, "shadowPointIndex");
    m_sceneInstanceFlagLoc = glGetUniformLocation(m_sceneShader.program, "uUseInstanceTransform");
    const GLint samplerLoc = glGetUniformLocation(m_sceneShader.program, "textureSampler");
    if (samplerLoc >= 0)
    {
        glUniform1i(samplerLoc, 0);
    }
    if (m_shadowMapLoc >= 0)
    {
        glUniform1i(m_shadowMapLoc, 1);
    }
    if (m_pointShadowMapLoc >= 0)
    {
        glUniform1i(m_pointShadowMapLoc, 2);
    }
    if (m_sceneInstanceFlagLoc >= 0)
    {
        glUniform1i(m_sceneInstanceFlagLoc, 0);
    }

    m_directionalDepthShader.Use();
    m_dirDepthModelLoc = glGetUniformLocation(m_directionalDepthShader.program, "model");
    m_dirDepthLightSpaceLoc = glGetUniformLocation(m_directionalDepthShader.program, "lightSpaceMatrix");
    m_dirDepthInstanceFlagLoc = glGetUniformLocation(m_directionalDepthShader.program, "uUseInstanceTransform");
    if (m_dirDepthInstanceFlagLoc >= 0)
    {
        glUniform1i(m_dirDepthInstanceFlagLoc, 0);
    }

    m_pointDepthShader.Use();
    m_pointDepthModelLoc = glGetUniformLocation(m_pointDepthShader.program, "model");
    m_pointDepthLightPosLoc = glGetUniformLocation(m_pointDepthShader.program, "lightPos");
    m_pointDepthFarPlaneLoc = glGetUniformLocation(m_pointDepthShader.program, "far_plane");
    m_pointDepthInstanceFlagLoc = glGetUniformLocation(m_pointDepthShader.program, "uUseInstanceTransform");
    for (int i = 0; i < 6; ++i)
    {
        m_pointDepthShadowMatricesLoc[i] =
            glGetUniformLocation(m_pointDepthShader.program, ("shadowMatrices[" + std::to_string(i) + "]").c_str());
    }
    if (m_pointDepthFarPlaneLoc >= 0)
    {
        glUniform1f(m_pointDepthFarPlaneLoc, kPointShadowFarPlane);
    }
    if (m_pointDepthInstanceFlagLoc >= 0)
    {
        glUniform1i(m_pointDepthInstanceFlagLoc, 0);
    }

    m_postProcessShader.Use();
    m_postSceneColorLoc = glGetUniformLocation(m_postProcessShader.program, "sceneColor");
    m_postHighlightsLoc = glGetUniformLocation(m_postProcessShader.program, "sceneHighlights");
    m_postExposureLoc = glGetUniformLocation(m_postProcessShader.program, "exposure");
    m_postBloomLoc = glGetUniformLocation(m_postProcessShader.program, "bloomIntensity");
    if (m_postSceneColorLoc >= 0)
    {
        glUniform1i(m_postSceneColorLoc, 0);
    }
    if (m_postHighlightsLoc >= 0)
    {
        glUniform1i(m_postHighlightsLoc, 1);
    }
    if (m_postExposureLoc >= 0)
    {
        glUniform1f(m_postExposureLoc, 1.05f);
    }
    if (m_postBloomLoc >= 0)
    {
        glUniform1f(m_postBloomLoc, 0.85f);
    }

    return true;
}

void Renderer::DestroyShaders()
{
    m_sceneShader.Destroy();
    m_directionalDepthShader.Destroy();
    m_pointDepthShader.Destroy();
    m_postProcessShader.Destroy();
}

bool Renderer::CreateFullscreenQuad()
{
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kFullscreenQuadVertices.size() * sizeof(float)),
                 kFullscreenQuadVertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

void Renderer::DestroyFullscreenQuad()
{
    if (m_quadVBO != 0)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_quadVAO != 0)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
}


void Renderer::SetupLights()
{
    m_primarySun = {
        glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f)),
        glm::vec3(0.25f, 0.22f, 0.20f),
        glm::vec3(0.9f, 0.85f, 0.8f),
        glm::vec3(1.0f),
        false,
        glm::vec3(0.0f),
        0.0f
    };

    m_directionalLights.AddLight(m_primarySun);
    m_directionalLights.AddLight({ glm::normalize(glm::vec3(0.3f, -1.0f, 0.15f)),
                                   glm::vec3(0.02f, 0.02f, 0.03f),
                                   glm::vec3(0.35f, 0.4f, 0.55f),
                                   glm::vec3(0.25f, 0.3f, 0.45f),
                                   false });

    m_shadowPointIndex = m_pointLights.AddLight({
        glm::vec3(0.0f, 2.8f, 0.0f),
        glm::vec3(0.03f, 0.03f, 0.03f),
        glm::vec3(1.0f, 0.85f, 0.6f),
        glm::vec3(1.0f, 0.95f, 0.9f),
        1.0f,
        0.09f,
        0.032f,
        18.0f
    });
    m_pointLights.AddLight({
        glm::vec3(-3.0f, 3.5f, -2.0f),
        glm::vec3(0.04f, 0.05f, 0.06f),
        glm::vec3(0.55f, 0.65f, 1.0f),
        glm::vec3(0.35f, 0.40f, 0.55f),
        1.0f,
        0.14f,
        0.07f,
        12.0f
    });
}

bool Renderer::SetupShadowResources()
{
    glGenFramebuffers(1, &m_depthMapFBO);
    glGenTextures(1, &m_depthMap);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapWidth, kShadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Erro ao configurar framebuffer de depth para shadow mapping." << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &m_pointDepthMapFBO);
    glGenTextures(1, &m_pointDepthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointDepthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
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

    glBindFramebuffer(GL_FRAMEBUFFER, m_pointDepthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_pointDepthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Erro ao configurar framebuffer de depth para point light shadow." << std::endl;
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}


void Renderer::UpdateOrbitingPointLight(float currentTime)
{
    if (PointLight* caster = m_pointLights.GetLightMutable(m_shadowPointIndex))
    {
        glm::vec3 orbitOffset(
            std::cos(currentTime) * m_pointLightOrbitRadius,
            m_pointLightVerticalAmplitude * std::sin(currentTime * 0.7f),
            std::sin(currentTime) * m_pointLightOrbitRadius);
        caster->position = m_pointLightOrbitCenter + orbitOffset;
        m_shadowLightPos = caster->position;
    }
    else
    {
        m_shadowLightPos = glm::vec3(0.0f);
    }
}

glm::mat4 Renderer::ComputeDirectionalLightMatrix() const
{
    glm::vec3 lightDirection = glm::normalize(m_primarySun.direction);
    if (glm::length(lightDirection) < 0.0001f)
    {
        lightDirection = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.3f));
    }
    const glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);
    const float lightDistance = 25.0f;
    glm::vec3 lightPos = sceneCenter - lightDirection * lightDistance;
    glm::vec3 upVector = glm::abs(lightDirection.y) > 0.95f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, kShadowNearPlane, kShadowFarPlane);
    glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, upVector);
    return lightProjection * lightView;
}

void Renderer::RenderDirectionalShadowPass(const glm::mat4& lightSpaceMatrix)
{
    glViewport(0, 0, kShadowMapWidth, kShadowMapHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_directionalDepthShader.Use();
    if (m_dirDepthLightSpaceLoc >= 0)
    {
        glUniformMatrix4fv(m_dirDepthLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    }
    if (m_dirDepthInstanceFlagLoc >= 0)
    {
        glUniform1i(m_dirDepthInstanceFlagLoc, 0);
    }
    DrawSceneObjects(m_dirDepthModelLoc, m_directionalDepthShader.program, 0, nullptr, &m_lastCameraPos);
    DrawInstancedBatches(m_dirDepthModelLoc, m_directionalDepthShader.program, 0, m_dirDepthInstanceFlagLoc, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPointShadowPass(const glm::vec3& lightPos)
{
    const glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, kPointShadowNearPlane, kPointShadowFarPlane);
    std::array<glm::mat4, 6> shadowTransforms{
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    glViewport(0, 0, kPointShadowSize, kPointShadowSize);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pointDepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_pointDepthShader.Use();
    if (m_pointDepthLightPosLoc >= 0)
    {
        glUniform3fv(m_pointDepthLightPosLoc, 1, glm::value_ptr(lightPos));
    }
    if (m_pointDepthFarPlaneLoc >= 0)
    {
        glUniform1f(m_pointDepthFarPlaneLoc, kPointShadowFarPlane);
    }
    for (int i = 0; i < 6; ++i)
    {
        if (m_pointDepthShadowMatricesLoc[i] >= 0)
        {
            glUniformMatrix4fv(m_pointDepthShadowMatricesLoc[i], 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
        }
    }
    if (m_pointDepthInstanceFlagLoc >= 0)
    {
        glUniform1i(m_pointDepthInstanceFlagLoc, 0);
    }
    DrawSceneObjects(m_pointDepthModelLoc, m_pointDepthShader.program, 0, nullptr, &m_lastCameraPos);
    DrawInstancedBatches(m_pointDepthModelLoc, m_pointDepthShader.program, 0, m_pointDepthInstanceFlagLoc, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderScenePass(const Camera& camera,
                               const glm::mat4& projection,
                               const glm::mat4& lightSpaceMatrix,
                               int viewportWidth,
                               int viewportHeight,
                               float currentTime)
{
    glViewport(0, 0, m_sceneFramebuffer.width, m_sceneFramebuffer.height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_sceneFramebuffer.fbo);
    glClearColor(0.02f, 0.02f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_sceneShader.Use();
    glm::mat4 view = camera.GetViewMatrix();
    if (m_viewLoc >= 0)
    {
        glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (m_projectionLoc >= 0)
    {
        glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    if (m_viewPosLoc >= 0)
    {
        glUniform3fv(m_viewPosLoc, 1, glm::value_ptr(camera.GetPosition()));
    }
    if (m_lightSpaceLoc >= 0)
    {
        glUniformMatrix4fv(m_lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    }

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_pointDepthCubemap);
    if (m_pointShadowLightPosLoc >= 0)
    {
        glUniform3fv(m_pointShadowLightPosLoc, 1, glm::value_ptr(m_shadowLightPos));
    }
    if (m_pointShadowFarPlaneLoc >= 0)
    {
        glUniform1f(m_pointShadowFarPlaneLoc, kPointShadowFarPlane);
    }
    if (m_shadowPointIndexLoc >= 0)
    {
        glUniform1i(m_shadowPointIndexLoc, m_shadowPointIndex);
    }

    m_directionalLights.Upload(m_sceneShader.program, currentTime);
    m_pointLights.Upload(m_sceneShader.program);

    glm::vec3 cameraPos = camera.GetPosition();
    const Frustum frustum = ExtractFrustum(projection * view);
    if (m_sceneInstanceFlagLoc >= 0)
    {
        glUniform1i(m_sceneInstanceFlagLoc, 0);
    }
    DrawSceneObjects(m_modelLoc, m_sceneShader.program, m_defaultWhiteTexture, &frustum, &cameraPos);
    DrawInstancedBatches(m_modelLoc, m_sceneShader.program, m_defaultWhiteTexture, m_sceneInstanceFlagLoc, &frustum);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPostProcessPass(int viewportWidth, int viewportHeight)
{
    glViewport(0, 0, viewportWidth, viewportHeight);
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_postProcessShader.Use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_sceneFramebuffer.colorAttachments[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_sceneFramebuffer.colorAttachments[1]);
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
}

void Renderer::DrawSceneObjects(GLint modelLocation,
                                GLuint program,
                                GLuint fallbackTexture,
                                const Frustum* frustum,
                                const glm::vec3* cameraPos)
{
    if (m_scene == nullptr)
    {
        return;
    }

    const auto& objects = m_scene->GetObjects();
    for (const auto& object : objects)
    {
        Model* resolvedModel = object.GetModel();
        if (resolvedModel == nullptr)
        {
            continue;
        }

        const glm::mat4 modelMatrix = object.GetModelMatrix();
        const glm::vec3 worldCenter = object.GetWorldCenter(modelMatrix);
        const float worldRadius = object.GetWorldRadius();

        if (frustum != nullptr && !frustum->IsSphereVisible(worldCenter, worldRadius))
        {
            continue;
        }

        if (cameraPos != nullptr)
        {
            const float distance = glm::length(worldCenter - *cameraPos);
            resolvedModel = object.ResolveModelForDistance(distance);
            if (resolvedModel == nullptr)
            {
                continue;
            }
        }

        if (modelLocation >= 0)
        {
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        }
        resolvedModel->Draw(program, fallbackTexture);
    }
}

void Renderer::DrawInstancedBatches(GLint modelLocation,
                                    GLuint program,
                                    GLuint fallbackTexture,
                                    GLint instancingUniformLoc,
                                    const Frustum* frustum)
{
    if (m_scene == nullptr || m_instanceVBO == 0)
    {
        return;
    }

    const auto& batches = m_scene->GetInstancedBatches();
    if (batches.empty())
    {
        return;
    }

    glm::mat4 identity(1.0f);
    if (modelLocation >= 0)
    {
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(identity));
    }
    if (instancingUniformLoc >= 0)
    {
        glUniform1i(instancingUniformLoc, 1);
    }

    std::vector<glm::mat4> culledTransforms;
    for (const auto& batch : batches)
    {
        if (batch.model == nullptr || batch.transforms.empty())
        {
            continue;
        }

        const std::vector<glm::mat4>* transformsToDraw = &batch.transforms;
        if (frustum != nullptr)
        {
            culledTransforms.clear();
            culledTransforms.reserve(batch.transforms.size());
            for (const glm::mat4& transform : batch.transforms)
            {
                const glm::vec3 center = glm::vec3(transform[3]);
                const float scaleX = glm::length(glm::vec3(transform[0]));
                const float scaleY = glm::length(glm::vec3(transform[1]));
                const float scaleZ = glm::length(glm::vec3(transform[2]));
                const float maxScale = std::max({ scaleX, scaleY, scaleZ });
                const float radius = batch.baseRadius * maxScale;
                if (frustum->IsSphereVisible(center, radius))
                {
                    culledTransforms.push_back(transform);
                }
            }
            transformsToDraw = &culledTransforms;
        }

        if (transformsToDraw->empty())
        {
            continue;
        }

        UpdateInstanceBuffer(*transformsToDraw);
        batch.model->DrawInstanced(program,
                                   fallbackTexture,
                                   m_instanceVBO,
                                   static_cast<GLsizei>(transformsToDraw->size()));
    }

    if (instancingUniformLoc >= 0)
    {
        glUniform1i(instancingUniformLoc, 0);
    }
}

bool Renderer::EnsureInstanceBufferSize(std::size_t instanceCount)
{
    if (instanceCount == 0)
    {
        return true;
    }

    const GLsizeiptr requiredSize = static_cast<GLsizeiptr>(instanceCount * sizeof(glm::mat4));
    if (requiredSize <= m_instanceBufferCapacity)
    {
        return true;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_instanceBufferCapacity = requiredSize;
    return true;
}

void Renderer::UpdateInstanceBuffer(const std::vector<glm::mat4>& matrices)
{
    if (matrices.empty())
    {
        return;
    }

    if (!EnsureInstanceBufferSize(matrices.size()))
    {
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(matrices.size() * sizeof(glm::mat4)),
                    matrices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Renderer::ApplyOverrideMode(TextureOverrideMode mode)
{
    if (m_scene != nullptr)
    {
        m_sceneModels = m_scene->GetModelPointers();
    }

    if (m_sceneModels.empty())
    {
        return;
    }

    for (Model* model : m_sceneModels)
    {
        if (model)
        {
            model->ClearTextureOverrides();
        }
    }

    Texture* checkerPtr = m_checkerTexture.GetID() != 0 ? &m_checkerTexture : nullptr;
    Texture* highlightPtr = m_highlightTexture.GetID() != 0 ? &m_highlightTexture : nullptr;

    auto apply = [&](Texture* texture) {
        if (!texture)
        {
            return;
        }
        for (Model* model : m_sceneModels)
        {
            if (model)
            {
                model->OverrideAllTextures(texture);
            }
        }
    };

    switch (mode)
    {
    case TextureOverrideMode::Checker:
        apply(checkerPtr);
        break;
    case TextureOverrideMode::Highlight:
        apply(highlightPtr);
        break;
    default:
        break;
    }
}

bool Renderer::EnsureFramebufferSize(MultiRenderTargetFramebuffer& framebuffer, int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);

    if (framebuffer.fbo == 0)
    {
        glGenFramebuffers(1, &framebuffer.fbo);
    }
    for (GLuint& attachment : framebuffer.colorAttachments)
    {
        if (attachment == 0)
        {
            glGenTextures(1, &attachment);
        }
    }
    if (framebuffer.depthBuffer == 0)
    {
        glGenRenderbuffers(1, &framebuffer.depthBuffer);
    }

    if (width == framebuffer.width && height == framebuffer.height)
    {
        return true;
    }

    framebuffer.width = width;
    framebuffer.height = height;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
    for (std::size_t i = 0; i < framebuffer.colorAttachments.size(); ++i)
    {
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

void Renderer::DestroyFramebuffer(MultiRenderTargetFramebuffer& framebuffer)
{
    if (framebuffer.fbo != 0)
    {
        glDeleteFramebuffers(1, &framebuffer.fbo);
        framebuffer.fbo = 0;
    }
    for (GLuint& attachment : framebuffer.colorAttachments)
    {
        if (attachment != 0)
        {
            glDeleteTextures(1, &attachment);
            attachment = 0;
        }
    }
    if (framebuffer.depthBuffer != 0)
    {
        glDeleteRenderbuffers(1, &framebuffer.depthBuffer);
        framebuffer.depthBuffer = 0;
    }
    framebuffer.width = 0;
    framebuffer.height = 0;
}


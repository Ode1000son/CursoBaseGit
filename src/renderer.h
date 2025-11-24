#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <string>
#include <vector>
#include <limits>

#include <glm/glm.hpp>

#include "camera.h"
#include "light_manager.h"
#include "material.h"
#include "model.h"
#include "texture.h"
#include "scene.h"

class PhysicsSystem;

struct Frustum;

enum class TextureOverrideMode
{
    Imported = 0,
    Checker,
    Highlight
};

struct ShaderProgram
{
    GLuint program = 0;

    bool Create(const std::string& vertexPath,
                const std::string& fragmentPath,
                const std::string& geometryPath = "");
    void Use() const;
    void Destroy();
};

struct MultiRenderTargetFramebuffer
{
    GLuint fbo = 0;
    std::array<GLuint, 2> colorAttachments{ 0, 0 };
    GLuint depthBuffer = 0;
    int width = 0;
    int height = 0;
};

struct RendererDebugMessage
{
    GLenum source = 0;
    GLenum type = 0;
    GLenum severity = 0;
    GLuint id = 0;
    std::string text;
    double timestamp = 0.0;
};

struct CpuFrameStats
{
    float lastMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = 0.0f;
    float maxMs = 0.0f;
};

struct GpuTimingSummary
{
    double directionalShadowMs = 0.0;
    double pointShadowMs = 0.0;
    double sceneMs = 0.0;
    double postProcessMs = 0.0;

    double Total() const
    {
        return directionalShadowMs + pointShadowMs + sceneMs + postProcessMs;
    }
};

struct GpuTimer
{
    std::array<GLuint, 2> startQueries{ 0, 0 };
    std::array<GLuint, 2> endQueries{ 0, 0 };
    int writeIndex = 0;
    bool primed = false;
    double lastResultMs = 0.0;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(Scene* scene, PhysicsSystem* physicsSystem);
    void Shutdown();

    void RenderFrame(GLFWwindow* window, const Camera& camera, float currentTime, float deltaTime);

    void SetOverrideMode(TextureOverrideMode mode);
    TextureOverrideMode GetOverrideMode() const { return m_overrideMode; }
    void SetWindowTitleBase(const std::string& title);
    void ToggleMetricsOverlay();
    bool IsMetricsOverlayEnabled() const { return m_metricsOverlayEnabled; }
    void ClearDebugMessages();
    void PushDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, const std::string& message);
    void PushOverlayStatus(const std::string& message);

private:
    bool EnsureOffscreenSize(int width, int height);
    bool CreateShaders();
    void DestroyShaders();
    bool CreateFullscreenQuad();
    void DestroyFullscreenQuad();
    void SetupLights();
    bool SetupShadowResources();
    void UpdateOrbitingPointLight(float currentTime);
    glm::mat4 ComputeDirectionalLightMatrix() const;
    void RenderDirectionalShadowPass(const glm::mat4& lightSpaceMatrix);
    void RenderPointShadowPass(const glm::vec3& lightPos);
    void RenderScenePass(const Camera& camera,
                         const glm::mat4& projection,
                         const glm::mat4& lightSpaceMatrix,
                         int viewportWidth,
                         int viewportHeight,
                         float currentTime);
    void RenderPostProcessPass(int viewportWidth, int viewportHeight);
    void RenderPhysicsDebugOverlay(const glm::mat4& viewProjection);
    void DrawSceneObjects(GLint modelLocation,
                          GLuint program,
                          GLuint fallbackTexture,
                          const Frustum* frustum,
                          const glm::vec3* cameraPos);
    void DrawInstancedBatches(GLint modelLocation,
                              GLuint program,
                              GLuint fallbackTexture,
                              GLint instancingUniformLoc,
                              const Frustum* frustum);
    void ApplyOverrideMode(TextureOverrideMode mode);
    bool EnsureFramebufferSize(MultiRenderTargetFramebuffer& framebuffer, int width, int height);
    void DestroyFramebuffer(MultiRenderTargetFramebuffer& framebuffer);
    bool EnsureInstanceBufferSize(std::size_t instanceCount);
    void UpdateInstanceBuffer(const std::vector<glm::mat4>& matrices);
    bool EnsurePhysicsDebugResources();
    void DestroyPhysicsDebugResources();
    void RecordCpuFrameTime(float deltaTimeSeconds);
    void UpdateOverlayTitle(GLFWwindow* window, float currentTime);
    bool SetupGpuTimers();
    void DestroyGpuTimers();
    void BeginGpuTimer(GpuTimer& timer);
    void EndGpuTimer(GpuTimer& timer);
    void AdvanceGpuTimer(GpuTimer& timer);
    void RefreshGpuTimingSummary();
    static const char* DebugSourceToString(GLenum source);
    static const char* DebugTypeToString(GLenum type);
    static const char* DebugSeverityToString(GLenum severity);
    void WriteDebugMessageToConsole(const RendererDebugMessage& message) const;

    bool m_initialized = false;
    Scene* m_scene = nullptr;
    PhysicsSystem* m_physicsSystem = nullptr;

    ShaderProgram m_sceneShader;
    ShaderProgram m_directionalDepthShader;
    ShaderProgram m_pointDepthShader;
    ShaderProgram m_postProcessShader;
    ShaderProgram m_physicsDebugShader;

    MultiRenderTargetFramebuffer m_sceneFramebuffer;

    GLuint m_depthMapFBO = 0;
    GLuint m_depthMap = 0;
    GLuint m_pointDepthMapFBO = 0;
    GLuint m_pointDepthCubemap = 0;

    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    GLuint m_defaultWhiteTexture = 0;

    DirectionalLightManager m_directionalLights;
    PointLightManager m_pointLights;
    int m_shadowPointIndex = -1;

    SceneObject* m_characterObject = nullptr;
    SceneObject* m_carObject = nullptr;

    Texture m_checkerTexture;
    Texture m_highlightTexture;

    TextureOverrideMode m_overrideMode = TextureOverrideMode::Imported;
    std::vector<Model*> m_sceneModels;

    glm::vec3 m_pointLightOrbitCenter{ 0.0f, 1.8f, 0.0f };
    float m_pointLightOrbitRadius = 3.8f;
    float m_pointLightVerticalAmplitude = 0.7f;
    glm::vec3 m_shadowLightPos{ 0.0f };
    float m_pointLightOrbitSpeed = 1.0f;
    float m_pointLightVerticalFrequency = 0.7f;
    bool m_shadowLightOrbitEnabled = true;

    DirectionalLight m_primarySun{};

    GLint m_modelLoc = -1;
    GLint m_viewLoc = -1;
    GLint m_projectionLoc = -1;
    GLint m_viewPosLoc = -1;
    GLint m_lightSpaceLoc = -1;
    GLint m_shadowMapLoc = -1;
    GLint m_pointShadowMapLoc = -1;
    GLint m_pointShadowLightPosLoc = -1;
    GLint m_pointShadowFarPlaneLoc = -1;
    GLint m_shadowPointIndexLoc = -1;

    GLint m_dirDepthModelLoc = -1;
    GLint m_dirDepthLightSpaceLoc = -1;

    GLint m_pointDepthModelLoc = -1;
    GLint m_pointDepthLightPosLoc = -1;
    GLint m_pointDepthFarPlaneLoc = -1;
    std::array<GLint, 6> m_pointDepthShadowMatricesLoc{};

    GLint m_postSceneColorLoc = -1;
    GLint m_postHighlightsLoc = -1;
    GLint m_postExposureLoc = -1;
    GLint m_postBloomLoc = -1;
    GLint m_sceneInstanceFlagLoc = -1;
    GLint m_dirDepthInstanceFlagLoc = -1;
    GLint m_pointDepthInstanceFlagLoc = -1;

    GLuint m_instanceVBO = 0;
    GLsizeiptr m_instanceBufferCapacity = 0;
    GLuint m_physicsDebugVAO = 0;
    GLuint m_physicsDebugVBO = 0;
    GLint m_physicsDebugViewProjLoc = -1;
    glm::vec3 m_lastCameraPos{ 0.0f };
    std::string m_windowTitleBase;
    std::string m_activeWindowTitle;
    bool m_metricsOverlayEnabled = false;
    float m_lastOverlayUpdate = 0.0f;
    bool m_forceOverlayUpdate = false;

    static constexpr std::size_t kCpuHistorySize = 240;
    static constexpr std::size_t kMaxDebugMessages = 64;
    std::array<float, kCpuHistorySize> m_cpuFrameHistory{};
    std::size_t m_cpuHistoryIndex = 0;
    bool m_cpuHistoryWrapped = false;
    CpuFrameStats m_cpuStats{};

    std::vector<RendererDebugMessage> m_debugMessages;
    bool m_gpuTimersAvailable = false;
    GpuTimer m_directionalShadowTimer;
    GpuTimer m_pointShadowTimer;
    GpuTimer m_sceneTimer;
    GpuTimer m_postProcessTimer;
    GpuTimingSummary m_gpuTimingSummary;
    std::string m_overlayStatusMessage;
    float m_lastFps = 0.0f;
};


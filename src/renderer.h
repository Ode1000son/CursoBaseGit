#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "camera.h"
#include "light_manager.h"
#include "material.h"
#include "model.h"
#include "texture.h"

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

struct SceneObjectTransform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct SceneObject
{
    SceneObject() = default;
    SceneObject(std::string objectName, Model* objectModel, const SceneObjectTransform& transform);

    glm::mat4 GetModelMatrix() const;

    std::string name;
    Model* model{ nullptr };
    SceneObjectTransform transform{};
    SceneObjectTransform baseTransform{};
};

class Scene
{
public:
    void Reserve(std::size_t count);
    SceneObject& AddObject(std::string name, Model* model, const SceneObjectTransform& transform);
    std::vector<SceneObject>& Objects();
    const std::vector<SceneObject>& Objects() const;

private:
    std::vector<SceneObject> m_objects;
};

struct MultiRenderTargetFramebuffer
{
    GLuint fbo = 0;
    std::array<GLuint, 2> colorAttachments{ 0, 0 };
    GLuint depthBuffer = 0;
    int width = 0;
    int height = 0;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();

    void RenderFrame(GLFWwindow* window, const Camera& camera, float currentTime);

    void SetOverrideMode(TextureOverrideMode mode);
    TextureOverrideMode GetOverrideMode() const { return m_overrideMode; }

private:
    bool EnsureOffscreenSize(int width, int height);
    bool CreateShaders();
    void DestroyShaders();
    bool CreateFullscreenQuad();
    void DestroyFullscreenQuad();
    bool LoadSceneAssets();
    void SetupSceneGraph();
    void SetupLights();
    bool SetupShadowResources();
    void UpdateSceneAnimation(float currentTime);
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
    void DrawSceneObjects(GLint modelLocation, GLuint program, GLuint fallbackTexture);
    void ApplyOverrideMode(TextureOverrideMode mode);
    bool EnsureFramebufferSize(MultiRenderTargetFramebuffer& framebuffer, int width, int height);
    void DestroyFramebuffer(MultiRenderTargetFramebuffer& framebuffer);

    bool m_initialized = false;

    ShaderProgram m_sceneShader;
    ShaderProgram m_directionalDepthShader;
    ShaderProgram m_pointDepthShader;
    ShaderProgram m_postProcessShader;

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

    Scene m_scene;
    SceneObject* m_characterObject = nullptr;
    SceneObject* m_carObject = nullptr;

    Model m_characterModel;
    Model m_floorModel;
    Model m_carModel;

    Texture m_characterTexture;
    Texture m_floorTexture;
    Texture m_checkerTexture;
    Texture m_highlightTexture;

    TextureOverrideMode m_overrideMode = TextureOverrideMode::Imported;
    std::vector<Model*> m_sceneModels;

    glm::vec3 m_pointLightOrbitCenter{ 0.0f, 1.8f, 0.0f };
    float m_pointLightOrbitRadius = 3.8f;
    float m_pointLightVerticalAmplitude = 0.7f;
    glm::vec3 m_shadowLightPos{ 0.0f };

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
};


#pragma once

#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "material.h"
#include "model.h"
#include "texture.h"
#include "light_manager.h"

struct SceneCameraSettings
{
    glm::vec3 position{ 0.0f, 1.5f, 5.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    float yaw = -90.0f;
    float pitch = -20.0f;
    float movementSpeed = 3.5f;
    float mouseSensitivity = 0.12f;
    float zoom = 60.0f;
};

struct ScenePointLightDefinition
{
    PointLight light{};
    bool castsShadows = false;
    struct OrbitSettings
    {
        bool enabled = false;
        glm::vec3 center{ 0.0f };
        float radius = 0.0f;
        float speed = 1.0f;
        float verticalAmplitude = 0.0f;
        float verticalFrequency = 0.7f;
    } orbit;
};

struct SceneLightingSetup
{
    std::vector<DirectionalLight> directionalLights;
    std::vector<ScenePointLightDefinition> pointLights;
};

struct SceneObjectTransform
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct SceneObjectLOD
{
    Model* model = nullptr;
    float maxDistance = 0.0f;
};

struct SceneInstancedBatch
{
    Model* model = nullptr;
    std::vector<glm::mat4> transforms;
    float baseRadius = 1.0f;
};

struct InstancedBatchConfig
{
    std::string name;
    std::string modelKey;
    int rings = 1;
    int instancesPerRing = 1;
    float radiusStart = 1.0f;
    float radiusStep = 0.0f;
    float heightBase = 0.0f;
    float heightStep = 0.0f;
    float scaleBase = 1.0f;
    float scaleStep = 0.0f;
    float heightScaleBase = 1.0f;
    float heightScaleStep = 0.0f;
    float twistMultiplier = 0.0f;
};

class SceneObject
{
public:
    SceneObject() = default;
    SceneObject(std::string name, Model* model, const SceneObjectTransform& transform);

    const std::string& GetName() const { return m_name; }
    Model* GetModel() const { return m_model; }
    SceneObjectTransform& Transform() { return m_transform; }
    const SceneObjectTransform& Transform() const { return m_transform; }
    const SceneObjectTransform& BaseTransform() const { return m_baseTransform; }
    glm::mat4 GetModelMatrix() const;
    glm::vec3 GetWorldCenter() const;
    glm::vec3 GetWorldCenter(const glm::mat4& modelMatrix) const;
    float GetWorldRadius() const;
    bool HasBounds() const { return m_hasBounds; }
    void SetBounds(const glm::vec3& center, float radius);
    void SetLODLevels(std::vector<SceneObjectLOD>&& lods);
    Model* ResolveModelForDistance(float distance) const;

    void ResetToBase();
    void ApplyTransform(const SceneObjectTransform& transform);

private:
    std::string m_name;
    Model* m_model = nullptr;
    SceneObjectTransform m_transform{};
    SceneObjectTransform m_baseTransform{};
    glm::vec3 m_boundsCenter{ 0.0f };
    float m_boundsRadius = 1.0f;
    bool m_hasBounds = false;
    std::vector<SceneObjectLOD> m_lodLevels;
};

class Scene
{
public:
    bool Initialize();
    void Update(float currentTime);

    const std::vector<SceneObject>& GetObjects() const { return m_objects; }
    SceneObject* GetCharacterObject() { return m_characterObject; }
    SceneObject* GetCarObject() { return m_carObject; }
    const std::vector<Model*>& GetModelPointers() const { return m_modelPointers; }
    const std::vector<SceneInstancedBatch>& GetInstancedBatches() const { return m_instancedBatches; }
    const SceneCameraSettings& GetCameraSettings() const { return m_cameraSettings; }
    const SceneLightingSetup& GetLightingSetup() const { return m_lightingSetup; }

private:
    bool LoadModels();
    bool LoadTextures();
    bool LoadSceneDefinition(const std::string& path);
    void ApplyBaseMaterials();
    void BuildInstancedBatches();
    Model* FindModel(const std::string& key);
    void RegisterModel(const std::string& key, Model* model);

    std::array<Model, 6> m_fishLodModels;
    Model m_floorModel;
    Model m_carModel;
    Model m_pillarModel;

    Texture m_floorTexture;

    std::vector<SceneObject> m_objects;
    std::vector<SceneInstancedBatch> m_instancedBatches;
    SceneObject* m_characterObject = nullptr;
    SceneObject* m_carObject = nullptr;
    std::vector<Model*> m_modelPointers;
    SceneCameraSettings m_cameraSettings;
    SceneLightingSetup m_lightingSetup;
    std::vector<InstancedBatchConfig> m_instancedBatchConfigs;
    std::unordered_map<std::string, Model*> m_modelLookup;
};


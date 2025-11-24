#pragma once

// Sistema de cena que gerencia objetos 3D, iluminação, configuração de câmera
// e carregamento de definições via JSON. Suporta LOD, instancing e animações.

#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "material.h"
#include "model.h"
#include "texture.h"
#include "light_manager.h"

// Configuração inicial da câmera da cena
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

enum class PhysicsShapeType
{
    Sphere,
    Box
};

enum class PhysicsBodyMode
{
    Solid,
    Container
};

struct SceneObjectPhysics
{
    bool enabled = false;
    PhysicsShapeType shape = PhysicsShapeType::Sphere;
    PhysicsBodyMode mode = PhysicsBodyMode::Solid;
    bool autoRadius = true;
    bool autoHalfExtents = true;
    bool alignToBounds = true;
    float radius = 0.5f;
    glm::vec3 halfExtents{ 0.5f };
    float mass = 1.0f;
    glm::vec3 initialVelocity{ 0.0f };
    float linearDamping = 0.15f;
    float angularDamping = 0.01f;
    float restitution = 0.35f;
    float friction = 0.7f;
};

// Objeto da cena com transformações, LOD e bounds para frustum culling
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
    // Calcula matriz de modelo a partir da transformação
    glm::mat4 GetModelMatrix() const;
    // Retorna centro do objeto no espaço mundial
    glm::vec3 GetWorldCenter() const;
    glm::vec3 GetWorldCenter(const glm::mat4& modelMatrix) const;
    // Retorna raio do bounding sphere
    float GetWorldRadius() const;
    glm::vec3 GetScaledHalfExtents() const;
    glm::vec3 GetLocalBoundsCenter() const { return m_boundsCenter; }
    float GetLocalBoundsRadius() const { return m_boundsRadius; }
    bool HasBounds() const { return m_hasBounds; }
    void SetBounds(const glm::vec3& center, float radius);
    // Define níveis de LOD baseados em distância
    void SetLODLevels(std::vector<SceneObjectLOD>&& lods);
    // Seleciona modelo LOD apropriado para a distância fornecida
    Model* ResolveModelForDistance(float distance) const;

    // Restaura transformação para o estado base
    void ResetToBase();
    // Aplica uma nova transformação
    void ApplyTransform(const SceneObjectTransform& transform);
    void ApplyPhysicsPose(const glm::vec3& position, const glm::quat& rotation);

    bool HasPhysicsDefinition() const { return m_hasPhysicsDefinition; }
    const SceneObjectPhysics& GetPhysicsDefinition() const { return m_physicsDefinition; }
    void SetPhysicsDefinition(const SceneObjectPhysics& definition);
    void ClearPhysicsDefinition();

private:
    std::string m_name;
    Model* m_model = nullptr;
    SceneObjectTransform m_transform{};
    SceneObjectTransform m_baseTransform{};
    glm::vec3 m_boundsCenter{ 0.0f };
    float m_boundsRadius = 1.0f;
    bool m_hasBounds = false;
    std::vector<SceneObjectLOD> m_lodLevels;
    SceneObjectPhysics m_physicsDefinition{};
    bool m_hasPhysicsDefinition = false;
};

// Gerencia toda a cena: objetos, modelos, iluminação e configurações
// Carrega definição JSON e constrói batches instanciados
class Scene
{
public:
    // Carrega modelos, texturas e definição JSON da cena
    bool Initialize();
    // Atualiza animações e transformações da cena
    void Update(float currentTime);
    bool Reload();

    const std::vector<SceneObject>& GetObjects() const { return m_objects; }
    std::vector<SceneObject>& GetMutableObjects() { return m_objects; }
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
    std::string m_lastScenePath;
};


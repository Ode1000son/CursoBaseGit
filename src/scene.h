#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "material.h"
#include "model.h"
#include "texture.h"

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

private:
    bool LoadModels();
    bool LoadTextures();
    void BuildSceneGraph();
    void ApplyBaseMaterials();
    void BuildInstancedBatches();

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
};


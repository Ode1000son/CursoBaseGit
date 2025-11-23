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

    void ResetToBase();
    void ApplyTransform(const SceneObjectTransform& transform);

private:
    std::string m_name;
    Model* m_model = nullptr;
    SceneObjectTransform m_transform{};
    SceneObjectTransform m_baseTransform{};
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

private:
    bool LoadModels();
    bool LoadTextures();
    void BuildSceneGraph();
    void ApplyBaseMaterials();

    Model m_characterModel;
    Model m_floorModel;
    Model m_carModel;

    Texture m_characterTexture;
    Texture m_floorTexture;

    std::vector<SceneObject> m_objects;
    SceneObject* m_characterObject = nullptr;
    SceneObject* m_carObject = nullptr;
    std::vector<Model*> m_modelPointers;
};


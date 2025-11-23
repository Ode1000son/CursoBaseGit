#include "scene.h"

#include <iostream>
#include <cmath>

SceneObject::SceneObject(std::string name, Model* model, const SceneObjectTransform& transform)
    : m_name(std::move(name))
    , m_model(model)
    , m_transform(transform)
    , m_baseTransform(transform)
{
}

glm::mat4 SceneObject::GetModelMatrix() const
{
    glm::mat4 modelMatrix(1.0f);
    modelMatrix = glm::translate(modelMatrix, m_transform.position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(m_transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, m_transform.scale);
    return modelMatrix;
}

void SceneObject::ResetToBase()
{
    m_transform = m_baseTransform;
}

void SceneObject::ApplyTransform(const SceneObjectTransform& transform)
{
    m_transform = transform;
}

bool Scene::Initialize()
{
    if (!LoadModels() || !LoadTextures())
    {
        return false;
    }

    ApplyBaseMaterials();
    BuildSceneGraph();
    m_modelPointers = { &m_characterModel, &m_floorModel, &m_carModel };
    return true;
}

void Scene::Update(float currentTime)
{
    if (m_characterObject != nullptr)
    {
        SceneObjectTransform transform = m_characterObject->BaseTransform();
        transform.position.y += 0.05f * std::sin(currentTime * 1.5f);
        transform.rotation.y += std::sin(currentTime * 0.3f) * 15.0f;
        m_characterObject->ApplyTransform(transform);
    }

    if (m_carObject != nullptr)
    {
        SceneObjectTransform transform = m_carObject->BaseTransform();
        transform.position.x += std::cos(currentTime * 0.4f) * 0.8f;
        transform.position.z += std::sin(currentTime * 0.4f) * 0.6f;
        transform.position.y += 0.02f * std::sin(currentTime * 2.2f);
        transform.rotation.y += static_cast<float>(std::fmod(currentTime * 45.0f, 360.0f));
        m_carObject->ApplyTransform(transform);
    }
}

bool Scene::LoadModels()
{
    if (!m_characterModel.LoadFromFile("assets/models/scene.gltf") || !m_characterModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo 3D (scene.gltf)." << std::endl;
        return false;
    }

    if (!m_floorModel.LoadFromFile("assets/models/cube.gltf") || !m_floorModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo do chão (cube.gltf)." << std::endl;
        return false;
    }

    if (!m_carModel.LoadFromFile("assets/models/car.glb") || !m_carModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo do carro (car.glb)." << std::endl;
        return false;
    }

    return true;
}

bool Scene::LoadTextures()
{
    if (!m_characterTexture.LoadFromFile("assets/models/Vitalik_edit_2.png"))
    {
        std::cerr << "Falha ao carregar textura do personagem (Vitalik_edit_2.png)." << std::endl;
    }

    if (!m_floorTexture.LoadFromFile("assets/models/CubeTexture.jpg"))
    {
        std::cerr << "Falha ao carregar textura do chão (CubeTexture.jpg)." << std::endl;
    }

    return true;
}

void Scene::ApplyBaseMaterials()
{
    if (m_characterTexture.GetID() != 0)
    {
        m_characterModel.ApplyTextureIfMissing(&m_characterTexture);
    }

    if (m_floorTexture.GetID() != 0)
    {
        m_floorModel.ApplyTextureIfMissing(&m_floorTexture);
    }

    m_carModel.ForEachMaterial([](Material& material) {
        material.SetSpecular(glm::vec3(0.0f));
        material.SetShininess(1.0f);
    });
}

void Scene::BuildSceneGraph()
{
    m_objects.clear();
    m_objects.reserve(4);

    m_objects.emplace_back("Floor", &m_floorModel, SceneObjectTransform{
        glm::vec3(0.0f, -0.15f, 0.0f),
        glm::vec3(0.0f),
        glm::vec3(10.0f, 0.2f, 10.0f)
    });

    m_objects.emplace_back("Character", &m_characterModel, SceneObjectTransform{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -90.0f, 0.0f),
        glm::vec3(1.0f)
    });

    m_objects.emplace_back("Car", &m_carModel, SceneObjectTransform{
        glm::vec3(2.5f, -0.05f, -2.5f),
        glm::vec3(0.0f, 90.0f, 0.0f),
        glm::vec3(0.9f)
    });

    m_characterObject = &m_objects[1];
    m_carObject = &m_objects[2];
}


#include "scene.h"

#include <algorithm>
#include <iostream>
#include <cmath>
#include <array>
#include <limits>
#include <assimp/Importer.hpp>
#include <glm/gtc/constants.hpp>

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

glm::vec3 SceneObject::GetWorldCenter() const
{
    const glm::mat4 modelMatrix = GetModelMatrix();
    return GetWorldCenter(modelMatrix);
}

glm::vec3 SceneObject::GetWorldCenter(const glm::mat4& modelMatrix) const
{
    if (m_hasBounds) {
        return glm::vec3(modelMatrix * glm::vec4(m_boundsCenter, 1.0f));
    }
    return m_transform.position;
}

float SceneObject::GetWorldRadius() const
{
    const float maxScale = std::max({ std::abs(m_transform.scale.x),
                                      std::abs(m_transform.scale.y),
                                      std::abs(m_transform.scale.z) });
    if (m_hasBounds) {
        return m_boundsRadius * maxScale;
    }
    return maxScale;
}

void SceneObject::SetBounds(const glm::vec3& center, float radius)
{
    m_boundsCenter = center;
    m_boundsRadius = radius;
    m_hasBounds = true;
}

void SceneObject::SetLODLevels(std::vector<SceneObjectLOD>&& lods)
{
    m_lodLevels = std::move(lods);
}

Model* SceneObject::ResolveModelForDistance(float distance) const
{
    if (m_lodLevels.empty()) {
        return m_model;
    }

    for (const auto& lod : m_lodLevels) {
        if (distance <= lod.maxDistance && lod.model != nullptr) {
            return lod.model;
        }
    }
    return m_lodLevels.back().model != nullptr ? m_lodLevels.back().model : m_model;
}

bool Scene::Initialize()
{
    if (!LoadModels() || !LoadTextures())
    {
        return false;
    }

    ApplyBaseMaterials();
    BuildSceneGraph();
    BuildInstancedBatches();

    m_modelPointers.clear();
    for (auto& model : m_fishLodModels)
    {
        m_modelPointers.push_back(&model);
    }
    m_modelPointers.push_back(&m_floorModel);
    m_modelPointers.push_back(&m_carModel);
    m_modelPointers.push_back(&m_pillarModel);
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
    static const std::array<const char*, 6> kFishNodes = {
        "Fish_LOD0",
        "Fish_LOD1",
        "Fish_LOD2",
        "Fish_LOD3",
        "Fish_LOD4",
        "Fish_LOD5"
    };

    static const std::array<const char*, 6> kFishMeshes = {
        "Sphere.004",
        "Sphere.006",
        "Sphere.008",
        "Sphere.010",
        "Sphere.012",
        "Sphere.014"
    };

    const std::string fishPath = "assets/models/Fish.glb";
    Assimp::Importer fishImporter;
    unsigned int fishImportFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_OptimizeMeshes;

    const aiScene* fishScene = fishImporter.ReadFile(fishPath, fishImportFlags);
    if (!fishScene || fishScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !fishScene->mRootNode)
    {
        std::cerr << "Falha ao carregar modelo do peixe (Fish.glb): "
                  << fishImporter.GetErrorString() << std::endl;
        return false;
    }

    std::string fishDirectory;
    const size_t fishLastSlash = fishPath.find_last_of("/\\");
    if (fishLastSlash != std::string::npos)
    {
        fishDirectory = fishPath.substr(0, fishLastSlash);
    }

    for (std::size_t i = 0; i < kFishNodes.size(); ++i)
    {
        const std::vector<std::string> identifiers{ kFishNodes[i], kFishMeshes[i] };
        if (!m_fishLodModels[i].LoadFromScene(fishScene, fishDirectory, identifiers) || !m_fishLodModels[i].HasMeshes())
        {
            std::cerr << "Falha ao carregar LOD " << i << " do peixe (" << kFishNodes[i] << ")." << std::endl;
            return false;
        }
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

    if (!m_pillarModel.LoadFromFile("assets/models/cube.gltf") || !m_pillarModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo para instancing (cube.gltf)." << std::endl;
        return false;
    }

    return true;
}

bool Scene::LoadTextures()
{
    if (!m_floorTexture.LoadFromFile("assets/models/CubeTexture.jpg"))
    {
        std::cerr << "Falha ao carregar textura do chão (CubeTexture.jpg)." << std::endl;
    }

    return true;
}

void Scene::ApplyBaseMaterials()
{
    if (m_floorTexture.GetID() != 0)
    {
        m_floorModel.ApplyTextureIfMissing(&m_floorTexture);
        m_pillarModel.ApplyTextureIfMissing(&m_floorTexture);
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
    if (m_floorModel.HasBounds())
    {
        m_objects.back().SetBounds(m_floorModel.GetBoundingCenter(), m_floorModel.GetBoundingRadius());
    }

    SceneObject fish("HeroFish", &m_fishLodModels[0], SceneObjectTransform{
        glm::vec3(0.0f, 0.2f, 0.0f),
        glm::vec3(0.0f, -90.0f, 0.0f),
        glm::vec3(0.8f)
    });

    std::vector<SceneObjectLOD> fishLods;
    const std::array<float, 6> kLodDistances = {
        10.0f, 18.0f, 28.0f, 40.0f, 55.0f, std::numeric_limits<float>::max()
    };
    for (std::size_t i = 0; i < m_fishLodModels.size(); ++i)
    {
        fishLods.push_back(SceneObjectLOD{ &m_fishLodModels[i], kLodDistances[i] });
    }
    fish.SetLODLevels(std::move(fishLods));
    if (m_fishLodModels[0].HasBounds())
    {
        fish.SetBounds(m_fishLodModels[0].GetBoundingCenter(), m_fishLodModels[0].GetBoundingRadius());
    }
    m_objects.push_back(fish);

    m_objects.emplace_back("Car", &m_carModel, SceneObjectTransform{
        glm::vec3(2.5f, -0.05f, -2.5f),
        glm::vec3(0.0f, 90.0f, 0.0f),
        glm::vec3(0.9f)
    });
    if (m_carModel.HasBounds())
    {
        m_objects.back().SetBounds(m_carModel.GetBoundingCenter(), m_carModel.GetBoundingRadius());
    }

    m_characterObject = &m_objects[1];
    m_carObject = &m_objects[2];
}

void Scene::BuildInstancedBatches()
{
    m_instancedBatches.clear();

    SceneInstancedBatch pillars;
    pillars.model = &m_pillarModel;
    pillars.baseRadius = m_pillarModel.HasBounds() ? m_pillarModel.GetBoundingRadius() : 0.5f;

    constexpr int kInstanceCount = 140;
    pillars.transforms.reserve(kInstanceCount);

    for (int i = 0; i < kInstanceCount; ++i)
    {
        const float ringIndex = static_cast<float>(i % 5);
        const float radius = 6.5f + ringIndex * 0.7f;
        const float angle = glm::two_pi<float>() * (static_cast<float>(i) / static_cast<float>(kInstanceCount));

        glm::mat4 transform(1.0f);
        transform = glm::translate(transform, glm::vec3(std::cos(angle) * radius,
                                                        -0.12f + 0.03f * ringIndex,
                                                        std::sin(angle) * radius));
        transform = glm::rotate(transform, angle * 1.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        const float baseScale = 0.18f + 0.02f * ringIndex;
        transform = glm::scale(transform, glm::vec3(baseScale, baseScale * (2.5f + ringIndex * 0.4f), baseScale));
        pillars.transforms.push_back(transform);
    }

    m_instancedBatches.push_back(std::move(pillars));
}


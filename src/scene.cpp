#include "scene.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <array>
#include <limits>
#include <cctype>
#include <assimp/Importer.hpp>
#include <glm/gtc/constants.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
glm::vec3 ParseVec3(const json& node, const glm::vec3& fallback)
{
    glm::vec3 value = fallback;
    if (node.is_object())
    {
        value.x = node.value("x", fallback.x);
        value.y = node.value("y", fallback.y);
        value.z = node.value("z", fallback.z);
    }
    return value;
}

SceneObjectTransform ParseTransform(const json& node)
{
    SceneObjectTransform transform;
    if (!node.is_object())
    {
        return transform;
    }
    transform.position = ParseVec3(node.value("position", json::object()), transform.position);
    transform.rotation = ParseVec3(node.value("rotation", json::object()), transform.rotation);
    transform.scale = ParseVec3(node.value("scale", json::object()), transform.scale);
    return transform;
}

void EnsureDefaultLighting(SceneLightingSetup& lighting)
{
    if (lighting.directionalLights.empty())
    {
        DirectionalLight primary{};
        primary.direction = glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
        primary.ambient = glm::vec3(0.25f, 0.22f, 0.20f);
        primary.diffuse = glm::vec3(0.9f, 0.85f, 0.8f);
        primary.specular = glm::vec3(1.0f);
        lighting.directionalLights.push_back(primary);

        DirectionalLight fill{};
        fill.direction = glm::normalize(glm::vec3(0.3f, -1.0f, 0.15f));
        fill.ambient = glm::vec3(0.02f, 0.02f, 0.03f);
        fill.diffuse = glm::vec3(0.35f, 0.4f, 0.55f);
        fill.specular = glm::vec3(0.25f, 0.3f, 0.45f);
        lighting.directionalLights.push_back(fill);
    }

    if (lighting.pointLights.empty())
    {
        ScenePointLightDefinition shadowCaster{};
        shadowCaster.light.position = glm::vec3(0.0f, 2.8f, 0.0f);
        shadowCaster.light.ambient = glm::vec3(0.03f);
        shadowCaster.light.diffuse = glm::vec3(1.0f, 0.85f, 0.6f);
        shadowCaster.light.specular = glm::vec3(1.0f, 0.95f, 0.9f);
        shadowCaster.light.linear = 0.09f;
        shadowCaster.light.quadratic = 0.032f;
        shadowCaster.light.range = 18.0f;
        shadowCaster.castsShadows = true;
        shadowCaster.orbit.enabled = true;
        shadowCaster.orbit.center = glm::vec3(0.0f, 1.8f, 0.0f);
        shadowCaster.orbit.radius = 3.8f;
        shadowCaster.orbit.speed = 1.0f;
        shadowCaster.orbit.verticalAmplitude = 0.7f;
        shadowCaster.orbit.verticalFrequency = 0.7f;
        lighting.pointLights.push_back(shadowCaster);

        ScenePointLightDefinition accent{};
        accent.light.position = glm::vec3(-3.0f, 3.5f, -2.0f);
        accent.light.ambient = glm::vec3(0.04f, 0.05f, 0.06f);
        accent.light.diffuse = glm::vec3(0.55f, 0.65f, 1.0f);
        accent.light.specular = glm::vec3(0.35f, 0.40f, 0.55f);
        accent.light.linear = 0.14f;
        accent.light.quadratic = 0.07f;
        accent.light.range = 12.0f;
        lighting.pointLights.push_back(accent);
    }
}
}

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
    m_modelLookup.clear();
    if (!LoadModels() || !LoadTextures())
    {
        return false;
    }

    ApplyBaseMaterials();
    if (!LoadSceneDefinition("assets/scenes/final_scene.json"))
    {
        return false;
    }
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
        RegisterModel("FishLOD" + std::to_string(i), &m_fishLodModels[i]);
    }
    RegisterModel("Fish", &m_fishLodModels[0]);
    RegisterModel("HeroFish", &m_fishLodModels[0]);

    if (!m_floorModel.LoadFromFile("assets/models/cube.gltf") || !m_floorModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo do chão (cube.gltf)." << std::endl;
        return false;
    }
    RegisterModel("Floor", &m_floorModel);

    if (!m_carModel.LoadFromFile("assets/models/car.glb") || !m_carModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo do carro (car.glb)." << std::endl;
        return false;
    }
    RegisterModel("Car", &m_carModel);

    if (!m_pillarModel.LoadFromFile("assets/models/cube.gltf") || !m_pillarModel.HasMeshes())
    {
        std::cerr << "Falha ao carregar modelo para instancing (cube.gltf)." << std::endl;
        return false;
    }
    RegisterModel("Pillar", &m_pillarModel);

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

bool Scene::LoadSceneDefinition(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Falha ao abrir arquivo de cena: " << path << std::endl;
        return false;
    }

    json document;
    try
    {
        file >> document;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Erro ao parsear cena JSON (" << path << "): " << e.what() << std::endl;
        return false;
    }

    m_objects.clear();
    m_instancedBatchConfigs.clear();
    m_characterObject = nullptr;
    m_carObject = nullptr;

    const json cameraNode = document.value("camera", json::object());
    m_cameraSettings.position = ParseVec3(cameraNode.value("position", json::object()), m_cameraSettings.position);
    m_cameraSettings.up = ParseVec3(cameraNode.value("up", json::object()), m_cameraSettings.up);
    m_cameraSettings.yaw = cameraNode.value("yaw", m_cameraSettings.yaw);
    m_cameraSettings.pitch = cameraNode.value("pitch", m_cameraSettings.pitch);
    m_cameraSettings.movementSpeed = cameraNode.value("movementSpeed", m_cameraSettings.movementSpeed);
    m_cameraSettings.mouseSensitivity = cameraNode.value("mouseSensitivity", m_cameraSettings.mouseSensitivity);
    m_cameraSettings.zoom = cameraNode.value("zoom", m_cameraSettings.zoom);

    const json lightingNode = document.value("lighting", json::object());
    m_lightingSetup.directionalLights.clear();
    m_lightingSetup.pointLights.clear();
    if (lightingNode.is_object())
    {
        if (const auto dirsIt = lightingNode.find("directional"); dirsIt != lightingNode.end() && dirsIt->is_array())
        {
            for (const auto& dirJson : *dirsIt)
            {
                DirectionalLight light{};
                light.direction = ParseVec3(dirJson.value("direction", json::object()), glm::vec3(-0.4f, -1.0f, -0.3f));
                light.ambient = ParseVec3(dirJson.value("ambient", json::object()), glm::vec3(0.25f, 0.22f, 0.20f));
                light.diffuse = ParseVec3(dirJson.value("diffuse", json::object()), glm::vec3(0.9f, 0.85f, 0.8f));
                light.specular = ParseVec3(dirJson.value("specular", json::object()), glm::vec3(1.0f));
                light.animated = dirJson.value("animated", light.animated);
                light.animationAxis = ParseVec3(dirJson.value("animationAxis", json::object()), light.animationAxis);
                light.animationSpeed = dirJson.value("animationSpeed", light.animationSpeed);
                m_lightingSetup.directionalLights.push_back(light);
            }
        }

        if (const auto pointIt = lightingNode.find("point"); pointIt != lightingNode.end() && pointIt->is_array())
        {
            for (const auto& pointJson : *pointIt)
            {
                ScenePointLightDefinition definition{};
                definition.light.position = ParseVec3(pointJson.value("position", json::object()), definition.light.position);
                definition.light.ambient = ParseVec3(pointJson.value("ambient", json::object()), definition.light.ambient);
                definition.light.diffuse = ParseVec3(pointJson.value("diffuse", json::object()), definition.light.diffuse);
                definition.light.specular = ParseVec3(pointJson.value("specular", json::object()), definition.light.specular);
                definition.light.constant = pointJson.value("constant", definition.light.constant);
                definition.light.linear = pointJson.value("linear", definition.light.linear);
                definition.light.quadratic = pointJson.value("quadratic", definition.light.quadratic);
                definition.light.range = pointJson.value("range", definition.light.range);
                definition.castsShadows = pointJson.value("castsShadows", definition.castsShadows);
                if (const auto orbitIt = pointJson.find("orbit"); orbitIt != pointJson.end() && orbitIt->is_object())
                {
                    definition.orbit.enabled = orbitIt->value("enabled", definition.orbit.enabled);
                    definition.orbit.center = ParseVec3(orbitIt->value("center", json::object()), definition.orbit.center);
                    definition.orbit.radius = orbitIt->value("radius", definition.orbit.radius);
                    definition.orbit.speed = orbitIt->value("speed", definition.orbit.speed);
                    definition.orbit.verticalAmplitude = orbitIt->value("verticalAmplitude", definition.orbit.verticalAmplitude);
                    definition.orbit.verticalFrequency = orbitIt->value("verticalFrequency", definition.orbit.verticalFrequency);
                }
                m_lightingSetup.pointLights.push_back(definition);
            }
        }
    }
    EnsureDefaultLighting(m_lightingSetup);

    const auto objectsIt = document.find("objects");
    if (objectsIt == document.end() || !objectsIt->is_array())
    {
        std::cerr << "Cena JSON precisa de um array 'objects'." << std::endl;
        return false;
    }

    for (const auto& objectJson : *objectsIt)
    {
        const std::string name = objectJson.value("name", "UnnamedObject");
        const std::string modelKey = objectJson.value("model", "");
        Model* model = FindModel(modelKey);
        if (model == nullptr)
        {
            std::cerr << "Objeto '" << name << "' referencia modelo desconhecido '" << modelKey << "'." << std::endl;
            continue;
        }

        SceneObjectTransform transform = ParseTransform(objectJson.value("transform", json::object()));
        m_objects.emplace_back(name, model, transform);
        SceneObject& created = m_objects.back();
        if (model->HasBounds())
        {
            created.SetBounds(model->GetBoundingCenter(), model->GetBoundingRadius());
        }

        if (const auto lodIt = objectJson.find("lods"); lodIt != objectJson.end() && lodIt->is_array())
        {
            std::vector<SceneObjectLOD> lods;
            lods.reserve(lodIt->size());
            for (const auto& lodJson : *lodIt)
            {
                const std::string lodKey = lodJson.value("model", "");
                Model* lodModel = FindModel(lodKey);
                if (lodModel == nullptr)
                {
                    std::cerr << "LOD de '" << name << "' referencia modelo desconhecido '" << lodKey << "'." << std::endl;
                    continue;
                }
                const float maxDistance = lodJson.value("maxDistance", std::numeric_limits<float>::max());
                lods.push_back(SceneObjectLOD{ lodModel, maxDistance });
                if (!created.HasBounds() && lodModel->HasBounds())
                {
                    created.SetBounds(lodModel->GetBoundingCenter(), lodModel->GetBoundingRadius());
                }
            }
            if (!lods.empty())
            {
                created.SetLODLevels(std::move(lods));
            }
        }

        const std::string role = objectJson.value("role", "");
        if (role == "hero")
        {
            m_characterObject = &created;
        }
        else if (role == "vehicle")
        {
            m_carObject = &created;
        }
    }

    const auto batchesIt = document.find("instancedBatches");
    if (batchesIt != document.end() && batchesIt->is_array())
    {
        for (const auto& batchJson : *batchesIt)
        {
            InstancedBatchConfig config;
            config.name = batchJson.value("name", "Batch");
            config.modelKey = batchJson.value("model", "");
            config.rings = std::max(1, batchJson.value("rings", 1));
            config.instancesPerRing = std::max(1, batchJson.value("instancesPerRing", 1));
            config.radiusStart = batchJson.value("radiusStart", 1.0f);
            config.radiusStep = batchJson.value("radiusStep", 0.0f);
            config.heightBase = batchJson.value("heightBase", 0.0f);
            config.heightStep = batchJson.value("heightStep", 0.0f);
            config.scaleBase = batchJson.value("scaleBase", 1.0f);
            config.scaleStep = batchJson.value("scaleStep", 0.0f);
            config.heightScaleBase = batchJson.value("heightScaleBase", 1.0f);
            config.heightScaleStep = batchJson.value("heightScaleStep", 0.0f);
            config.twistMultiplier = batchJson.value("twistMultiplier", 0.0f);
            m_instancedBatchConfigs.push_back(config);
        }
    }

    if (m_instancedBatchConfigs.empty())
    {
        InstancedBatchConfig fallback;
        fallback.name = "FallbackRing";
        fallback.modelKey = "Pillar";
        fallback.rings = 5;
        fallback.instancesPerRing = 28;
        fallback.radiusStart = 6.5f;
        fallback.radiusStep = 0.7f;
        fallback.heightBase = -0.12f;
        fallback.heightStep = 0.03f;
        fallback.scaleBase = 0.18f;
        fallback.scaleStep = 0.02f;
        fallback.heightScaleBase = 2.5f;
        fallback.heightScaleStep = 0.4f;
        fallback.twistMultiplier = 1.3f;
        m_instancedBatchConfigs.push_back(fallback);
    }

    return true;
}

void Scene::BuildInstancedBatches()
{
    m_instancedBatches.clear();
    for (const auto& config : m_instancedBatchConfigs)
    {
        Model* model = FindModel(config.modelKey);
        if (model == nullptr)
        {
            std::cerr << "Batch instanciado '" << config.name << "' referencia modelo desconhecido '"
                      << config.modelKey << "'." << std::endl;
            continue;
        }

        SceneInstancedBatch batch;
        batch.model = model;
        batch.baseRadius = model->HasBounds() ? model->GetBoundingRadius() : 0.5f;

        const int rings = std::max(1, config.rings);
        const int perRing = std::max(1, config.instancesPerRing);
        batch.transforms.reserve(static_cast<std::size_t>(rings * perRing));

        for (int ring = 0; ring < rings; ++ring)
        {
            const float factor = static_cast<float>(ring);
            for (int instance = 0; instance < perRing; ++instance)
            {
                const float angle = glm::two_pi<float>() * (static_cast<float>(instance) / static_cast<float>(perRing));
                glm::mat4 transform(1.0f);
                const float radius = config.radiusStart + config.radiusStep * factor;
                const float height = config.heightBase + config.heightStep * factor;
                transform = glm::translate(transform, glm::vec3(std::cos(angle) * radius,
                                                                height,
                                                                std::sin(angle) * radius));
                const float twist = angle * config.twistMultiplier;
                transform = glm::rotate(transform, twist, glm::vec3(0.0f, 1.0f, 0.0f));
                const float baseScale = config.scaleBase + config.scaleStep * factor;
                const float heightScale = config.heightScaleBase + config.heightScaleStep * factor;
                transform = glm::scale(transform, glm::vec3(baseScale,
                                                            baseScale * heightScale,
                                                            baseScale));
                batch.transforms.push_back(transform);
            }
        }

        m_instancedBatches.push_back(std::move(batch));
    }
}

Model* Scene::FindModel(const std::string& key)
{
    if (key.empty())
    {
        return nullptr;
    }
    std::string normalized = key;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    auto it = m_modelLookup.find(normalized);
    if (it != m_modelLookup.end())
    {
        return it->second;
    }
    return nullptr;
}

void Scene::RegisterModel(const std::string& key, Model* model)
{
    if (key.empty() || model == nullptr)
    {
        return;
    }
    std::string normalized = key;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    m_modelLookup[normalized] = model;
}


// Implementação do sistema de modelos
// Processa cenas Assimp, extrai malhas, materiais e texturas

#include "model.h"

#include <cstddef>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cctype>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtc/type_ptr.hpp>

Mesh::Mesh(std::vector<Vertex>&& vertices,
           std::vector<unsigned int>&& indices,
           Material* material)
    : m_vertices(std::move(vertices))
    , m_indices(std::move(indices))
    , m_material(material)
    , m_VAO(0)
    , m_VBO(0)
    , m_EBO(0)
{
    SetupMesh();
}

Mesh::~Mesh()
{
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
}

void Mesh::SetupMesh()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

    glBindVertexArray(0);
}

void Mesh::Draw(GLuint program, GLuint fallbackTextureID) const
{
    bool hasTextureBound = false;
    if (m_material) {
        m_material->Apply(program);
        if (m_material->HasTexture()) {
            m_material->BindTexture(GL_TEXTURE0);
            hasTextureBound = true;
        }
    }

    if (!hasTextureBound && fallbackTextureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fallbackTextureID);
    }

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::DrawInstanced(GLuint program, GLuint fallbackTextureID, GLuint instanceVBO, GLsizei instanceCount) const
{
    if (instanceCount <= 0) {
        return;
    }

    bool hasTextureBound = false;
    if (m_material) {
        m_material->Apply(program);
        if (m_material->HasTexture()) {
            m_material->BindTexture(GL_TEXTURE0);
            hasTextureBound = true;
        }
    }

    if (!hasTextureBound && fallbackTextureID != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fallbackTextureID);
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    const std::size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(static_cast<std::size_t>(i) * vec4Size));
        glVertexAttribDivisor(3 + i, 1);
    }

    glDrawElementsInstanced(GL_TRIANGLES,
                            static_cast<GLsizei>(m_indices.size()),
                            GL_UNSIGNED_INT,
                            nullptr,
                            instanceCount);
    glBindVertexArray(0);
}

bool Model::LoadFromFile(const std::string& filePath)
{
    return LoadFromFile(filePath, {});
}

bool Model::LoadFromFile(const std::string& filePath, const std::vector<std::string>& allowedNodes)
{
    std::string directory;
    const size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        directory = filePath.substr(0, lastSlash);
    }

    unsigned int importFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph;

    if (!allowedNodes.empty())
    {
        importFlags &= ~aiProcess_OptimizeGraph;
    }

    const aiScene* scene = m_importer.ReadFile(filePath, importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Erro ao carregar modelo (" << filePath << "): "
                  << m_importer.GetErrorString() << std::endl;
        return false;
    }

    return LoadFromScene(scene, directory, allowedNodes);
}

bool Model::LoadFromScene(const aiScene* scene, const std::string& directory, const std::vector<std::string>& allowedNodes)
{
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "Cena inválida fornecida para carregamento de modelo." << std::endl;
        return false;
    }

    m_meshes.clear();
    m_materials.clear();
    m_ownedTextures.clear();
    m_directory = directory;
    m_allowedNames.clear();
    m_useNodeFilter = !allowedNodes.empty();
    if (m_useNodeFilter)
    {
        for (const auto& nodeName : allowedNodes) {
            const std::string normalized = NormalizeIdentifier(nodeName);
            if (!normalized.empty()) {
                m_allowedNames.insert(normalized);
            }
        }
    }

    ResetBounds();
    ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f), false);

    if (m_hasBounds) {
        m_boundingCenter = (m_aabbMin + m_aabbMax) * 0.5f;
        m_boundingRadius = glm::length(m_aabbMax - m_boundingCenter);
    } else {
        m_boundingCenter = glm::vec3(0.0f);
        m_boundingRadius = 0.0f;
    }

    m_useNodeFilter = false;
    m_allowedNames.clear();
    return !m_meshes.empty();
}

glm::vec3 Model::GetBoundingHalfExtents() const
{
    if (!m_hasBounds)
    {
        return glm::vec3(0.5f);
    }
    return (m_aabbMax - m_aabbMin) * 0.5f;
}

void Model::Draw(GLuint program, GLuint fallbackTextureID) const
{
    for (const auto& mesh : m_meshes) {
        mesh->Draw(program, fallbackTextureID);
    }
}

void Model::DrawInstanced(GLuint program, GLuint fallbackTextureID, GLuint instanceVBO, GLsizei instanceCount) const
{
    for (const auto& mesh : m_meshes) {
        mesh->DrawInstanced(program, fallbackTextureID, instanceVBO, instanceCount);
    }
}

void Model::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform, bool parentIncluded)
{
    const glm::mat4 nodeTransform = parentTransform * ConvertMatrix(node->mTransformation);
    const bool includeCurrentNode = parentIncluded || ShouldIncludeNode(node->mName.C_Str());

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        bool allowMesh = includeCurrentNode || !m_useNodeFilter;
        if (m_useNodeFilter && !allowMesh) {
            const std::string meshName = NormalizeIdentifier(mesh->mName.C_Str());
            allowMesh = m_allowedNames.find(meshName) != m_allowedNames.end();
        }
        if (allowMesh || !m_useNodeFilter) {
            m_meshes.emplace_back(ProcessMesh(mesh, scene, nodeTransform));
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, nodeTransform, includeCurrentNode);
    }
}

std::unique_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
{
    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        glm::vec4 position(mesh->mVertices[i].x,
                           mesh->mVertices[i].y,
                           mesh->mVertices[i].z,
                           1.0f);
        vertex.position = glm::vec3(transform * position);

        if (mesh->HasNormals()) {
            glm::vec3 normal(mesh->mNormals[i].x,
                             mesh->mNormals[i].y,
                             mesh->mNormals[i].z);
            vertex.normal = glm::normalize(normalMatrix * normal);
        } else {
            vertex.normal = glm::normalize(normalMatrix * glm::vec3(0.0f, 1.0f, 0.0f));
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                         mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = glm::vec2(0.0f);
        }

        vertices.push_back(vertex);
        UpdateBounds(vertex.position);
    }

    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Material* meshMaterial = nullptr;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        auto createdMaterial = CreateMaterial(material, scene);
        meshMaterial = createdMaterial.get();
        m_materials.emplace_back(std::move(createdMaterial));
    } else {
        auto defaultMaterial = std::make_unique<Material>();
        meshMaterial = defaultMaterial.get();
        m_materials.emplace_back(std::move(defaultMaterial));
    }

    return std::make_unique<Mesh>(std::move(vertices), std::move(indices), meshMaterial);
}

namespace
{
glm::vec3 ToVec3(const aiColor3D& color)
{
    return glm::vec3(color.r, color.g, color.b);
}

glm::vec3 ToVec3(const aiColor4D& color)
{
    return glm::vec3(color.r, color.g, color.b);
}
}

std::unique_ptr<Material> Model::CreateMaterial(aiMaterial* sourceMaterial, const aiScene* scene)
{
    if (!sourceMaterial) {
        return std::make_unique<Material>();
    }

    glm::vec3 baseColor(1.0f);
    aiColor4D tempColor4(1.0f, 1.0f, 1.0f, 1.0f);
    if (sourceMaterial->Get(AI_MATKEY_BASE_COLOR, tempColor4) == AI_SUCCESS) {
        baseColor = ToVec3(tempColor4);
    } else {
        aiColor3D tempColor3(1.0f, 1.0f, 1.0f);
        if (sourceMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, tempColor3) == AI_SUCCESS) {
            baseColor = ToVec3(tempColor3);
        }
    }

    glm::vec3 ambient = baseColor * 0.2f;
    glm::vec3 diffuse = baseColor;

    float metallic = 0.0f;
    sourceMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    metallic = glm::clamp(metallic, 0.0f, 1.0f);

    float roughness = 0.5f;
    sourceMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    roughness = glm::clamp(roughness, 0.02f, 0.98f);

    glm::vec3 specular = glm::mix(glm::vec3(0.02f), diffuse, metallic);
    float shininess = glm::mix(32.0f, 4.0f, roughness);

    auto material = std::make_unique<Material>(ambient, diffuse, specular, shininess);
    if (Texture* baseTexture = LoadMaterialTexture(sourceMaterial, aiTextureType_BASE_COLOR, scene)) {
        material->SetDiffuseTexture(baseTexture);
    } else if (Texture* diffuseTexture = LoadMaterialTexture(sourceMaterial, aiTextureType_DIFFUSE, scene)) {
        material->SetDiffuseTexture(diffuseTexture);
    }

    return material;
}

Texture* Model::LoadMaterialTexture(aiMaterial* material, aiTextureType type, const aiScene* scene)
{
    if (material->GetTextureCount(type) == 0) {
        return nullptr;
    }

    aiString texturePath;
    if (material->GetTexture(type, 0, &texturePath) != AI_SUCCESS) {
        return nullptr;
    }

    if (Texture* embedded = LoadEmbeddedTexture(scene, texturePath.C_Str())) {
        return embedded;
    }

    std::string filename = texturePath.C_Str();
    if (filename.empty()) {
        return nullptr;
    }

    const size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    std::vector<std::string> candidates;
    if (!m_directory.empty()) {
        candidates.push_back(m_directory + "/" + filename);
    }
    candidates.push_back("assets/" + filename);
    candidates.push_back("assets/textures/" + filename);

    for (const auto& path : candidates) {
        if (Texture* texture = LoadTextureFromPath(path)) {
            return texture;
        }
    }

    return nullptr;
}

void Model::OverrideAllTextures(Texture* texture)
{
    if (!texture) {
        ClearTextureOverrides();
        return;
    }

    for (auto& material : m_materials) {
        if (material) {
            material->SetDiffuseOverride(texture);
        }
    }
}

void Model::ClearTextureOverrides()
{
    for (auto& material : m_materials) {
        if (material) {
            material->ClearDiffuseOverride();
        }
    }
}

void Model::ApplyTextureIfMissing(Texture* texture)
{
    if (!texture) {
        return;
    }

    for (auto& material : m_materials) {
        if (material && !material->HasTexture()) {
            material->SetDiffuseTexture(texture);
        }
    }
}

void Model::ForEachMaterial(const std::function<void(Material&)>& callback)
{
    if (!callback) {
        return;
    }

    for (auto& material : m_materials) {
        if (material) {
            callback(*material);
        }
    }
}

Texture* Model::LoadTextureFromPath(const std::string& filepath)
{
    auto texture = std::make_unique<Texture>();
    if (texture->LoadFromFile(filepath)) {
        Texture* texturePtr = texture.get();
        m_ownedTextures.emplace_back(std::move(texture));
        return texturePtr;
    }
    return nullptr;
}

Texture* Model::LoadEmbeddedTexture(const aiScene* scene, const std::string& identifier)
{
    if (!scene || identifier.empty() || identifier[0] != '*') {
        return nullptr;
    }

    const aiTexture* embedded = scene->GetEmbeddedTexture(identifier.c_str());
    if (!embedded) {
        return nullptr;
    }

    auto texture = std::make_unique<Texture>();
    bool loaded = false;
    if (embedded->mHeight == 0) {
        const unsigned char* data = reinterpret_cast<const unsigned char*>(embedded->pcData);
        const std::size_t size = static_cast<std::size_t>(embedded->mWidth);
        loaded = texture->LoadFromMemory(data, size);
    } else {
        const std::size_t pixelCount = static_cast<std::size_t>(embedded->mWidth) * static_cast<std::size_t>(embedded->mHeight);
        if (pixelCount == 0) {
            return nullptr;
        }
        std::vector<unsigned char> pixels(pixelCount * 4);
        for (std::size_t i = 0; i < pixelCount; ++i) {
            const aiTexel& texel = embedded->pcData[i];
            pixels[i * 4 + 0] = texel.r;
            pixels[i * 4 + 1] = texel.g;
            pixels[i * 4 + 2] = texel.b;
            pixels[i * 4 + 3] = texel.a;
        }
        loaded = texture->LoadFromRawData(pixels.data(), embedded->mWidth, embedded->mHeight, 4);
    }

    if (!loaded) {
        return nullptr;
    }

    Texture* texturePtr = texture.get();
    m_ownedTextures.emplace_back(std::move(texture));
    return texturePtr;
}

glm::mat4 Model::ConvertMatrix(const aiMatrix4x4& matrix)
{
    return glm::mat4(
        matrix.a1, matrix.b1, matrix.c1, matrix.d1,
        matrix.a2, matrix.b2, matrix.c2, matrix.d2,
        matrix.a3, matrix.b3, matrix.c3, matrix.d3,
        matrix.a4, matrix.b4, matrix.c4, matrix.d4
    );
}

void Model::ResetBounds()
{
    m_aabbMin = glm::vec3(std::numeric_limits<float>::max());
    m_aabbMax = glm::vec3(std::numeric_limits<float>::lowest());
    m_boundingCenter = glm::vec3(0.0f);
    m_boundingRadius = 0.0f;
    m_hasBounds = false;
}

void Model::UpdateBounds(const glm::vec3& position)
{
    if (!m_hasBounds) {
        m_aabbMin = position;
        m_aabbMax = position;
        m_hasBounds = true;
        return;
    }

    m_aabbMin = glm::min(m_aabbMin, position);
    m_aabbMax = glm::max(m_aabbMax, position);
}

bool Model::ShouldIncludeNode(const std::string& nodeName) const
{
    if (!m_useNodeFilter) {
        return true;
    }
    const std::string normalized = NormalizeIdentifier(nodeName);
    if (normalized.empty()) {
        return false;
    }
    return m_allowedNames.find(normalized) != m_allowedNames.end();
}

std::string Model::NormalizeIdentifier(const std::string& name)
{
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return normalized;
}


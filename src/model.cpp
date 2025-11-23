#include "model.h"

#include <cstddef>
#include <iostream>
#include <vector>

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

bool Model::LoadFromFile(const std::string& filePath)
{
    m_meshes.clear();
    m_materials.clear();
    m_ownedTextures.clear();
    m_directory.clear();

    const size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        m_directory = filePath.substr(0, lastSlash);
    }

    const aiScene* scene = m_importer.ReadFile(
        filePath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_OptimizeMeshes |
        aiProcess_OptimizeGraph
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Erro ao carregar modelo (" << filePath << "): "
                  << m_importer.GetErrorString() << std::endl;
        return false;
    }

    ProcessNode(scene->mRootNode, scene);
    return true;
}

void Model::Draw(GLuint program, GLuint fallbackTextureID) const
{
    for (const auto& mesh : m_meshes) {
        mesh->Draw(program, fallbackTextureID);
    }
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.emplace_back(ProcessMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene);
    }
}

std::unique_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;
        vertex.position = glm::vec3(mesh->mVertices[i].x,
                                    mesh->mVertices[i].y,
                                    mesh->mVertices[i].z);

        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x,
                                      mesh->mNormals[i].y,
                                      mesh->mNormals[i].z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                         mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = glm::vec2(0.0f);
        }

        vertices.push_back(vertex);
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
        auto createdMaterial = CreateMaterial(material);
        meshMaterial = createdMaterial.get();
        m_materials.emplace_back(std::move(createdMaterial));
    } else {
        auto defaultMaterial = std::make_unique<Material>();
        meshMaterial = defaultMaterial.get();
        m_materials.emplace_back(std::move(defaultMaterial));
    }

    return std::make_unique<Mesh>(std::move(vertices), std::move(indices), meshMaterial);
}

std::unique_ptr<Material> Model::CreateMaterial(aiMaterial* sourceMaterial)
{
    if (!sourceMaterial) {
        return std::make_unique<Material>();
    }

    auto toVec3 = [](const aiColor3D& color) {
        return glm::vec3(color.r, color.g, color.b);
    };

    glm::vec3 ambient(0.2f);
    glm::vec3 diffuse(1.0f);
    glm::vec3 specular(1.0f);
    aiColor3D tempColor(0.0f, 0.0f, 0.0f);

    if (sourceMaterial->Get(AI_MATKEY_COLOR_AMBIENT, tempColor) == AI_SUCCESS) {
        ambient = toVec3(tempColor);
    }
    if (sourceMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, tempColor) == AI_SUCCESS) {
        diffuse = toVec3(tempColor);
    }
    if (sourceMaterial->Get(AI_MATKEY_COLOR_SPECULAR, tempColor) == AI_SUCCESS) {
        specular = toVec3(tempColor);
    }

    float shininess = 32.0f;
    if (sourceMaterial->Get(AI_MATKEY_SHININESS, shininess) != AI_SUCCESS || shininess <= 0.0f) {
        shininess = 32.0f;
    }

    auto material = std::make_unique<Material>(ambient, diffuse, specular, shininess);
    if (Texture* diffuseTexture = LoadMaterialTexture(sourceMaterial, aiTextureType_DIFFUSE)) {
        material->SetDiffuseTexture(diffuseTexture);
    }

    return material;
}

Texture* Model::LoadMaterialTexture(aiMaterial* material, aiTextureType type)
{
    if (material->GetTextureCount(type) == 0) {
        return nullptr;
    }

    aiString texturePath;
    if (material->GetTexture(type, 0, &texturePath) != AI_SUCCESS) {
        return nullptr;
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


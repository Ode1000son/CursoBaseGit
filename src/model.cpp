#include "model.h"

#include <cstddef>
#include <iostream>
#include <vector>

Mesh::Mesh(std::vector<Vertex>&& vertices,
           std::vector<unsigned int>&& indices,
           Texture* diffuseTexture)
    : m_vertices(std::move(vertices))
    , m_indices(std::move(indices))
    , m_diffuseTexture(diffuseTexture)
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

void Mesh::Draw(GLuint fallbackTextureID) const
{
    if (m_diffuseTexture) {
        m_diffuseTexture->Bind(GL_TEXTURE0);
    } else if (fallbackTextureID != 0) {
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
    m_ownedTextures.clear();
    m_directory.clear();

    const size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        m_directory = filePath.substr(0, lastSlash);
    }

    // Carrega o arquivo usando Assimp com flags de processamento específicas
    const aiScene* scene = m_importer.ReadFile(
        filePath,
        aiProcess_Triangulate |      // Converte todas as faces para triângulos
        aiProcess_GenSmoothNormals |  // Gera normais suaves se não existirem
        aiProcess_OptimizeMeshes |    // Junta meshes idênticos para otimização
        aiProcess_OptimizeGraph       // Otimiza a hierarquia de nós da cena
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Erro ao carregar modelo (" << filePath << "): "
                  << m_importer.GetErrorString() << std::endl;
        return false;
    }

    ProcessNode(scene->mRootNode, scene);
    return true;
}

void Model::Draw(GLuint fallbackTextureID) const
{
    for (const auto& mesh : m_meshes) {
        mesh->Draw(fallbackTextureID);
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

    // Processa cada vértice do mesh Assimp
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex vertex;

        // Copia posição do vértice
        vertex.position = glm::vec3(mesh->mVertices[i].x,
                                    mesh->mVertices[i].y,
                                    mesh->mVertices[i].z);

        // Copia normal se existir, senão usa um vetor padrão apontando para cima
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x,
                                      mesh->mNormals[i].y,
                                      mesh->mNormals[i].z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Normal padrão (para cima)
        }

        // Copia coordenadas UV do primeiro conjunto (canal 0) se existir
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                         mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = glm::vec2(0.0f); // UV padrão (0,0)
        }

        vertices.push_back(vertex);
    }

    // Processa os índices das faces para renderização indexed
    // Como usamos aiProcess_Triangulate, todas as faces são triângulos
    std::vector<unsigned int> indices;
    indices.reserve(mesh->mNumFaces * 3); // 3 índices por triângulo
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        // Copia todos os índices da face (normalmente 3 para triângulos)
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Tenta carregar a textura difusa do material do mesh
    Texture* diffuseTexture = nullptr;
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        diffuseTexture = LoadMaterialTexture(material, aiTextureType_DIFFUSE);
    }

    return std::make_unique<Mesh>(std::move(vertices), std::move(indices), diffuseTexture);
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

    // Extrai apenas o nome do arquivo da textura (remove caminho completo)
    std::string filename = texturePath.C_Str();
    if (filename.empty()) {
        return nullptr;
    }

    // Remove o caminho do arquivo, mantendo apenas o nome
    const size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // Tenta carregar a textura de múltiplos locais possíveis
    // Estratégia: 1) Mesmo diretório do modelo, 2) pasta assets, 3) pasta assets/textures
    std::vector<std::string> candidates;
    if (!m_directory.empty()) {
        candidates.push_back(m_directory + "/" + filename);  // Mesmo diretório do modelo
    }
    candidates.push_back("assets/" + filename);             // Pasta assets do projeto
    candidates.push_back("assets/textures/" + filename);    // Pasta assets/textures

    // Tenta carregar de cada local candidato
    for (const auto& path : candidates) {
        if (Texture* texture = LoadTextureFromPath(path)) {
            return texture;
        }
    }

    return nullptr;
}

/// @brief Carrega uma textura de arquivo e a armazena no modelo
/// @param filepath Caminho completo para o arquivo de textura
/// @return Ponteiro para a textura carregada ou nullptr se falhar
Texture* Model::LoadTextureFromPath(const std::string& filepath)
{
    // Cria uma nova textura e tenta carregá-la
    auto texture = std::make_unique<Texture>();
    if (texture->LoadFromFile(filepath)) {
        // Se carregou com sucesso, armazena no vetor de texturas owned
        // O modelo assume ownership da textura
        Texture* texturePtr = texture.get();
        m_ownedTextures.emplace_back(std::move(texture));
        return texturePtr;
    }
    return nullptr;
}


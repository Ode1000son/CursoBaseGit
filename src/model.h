#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <memory>
#include <string>

#include "texture.h"

struct Vertex
{
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
    glm::vec2 texCoords{ 0.0f };
};

class Mesh
{
public:
    Mesh(std::vector<Vertex>&& vertices,
         std::vector<unsigned int>&& indices,
         Texture* diffuseTexture);
    ~Mesh();

    void Draw(GLuint fallbackTextureID) const;

private:
    void SetupMesh();

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    Texture* m_diffuseTexture; // Não possui o recurso

    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
};

class Model
{
public:
    bool LoadFromFile(const std::string& filePath);
    void Draw(GLuint fallbackTextureID) const;
    bool HasMeshes() const { return !m_meshes.empty(); }

private:
    void ProcessNode(aiNode* node, const aiScene* scene);
    std::unique_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);
    Texture* LoadMaterialTexture(aiMaterial* material, aiTextureType type);
    Texture* LoadTextureFromPath(const std::string& filepath);

    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::vector<std::unique_ptr<Texture>> m_ownedTextures;
    std::string m_directory;
    Assimp::Importer m_importer;
};


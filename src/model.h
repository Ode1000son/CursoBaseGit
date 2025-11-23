#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <memory>
#include <string>
#include <functional>

#include "texture.h"
#include "material.h"

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
         Material* material);
    ~Mesh();

    void Draw(GLuint program, GLuint fallbackTextureID) const;

private:
    void SetupMesh();

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    Material* m_material;

    GLuint m_VAO;
    GLuint m_VBO;
    GLuint m_EBO;
};

class Model
{
public:
    bool LoadFromFile(const std::string& filePath);
    void Draw(GLuint program, GLuint fallbackTextureID) const;
    bool HasMeshes() const { return !m_meshes.empty(); }
    void OverrideAllTextures(Texture* texture);
    void ClearTextureOverrides();
    void ApplyTextureIfMissing(Texture* texture);
    void ForEachMaterial(const std::function<void(Material&)>& callback);

private:
    void ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    std::unique_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
    std::unique_ptr<Material> CreateMaterial(aiMaterial* sourceMaterial, const aiScene* scene);
    Texture* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const aiScene* scene);
    Texture* LoadEmbeddedTexture(const aiScene* scene, const std::string& identifier);
    Texture* LoadTextureFromPath(const std::string& filepath);
    static glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix);

    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::vector<std::unique_ptr<Material>> m_materials;
    std::vector<std::unique_ptr<Texture>> m_ownedTextures;
    std::string m_directory;
    Assimp::Importer m_importer;
};


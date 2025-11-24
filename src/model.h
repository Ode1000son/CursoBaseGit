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
#include <unordered_set>

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
    void DrawInstanced(GLuint program, GLuint fallbackTextureID, GLuint instanceVBO, GLsizei instanceCount) const;

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
    bool LoadFromFile(const std::string& filePath, const std::vector<std::string>& allowedNodes);
    void Draw(GLuint program, GLuint fallbackTextureID) const;
    void DrawInstanced(GLuint program, GLuint fallbackTextureID, GLuint instanceVBO, GLsizei instanceCount) const;
    bool HasMeshes() const { return !m_meshes.empty(); }
    void OverrideAllTextures(Texture* texture);
    void ClearTextureOverrides();
    void ApplyTextureIfMissing(Texture* texture);
    void ForEachMaterial(const std::function<void(Material&)>& callback);
    glm::vec3 GetBoundingCenter() const { return m_boundingCenter; }
    float GetBoundingRadius() const { return m_boundingRadius; }
    bool HasBounds() const { return m_hasBounds; }
    glm::vec3 GetBoundingHalfExtents() const;
    bool LoadFromScene(const aiScene* scene, const std::string& directory, const std::vector<std::string>& allowedNodes);

private:
    void ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform, bool parentIncluded);
    std::unique_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
    std::unique_ptr<Material> CreateMaterial(aiMaterial* sourceMaterial, const aiScene* scene);
    Texture* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const aiScene* scene);
    Texture* LoadEmbeddedTexture(const aiScene* scene, const std::string& identifier);
    Texture* LoadTextureFromPath(const std::string& filepath);
    static glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix);
    void ResetBounds();
    void UpdateBounds(const glm::vec3& position);
    bool ShouldIncludeNode(const std::string& nodeName) const;
    static std::string NormalizeIdentifier(const std::string& name);

    std::vector<std::unique_ptr<Mesh>> m_meshes;
    std::vector<std::unique_ptr<Material>> m_materials;
    std::vector<std::unique_ptr<Texture>> m_ownedTextures;
    std::string m_directory;
    Assimp::Importer m_importer;
    glm::vec3 m_aabbMin{ 0.0f };
    glm::vec3 m_aabbMax{ 0.0f };
    glm::vec3 m_boundingCenter{ 0.0f };
    float m_boundingRadius = 0.0f;
    bool m_hasBounds = false;
    bool m_useNodeFilter = false;
    std::unordered_set<std::string> m_allowedNames;
};


#pragma once

// Sistema de carregamento e renderização de modelos 3D usando Assimp
// Suporta múltiplas malhas, materiais, texturas embutidas e filtragem de nós

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

// Estrutura de vértice com posição, normal e coordenadas de textura
struct Vertex
{
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
    glm::vec2 texCoords{ 0.0f };
};

// Malha OpenGL com VAO/VBO/EBO e material associado
class Mesh
{
public:
    Mesh(std::vector<Vertex>&& vertices,
         std::vector<unsigned int>&& indices,
         Material* material);
    ~Mesh();

    // Renderiza a malha com shader e textura fallback
    void Draw(GLuint program, GLuint fallbackTextureID) const;
    // Renderiza múltiplas instâncias usando instancing
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

// Modelo 3D composto por múltiplas malhas e materiais
// Carrega arquivos via Assimp e gerencia texturas e bounds
class Model
{
public:
    // Carrega modelo completo de arquivo (GLTF, OBJ, FBX, etc)
    bool LoadFromFile(const std::string& filePath);
    // Carrega modelo filtrando apenas nós permitidos por nome
    bool LoadFromFile(const std::string& filePath, const std::vector<std::string>& allowedNodes);
    // Renderiza todas as malhas do modelo
    void Draw(GLuint program, GLuint fallbackTextureID) const;
    // Renderiza modelo com instancing
    void DrawInstanced(GLuint program, GLuint fallbackTextureID, GLuint instanceVBO, GLsizei instanceCount) const;
    bool HasMeshes() const { return !m_meshes.empty(); }
    // Substitui todas as texturas do modelo por uma única textura
    void OverrideAllTextures(Texture* texture);
    // Remove override de texturas
    void ClearTextureOverrides();
    // Aplica textura apenas se o material não tiver textura
    void ApplyTextureIfMissing(Texture* texture);
    // Itera sobre todos os materiais do modelo
    void ForEachMaterial(const std::function<void(Material&)>& callback);
    glm::vec3 GetBoundingCenter() const { return m_boundingCenter; }
    float GetBoundingRadius() const { return m_boundingRadius; }
    bool HasBounds() const { return m_hasBounds; }
    // Carrega modelo a partir de cena Assimp já carregada
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


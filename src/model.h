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

/// @brief Estrutura que representa um vértice de um mesh 3D
/// Contém todos os atributos necessários para renderização com iluminação e texturização
struct Vertex
{
    glm::vec3 position{ 0.0f };   ///< Posição 3D do vértice no espaço local do modelo
    glm::vec3 normal{ 0.0f };     ///< Vetor normal usado para cálculos de iluminação
    glm::vec2 texCoords{ 0.0f };  ///< Coordenadas UV para mapeamento de textura (0-1)
};

/// @brief Classe que representa um mesh individual carregado do modelo
/// Encapsula VAO, VBO, EBO e dados de geometria para renderização eficiente
class Mesh
{
public:
    /// @brief Construtor que inicializa o mesh com dados de geometria
    /// @param vertices Dados dos vértices (posição, normal, UV)
    /// @param indices Índices para renderização indexed (triângulos)
    /// @param diffuseTexture Textura difusa do material (pode ser nullptr)
    Mesh(std::vector<Vertex>&& vertices,
         std::vector<unsigned int>&& indices,
         Texture* diffuseTexture);

    /// @brief Destrutor que libera recursos OpenGL (VAO, VBO, EBO)
    ~Mesh();

    /// @brief Renderiza o mesh usando OpenGL
    /// @param fallbackTextureID ID da textura padrão se não houver textura própria
    void Draw(GLuint fallbackTextureID) const;

private:
    /// @brief Configura VAO, VBO e EBO com os dados de geometria
    void SetupMesh();

    std::vector<Vertex> m_vertices;          ///< Dados dos vértices do mesh
    std::vector<unsigned int> m_indices;     ///< Índices para renderização indexed
    Texture* m_diffuseTexture;               ///< Ponteiro para textura difusa (não possui o recurso)

    GLuint m_VAO;                            ///< Vertex Array Object - organiza atributos de vértice
    GLuint m_VBO;                            ///< Vertex Buffer Object - armazena dados de vértice na GPU
    GLuint m_EBO;                            ///< Element Buffer Object - armazena índices na GPU
};

/// @brief Classe que representa um modelo 3D carregado via Assimp
/// Gerencia múltiplos meshes, materiais e texturas de um arquivo de modelo.
/// Suporta carregamento de formatos como OBJ, FBX, glTF, etc.
class Model
{
public:
    /// @brief Carrega um modelo 3D de arquivo usando Assimp
    /// @param filePath Caminho para o arquivo de modelo (ex: .gltf, .obj, .fbx)
    /// @return true se carregou com sucesso, false caso contrário
    bool LoadFromFile(const std::string& filePath);

    /// @brief Renderiza todos os meshes do modelo
    /// @param fallbackTextureID ID da textura padrão para meshes sem textura própria
    void Draw(GLuint fallbackTextureID) const;

    /// @brief Verifica se o modelo contém meshes válidos
    /// @return true se há pelo menos um mesh carregado
    bool HasMeshes() const { return !m_meshes.empty(); }

private:
    /// @brief Processa recursivamente um nó da cena Assimp
    /// @param node Nó da hierarquia da cena
    /// @param scene Ponteiro para a cena Assimp completa
    void ProcessNode(aiNode* node, const aiScene* scene);

    /// @brief Converte um aiMesh do Assimp em um objeto Mesh próprio
    /// @param mesh Dados do mesh do Assimp
    /// @param scene Cena Assimp para acesso a materiais
    /// @return Ponteiro único para o mesh criado
    std::unique_ptr<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene);

    /// @brief Carrega textura de um material Assimp
    /// @param material Material do Assimp
    /// @param type Tipo de textura (diffuse, specular, etc.)
    /// @return Ponteiro para textura carregada ou nullptr se falhar
    Texture* LoadMaterialTexture(aiMaterial* material, aiTextureType type);

    /// @brief Carrega uma textura de arquivo e armazena no modelo
    /// @param filepath Caminho para o arquivo de textura
    /// @return Ponteiro para textura carregada ou nullptr se falhar
    Texture* LoadTextureFromPath(const std::string& filepath);

    std::vector<std::unique_ptr<Mesh>> m_meshes;         ///< Coleção de meshes do modelo
    std::vector<std::unique_ptr<Texture>> m_ownedTextures; ///< Texturas carregadas (owned pelo modelo)
    std::string m_directory;                              ///< Diretório base do arquivo do modelo
    Assimp::Importer m_importer;                          ///< Importador Assimp para carregamento
};


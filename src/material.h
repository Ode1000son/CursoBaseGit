#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "texture.h"

/// @brief Representa um material Phong com texturas opcionais.
class Material
{
public:
    Material() = default;
    Material(const glm::vec3& ambient,
             const glm::vec3& diffuse,
             const glm::vec3& specular,
             float shininess,
             Texture* diffuseTexture = nullptr);

    void SetAmbient(const glm::vec3& value) { m_ambient = value; }
    void SetDiffuse(const glm::vec3& value) { m_diffuse = value; }
    void SetSpecular(const glm::vec3& value) { m_specular = value; }
    void SetShininess(float value) { m_shininess = value; }

    void SetDiffuseTexture(Texture* texture) { m_diffuseTexture = texture; }
    void SetDiffuseOverride(Texture* texture) { m_overrideTexture = texture; }
    void ClearDiffuseOverride() { m_overrideTexture = nullptr; }
    Texture* GetActiveTexture() const { return m_overrideTexture ? m_overrideTexture : m_diffuseTexture; }
    bool HasTexture() const { return GetActiveTexture() != nullptr; }

    /// @brief Aplica os uniforms do material ao shader ativo.
    void Apply(GLuint program) const;

    /// @brief Vincula a textura difusa (se existir) na unidade indicada.
    void BindTexture(GLenum textureUnit = GL_TEXTURE0) const;

private:
    glm::vec3 m_ambient{ 0.2f };
    glm::vec3 m_diffuse{ 1.0f };
    glm::vec3 m_specular{ 1.0f };
    float m_shininess{ 32.0f };
    Texture* m_diffuseTexture{ nullptr }; // não possui a textura, apenas referência
    Texture* m_overrideTexture{ nullptr };
};


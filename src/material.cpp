#include "material.h"

#include <glm/gtc/type_ptr.hpp>

Material::Material(const glm::vec3& ambient,
                   const glm::vec3& diffuse,
                   const glm::vec3& specular,
                   float shininess,
                   Texture* diffuseTexture)
    : m_ambient(ambient)
    , m_diffuse(diffuse)
    , m_specular(specular)
    , m_shininess(shininess)
    , m_diffuseTexture(diffuseTexture)
{}

void Material::Apply(GLuint program) const
{
    const GLint ambientLoc = glGetUniformLocation(program, "material.ambient");
    const GLint diffuseLoc = glGetUniformLocation(program, "material.diffuse");
    const GLint specularLoc = glGetUniformLocation(program, "material.specular");
    const GLint shininessLoc = glGetUniformLocation(program, "material.shininess");

    glUniform3fv(ambientLoc, 1, glm::value_ptr(m_ambient));
    glUniform3fv(diffuseLoc, 1, glm::value_ptr(m_diffuse));
    glUniform3fv(specularLoc, 1, glm::value_ptr(m_specular));
    glUniform1f(shininessLoc, m_shininess);
}

void Material::BindTexture(GLenum textureUnit) const
{
    if (m_diffuseTexture) {
        m_diffuseTexture->Bind(textureUnit);
    } else {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}


#include "light_manager.h"

#include <glm/gtc/type_ptr.hpp>

glm::vec3 DirectionalLight::GetDirection(float time) const
{
    if (!animated) {
        return glm::normalize(direction);
    }

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), time * animationSpeed, animationAxis);
    glm::vec3 rotated = glm::vec3(rotation * glm::vec4(direction, 0.0f));
    return glm::normalize(rotated);
}

DirectionalLightManager::DirectionalLightManager(int maxLights)
    : m_maxLights(maxLights)
{}

void DirectionalLightManager::AddLight(const DirectionalLight& light)
{
    if (static_cast<int>(m_lights.size()) < m_maxLights) {
        m_lights.push_back(light);
    }
}

void DirectionalLightManager::Upload(GLuint program, float time) const
{
    const GLint directionalCountLoc = glGetUniformLocation(program, "directionalCount");
    const int count = static_cast<int>(std::min(m_lights.size(), static_cast<size_t>(m_maxLights)));
    glUniform1i(directionalCountLoc, count);

    for (int i = 0; i < count; ++i) {
        const DirectionalLight& light = m_lights[i];
        const glm::vec3 direction = light.GetDirection(time);

        const std::string base = "dirLights[" + std::to_string(i) + "]";
        glUniform3fv(glGetUniformLocation(program, (base + ".direction").c_str()), 1, glm::value_ptr(direction));
        glUniform3fv(glGetUniformLocation(program, (base + ".ambient").c_str()), 1, glm::value_ptr(light.ambient));
        glUniform3fv(glGetUniformLocation(program, (base + ".diffuse").c_str()), 1, glm::value_ptr(light.diffuse));
        glUniform3fv(glGetUniformLocation(program, (base + ".specular").c_str()), 1, glm::value_ptr(light.specular));
    }
}


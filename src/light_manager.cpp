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

PointLightManager::PointLightManager(int maxLights)
    : m_maxLights(maxLights)
{}

void PointLightManager::AddLight(const PointLight& light)
{
    if (static_cast<int>(m_lights.size()) < m_maxLights) {
        m_lights.push_back(light);
    }
}

void PointLightManager::Upload(GLuint program) const
{
    const GLint pointCountLoc = glGetUniformLocation(program, "pointCount");
    const int count = static_cast<int>(std::min(m_lights.size(), static_cast<size_t>(m_maxLights)));
    glUniform1i(pointCountLoc, count);

    for (int i = 0; i < count; ++i) {
        const PointLight& light = m_lights[i];
        const std::string base = "pointLights[" + std::to_string(i) + "]";

        glUniform3fv(glGetUniformLocation(program, (base + ".position").c_str()), 1, glm::value_ptr(light.position));
        glUniform3fv(glGetUniformLocation(program, (base + ".ambient").c_str()), 1, glm::value_ptr(light.ambient));
        glUniform3fv(glGetUniformLocation(program, (base + ".diffuse").c_str()), 1, glm::value_ptr(light.diffuse));
        glUniform3fv(glGetUniformLocation(program, (base + ".specular").c_str()), 1, glm::value_ptr(light.specular));
        glUniform1f(glGetUniformLocation(program, (base + ".constant").c_str()), light.constant);
        glUniform1f(glGetUniformLocation(program, (base + ".linear").c_str()), light.linear);
        glUniform1f(glGetUniformLocation(program, (base + ".quadratic").c_str()), light.quadratic);
        glUniform1f(glGetUniformLocation(program, (base + ".range").c_str()), light.range);
    }
}


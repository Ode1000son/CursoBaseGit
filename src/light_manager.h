#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include <string>

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    bool animated = false;
    glm::vec3 animationAxis{ 0.0f, 1.0f, 0.0f };
    float animationSpeed = 0.0f;

    glm::vec3 GetDirection(float time) const;
};

struct PointLight
{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float range = 10.0f;
};

class DirectionalLightManager
{
public:
    explicit DirectionalLightManager(int maxLights);

    void AddLight(const DirectionalLight& light);
    void Upload(GLuint program, float time) const;

private:
    int m_maxLights;
    std::vector<DirectionalLight> m_lights;
};

class PointLightManager
{
public:
    explicit PointLightManager(int maxLights);

    void AddLight(const PointLight& light);
    void Upload(GLuint program) const;

private:
    int m_maxLights;
    std::vector<PointLight> m_lights;
};


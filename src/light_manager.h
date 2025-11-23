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


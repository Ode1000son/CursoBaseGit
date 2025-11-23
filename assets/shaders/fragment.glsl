#version 330 core

const int MAX_DIRECTIONAL_LIGHTS = 4;

// Dados recebidos do vertex shader
in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform int directionalCount;
uniform DirectionalLight dirLights[MAX_DIRECTIONAL_LIGHTS];
uniform Material material;
uniform vec3 viewPos;
uniform sampler2D textureSampler;

out vec4 FragColor;

vec3 CalculateDirectionalLight(DirectionalLight light, vec3 norm, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = light.ambient * (material.ambient * albedo);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * (material.diffuse * albedo);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 albedo = texture(textureSampler, texCoord).rgb;

    vec3 accumulated = vec3(0.0f);
    for (int i = 0; i < directionalCount; ++i) {
        accumulated += CalculateDirectionalLight(dirLights[i], norm, viewDir, albedo);
    }

    FragColor = vec4(accumulated, 1.0f);
}

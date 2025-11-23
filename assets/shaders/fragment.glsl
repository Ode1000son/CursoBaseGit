#version 330 core

// Dados recebidos do vertex shader
in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 specular;
    float shininess;
};

uniform Light light;
uniform Material material;
uniform vec3 viewPos;
uniform sampler2D textureSampler;

out vec4 FragColor;

void main()
{
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(-light.direction);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 diffuseMap = texture(textureSampler, texCoord).rgb;

    vec3 ambient = light.ambient * diffuseMap;
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * diffuseMap;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    FragColor = vec4(ambient + diffuse + specular, 1.0f);
}

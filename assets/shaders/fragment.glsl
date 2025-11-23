#version 330 core

const int MAX_DIRECTIONAL_LIGHTS = 4;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;
in vec4 fragPosLightSpace;

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
uniform sampler2D shadowMap;

out vec4 FragColor;

/**
 * @brief Calcula o fator de sombra usando Percentage Closer Filtering (PCF) com bias dinâmico
 *
 * Algoritmo: Transforma coordenadas do espaço da luz para coordenadas normalizadas [0,1],
 * aplica bias dinâmico baseado no ângulo entre normal e direção da luz, e usa PCF
 * 3x3 para suavizar artefatos de aliasing no shadow mapping.
 *
 * @param fragPosLS Posição do fragmento no espaço da luz (light space)
 * @param norm Normal do fragmento (normalizado)
 * @param lightDir Direção da luz para o fragmento (normalizado)
 * @return Fator de sombra [0.0 = totalmente iluminado, 1.0 = totalmente sombreado]
 */
float CalculateShadow(vec4 fragPosLS, vec3 norm, vec3 lightDir)
{
    // Transforma coordenadas do espaço da luz [-1,1] para coordenadas de textura [0,1]
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Verifica se o fragmento está dentro do frustum da luz (fora = sem sombra)
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // Bias dinâmico: maior quando superfície está quase paralela à luz (shadow acne)
    // Fórmula: bias = max(baseBias * (1.0 - cosθ), minBias) onde θ = ângulo normal-luz
    float bias = max(0.0005 * (1.0 - dot(norm, lightDir)), 0.00005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0); // Tamanho de um texel no shadow map

    // Percentage Closer Filtering (PCF): amostra múltiplos texels para suavizar bordas
    // Kernel 3x3 centrado na coordenada do fragmento
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            float currentDepth = projCoords.z - bias; // Profundidade atual com bias aplicado
            shadow += currentDepth > closestDepth ? 1.0 : 0.0; // 1.0 = sombreado, 0.0 = iluminado
        }
    }

    shadow /= 9.0; // Média das 9 amostras (3x3)
    return shadow;
}

vec3 EvaluateDirectionalLight(const DirectionalLight light, vec3 norm, vec3 viewDir, vec3 albedo, float shadowFactor)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = light.ambient * (material.ambient * albedo);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * (material.diffuse * albedo);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return ambient + (diffuse + specular) * shadowFactor;
}

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 albedo = texture(textureSampler, texCoord).rgb;

    vec3 result = vec3(0.0f);
    for (int i = 0; i < directionalCount; ++i) {
        vec3 lightDir = normalize(-dirLights[i].direction);
        float shadow = (i == 0) ? (1.0f - CalculateShadow(fragPosLightSpace, norm, lightDir)) : 1.0f;
        result += EvaluateDirectionalLight(dirLights[i], norm, viewDir, albedo, shadow);
    }

    FragColor = vec4(result, 1.0f);
}

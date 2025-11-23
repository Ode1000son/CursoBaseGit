#version 330 core

const int MAX_DIRECTIONAL_LIGHTS = 4;
const int MAX_POINT_LIGHTS = 4;

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

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float range;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform int directionalCount;
uniform int pointCount;
uniform DirectionalLight dirLights[MAX_DIRECTIONAL_LIGHTS];
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform Material material;
uniform vec3 viewPos;
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;
uniform samplerCube pointShadowMap;
uniform vec3 pointShadowLightPos;
uniform float shadowFarPlane;
uniform int shadowPointIndex;

out vec4 FragColor;

float CalculateDirectionalShadow(vec4 fragPosLS, vec3 norm, vec3 lightDir)
{
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias = max(0.0005 * (1.0 - dot(norm, lightDir)), 0.00005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            float currentDepth = projCoords.z - bias;
            shadow += currentDepth > closestDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

/**
 * @brief Calcula sombra omnidirecional usando Point Light Shadow Mapping com PCF 3D
 *
 * Algoritmo: Usa cubemap de profundidade para sombras 360°, calcula distância do fragmento
 * à luz, aplica PCF em 3D com offsets esfericamente distribuídos, e usa disk radius
 * adaptativo baseado na distância para reduzir shadow acne em superfícies distantes.
 *
 * Técnica: Omnidirectional Shadow Mapping com Percentage Closer Filtering volumétrico.
 * O cubemap armazena distâncias normalizadas [0,far_plane] em cada direção da luz.
 *
 * @param fragPosition Posição do fragmento no espaço do mundo
 * @return Fator de sombra [0.0 = totalmente iluminado, 1.0 = totalmente sombreado]
 */
float CalculatePointShadow(vec3 fragPosition)
{
    // Vetor do fragmento para a posição da luz (direção da amostragem no cubemap)
    vec3 fragToLight = fragPosition - pointShadowLightPos;
    float currentDepth = length(fragToLight); // Distância atual do fragmento à luz

    // Bias fixo para reduzir shadow acne (menor que no directional por ser menos problemático)
    float bias = 0.15;
    int samples = 20; // Número de amostras para PCF 3D
    float shadow = 0.0;

    // Distribuição esférica otimizada de offsets para PCF 3D
    // Inclui: 8 vértices do cubo, 4 arestas do equador, 4 faces laterais, 4 diagonais verticais
    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), // Vértices superiores
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1), // Vértices inferiores
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0), // Arestas equatoriais
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1), // Arestas longitudinais
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)  // Arestas latitudinais
    );

    // Disk radius adaptativo: aumenta com a distância para compensar precisão reduzida
    // Fórmula: diskRadius = (1.0 + (distância / far_plane)) / 25.0
    // Resulta em raio maior para fragmentos distantes, reduzindo shadow acne
    float diskRadius = (1.0 + (currentDepth / shadowFarPlane)) / 25.0;

    // PCF 3D: amostra o cubemap em múltiplas direções ao redor da direção principal
    for (int i = 0; i < samples; ++i) {
        // Direção amostrada = direção principal + offset esfericamente distribuído
        float closestDepth = texture(pointShadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= shadowFarPlane; // Converte profundidade normalizada [0,1] para [0,far_plane]

        // Comparação: se distância atual (com bias) > distância armazenada = está em sombra
        if ((currentDepth - bias) > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples); // Média das amostras para suavização
    return shadow;
}

vec3 EvaluateDirectionalLight(const DirectionalLight light, vec3 norm, vec3 viewDir, vec3 albedo, float visibility)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = light.ambient * (material.ambient * albedo);
    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = light.diffuse * diff * (material.diffuse * albedo);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);
    vec3 specular = light.specular * spec * material.specular;

    return ambient + (diffuse + specular) * visibility;
}

vec3 EvaluatePointLight(const PointLight light, vec3 norm, vec3 viewDir, vec3 fragPosition, vec3 albedo)
{
    vec3 lightDir = light.position - fragPosition;
    float distance = length(lightDir);
    if (distance > light.range) {
        return vec3(0.0);
    }

    lightDir = normalize(lightDir);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0f);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), material.shininess);

    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float rangeFactor = clamp(1.0 - (distance / light.range), 0.0, 1.0);
    float intensity = attenuation * rangeFactor;

    vec3 ambient = light.ambient * (material.ambient * albedo);
    vec3 diffuse = light.diffuse * diff * (material.diffuse * albedo);
    vec3 specular = light.specular * spec * material.specular;

    return (ambient + diffuse + specular) * intensity;
}

void main()
{
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 albedo = texture(textureSampler, texCoord).rgb;

    vec3 result = vec3(0.0f);
    for (int i = 0; i < directionalCount; ++i) {
        vec3 lightDir = normalize(-dirLights[i].direction);
        float visibility = (i == 0) ? (1.0f - CalculateDirectionalShadow(fragPosLightSpace, norm, lightDir)) : 1.0f;
        result += EvaluateDirectionalLight(dirLights[i], norm, viewDir, albedo, visibility);
    }

    for (int i = 0; i < pointCount; ++i) {
        vec3 pointContribution = EvaluatePointLight(pointLights[i], norm, viewDir, fragPos, albedo);
        if (shadowPointIndex >= 0 && i == shadowPointIndex) {
            float shadow = CalculatePointShadow(fragPos);
            pointContribution *= (1.0 - shadow);
        }
        result += pointContribution;
    }

    FragColor = vec4(result, 1.0f);
}

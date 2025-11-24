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

layout (location = 0) out vec4 SceneColor;
layout (location = 1) out vec4 HighlightColor;

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

float CalculatePointShadow(vec3 fragPosition)
{
    vec3 fragToLight = fragPosition - pointShadowLightPos;
    float currentDepth = length(fragToLight);
    float bias = 0.15;
    int samples = 20;
    float shadow = 0.0;

    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );

    float diskRadius = (1.0 + (currentDepth / shadowFarPlane)) / 25.0;

    for (int i = 0; i < samples; ++i) {
        float closestDepth = texture(pointShadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= shadowFarPlane;
        if ((currentDepth - bias) > closestDepth) {
            shadow += 1.0;
        }
    }

    shadow /= float(samples);
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
    const float gamma = 3.2;
    vec3 norm = normalize(normal);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 albedo = texture(textureSampler, texCoord).rgb;
    albedo = pow(albedo, vec3(gamma));

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

    result = max(result, vec3(0.0));
    vec3 gammaCorrected = pow(result, vec3(1.0 / gamma));
    float brightness = dot(gammaCorrected, vec3(0.2126, 0.7152, 0.0722));
    vec3 highlight = brightness > 0.8 ? gammaCorrected : vec3(0.0);
    SceneColor = vec4(gammaCorrected, 1.0f);
    HighlightColor = vec4(highlight, 1.0f);
}

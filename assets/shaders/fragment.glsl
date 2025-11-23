#version 330 core

// Dados recebidos do vertex shader
in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

// Uniforms de iluminação
uniform vec3 lightDirection;
uniform vec3 viewPos;
uniform sampler2D textureSampler;

out vec4 FragColor;

void main()
{
    // Normaliza vetores para cálculos de iluminação
    vec3 norm = normalize(normal);                    // Normal da superfície
    vec3 lightDir = normalize(-lightDirection);       // Direção da luz (inverte pois vem como direção para a luz)
    vec3 viewDir = normalize(viewPos - fragPos);      // Direção da câmera para o fragmento
    vec3 halfway = normalize(lightDir + viewDir);     // Vetor halfway para Blinn-Phong specular

    // Calcula componentes de iluminação (Phong Reflection Model)
    float ambientStrength = 0.2f;                                     // Iluminação ambiente constante
    float diffuseStrength = max(dot(norm, lightDir), 0.0f);           // Iluminação difusa (Lambert)
    float specularStrength = pow(max(dot(norm, halfway), 0.0f), 32.0f); // Iluminação especular (Blinn-Phong)

    // Amostra cor da textura
    vec3 texColor = texture(textureSampler, texCoord).rgb;

    // Combina iluminação com cor da textura
    // Diffuse e ambient afetam a cor base, specular é adicionado separadamente
    vec3 lighting = texColor * (ambientStrength + diffuseStrength) + vec3(specularStrength * 0.2f);

    FragColor = vec4(lighting, 1.0f);
}

#version 330 core

/**
 * @brief Fragment Shader para Point Light Shadow Mapping - Armazenamento de distâncias
 *
 * Técnica: Depth-only rendering omnidirecional. Calcula e armazena a distância
 * do fragmento à luz pontual no cubemap, normalizada pelo far_plane.
 *
 * Funcionamento:
 * - Recebe posição do fragmento no espaço do mundo
 * - Calcula distância euclidiana até a posição da luz
 * - Normaliza pela distância máxima (far_plane) para otimizar precisão
 * - Escreve diretamente em gl_FragDepth (depth-only pass)
 *
 * Resultado: Cada texel do cubemap armazena a distância mais próxima da geometria
 * à luz nessa direção específica, permitindo comparação para detecção de sombra.
 */

in vec4 FragPos; // Posição do fragmento (saída do geometry shader)

uniform vec3 lightPos;   // Posição da luz pontual no espaço do mundo
uniform float far_plane; // Distância máxima do volume de sombra

void main()
{
    // Calcula distância euclidiana do fragmento à luz
    // FragPos.xyz está no espaço do mundo (transformado pelo geometry shader)
    float lightDistance = length(FragPos.xyz - lightPos);

    // Normaliza a distância pelo far_plane para obter valores [0,1]
    // Isso otimiza a precisão do depth buffer e permite comparação consistente
    lightDistance = lightDistance / far_plane;

    // Escreve diretamente na profundidade do fragmento
    // O hardware rasteriza apenas a profundidade mais próxima (depth test)
    gl_FragDepth = lightDistance;
}


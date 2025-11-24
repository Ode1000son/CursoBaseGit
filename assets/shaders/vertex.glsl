#version 330 core

// Vertex shader principal para renderização de cena
// Transforma vértices do espaço local para espaço de tela e espaço de luz
// Suporta instancing via matriz de instância e calcula normais transformadas

// Atributos do modelo importado
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceModel;  // Matriz de instância (instancing)

// Matrizes de transformação
uniform mat4 model;              // Matriz modelo (espaço local -> mundo)
uniform mat4 view;               // Matriz de visão (mundo -> câmera)
uniform mat4 projection;         // Matriz de projeção (câmera -> tela)
uniform mat4 lightSpaceMatrix;   // Matriz para shadow mapping direcional
uniform int uUseInstanceTransform = 0;  // Flag para usar instancing

// Dados enviados ao fragment shader
out vec3 fragPos;                // Posição do fragmento no espaço mundial
out vec3 normal;                  // Normal transformada
out vec2 texCoord;                // Coordenadas de textura
out vec4 fragPosLightSpace;      // Posição no espaço da luz (shadow mapping)

void main()
{
    mat4 finalModel = model;
    if (uUseInstanceTransform == 1)
    {
        finalModel = aInstanceModel;
    }

    vec4 worldPosition = finalModel * vec4(aPos, 1.0);
    fragPos = worldPosition.xyz;

    // Transformação correta das normais (sem translação)
    normal = normalize(mat3(transpose(inverse(finalModel))) * aNormal);

    texCoord = aTexCoord;
    fragPosLightSpace = lightSpaceMatrix * worldPosition;
    gl_Position = projection * view * worldPosition;
}

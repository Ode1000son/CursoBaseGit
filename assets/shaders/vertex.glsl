#version 330 core

// Atributos do modelo importado
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

// Matrizes de transformação
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Dados enviados ao fragment shader
out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

void main()
{
    // Calcula posição do vértice no espaço do mundo
    vec4 worldPosition = model * vec4(aPos, 1.0);
    fragPos = worldPosition.xyz;

    // Transformação correta das normais usando Normal Matrix
    // transpose(inverse(model)) remove translação e preserva ângulos
    // mat3() converte para 3x3 (normais não precisam de componente de translação)
    normal = mat3(transpose(inverse(model))) * aNormal;

    // Passa coordenadas UV diretamente (não precisam de transformação)
    texCoord = aTexCoord;

    // Aplica pipeline MVP completo: Projeção * Visão * Modelo
    gl_Position = projection * view * worldPosition;
}

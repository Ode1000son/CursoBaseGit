#version 330 core

// Atributo de entrada 0: posição do vértice (vec3)
layout (location = 0) in vec3 aPos;

// Atributo de entrada 1: cor do vértice (vec3)
layout (location = 1) in vec3 aColor;

// Atributo de entrada 2: coordenadas UV (vec2)
layout (location = 2) in vec2 aTexCoord;

// Uniforms das matrizes de transformação
uniform mat4 model;      // Matriz Model (objeto -> mundo)
uniform mat4 view;       // Matriz View (mundo -> câmera)
uniform mat4 projection; // Matriz Projection (câmera -> recorte)

// Saídas para o fragment shader
out vec3 ourColor;    // Cor interpolada
out vec2 texCoord;    // Coordenadas UV interpoladas

void main()
{
    // Aplicar as transformações MVP: P * V * M * posição
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Passa a cor e coordenadas UV para o fragment shader
    ourColor = aColor;
    texCoord = aTexCoord;
}

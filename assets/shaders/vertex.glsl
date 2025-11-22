#version 330 core

// Atributo de entrada 0: posição do vértice (vec3)
layout (location = 0) in vec3 aPos;

// Atributo de entrada 1: cor do vértice (vec3)
layout (location = 1) in vec3 aColor;

// Uniforms das matrizes de transformação
uniform mat4 model;      // Matriz Model (objeto -> mundo)
uniform mat4 view;       // Matriz View (mundo -> câmera)
uniform mat4 projection; // Matriz Projection (câmera -> recorte)

// Saída para o fragment shader: cor interpolada
out vec3 ourColor;

void main()
{
    // Aplicar as transformações MVP: P * V * M * posição
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Passa a cor do vértice para o fragment shader
    ourColor = aColor;
}

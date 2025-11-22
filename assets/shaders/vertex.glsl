#version 330 core

// Atributo de entrada 0: posição do vértice (vec3)
layout (location = 0) in vec3 aPos;

// Atributo de entrada 1: cor do vértice (vec3)
layout (location = 1) in vec3 aColor;

// Saída para o fragment shader: cor interpolada
out vec3 ourColor;

void main()
{
    // Define a posição do vértice no espaço de recorte
    // Como estamos trabalhando em 2D, z = 0.0 e w = 1.0
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);

    // Passa a cor do vértice para o fragment shader
    ourColor = aColor;
}

#version 330 core

// Vertex shader para post-processing (fullscreen quad)
// Renderiza um quad que cobre toda a tela para aplicar efeitos de pós-processamento
// Passa coordenadas de textura para o fragment shader

layout (location = 0) in vec2 aPos;      // Posição do vértice em espaço NDC (-1 a 1)
layout (location = 1) in vec2 aTexCoord; // Coordenadas de textura (0 a 1)

out vec2 TexCoord;  // Coordenadas de textura para o fragment shader

void main()
{
    TexCoord = aTexCoord;
    // Posição já está em espaço NDC, apenas passa adiante
    gl_Position = vec4(aPos, 0.0, 1.0);
}


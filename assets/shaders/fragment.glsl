#version 330 core

// Entradas do vertex shader
in vec3 ourColor;    // Cor interpolada
in vec2 texCoord;    // Coordenadas UV interpoladas

// Uniform da textura
uniform sampler2D textureSampler;  // Sampler da textura

// Saída final: cor do fragmento
out vec4 FragColor;

void main()
{
    // Amostra a cor da textura usando as coordenadas UV
    vec4 texColor = texture(textureSampler, texCoord);

    // Combina a cor da textura com a cor do vértice
    // Isso permite modular a textura com as cores dos vértices
    FragColor = texColor * vec4(ourColor, 1.0);
}

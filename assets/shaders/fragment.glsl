#version 330 core

// Entrada do vertex shader: cor interpolada
in vec3 ourColor;

// Saída final: cor do fragmento
out vec4 FragColor;

void main()
{
    // Define a cor final do fragmento
    // Alpha = 1.0 (opaco)
    FragColor = vec4(ourColor, 1.0);
}

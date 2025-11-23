#version 330 core

/**
 * @brief Geometry Shader para Point Light Shadow Mapping - Renderização omnidirecional
 *
 * Técnica: Layered Rendering para cubemap. Recebe um triângulo e o replica 6 vezes,
 * uma para cada face do cubemap, usando gl_Layer para especificar qual face renderizar.
 *
 * Processo:
 * 1. Para cada face do cubemap (0-5 = +X,-X,+Y,-Y,+Z,-Z)
 * 2. Define gl_Layer para selecionar a face alvo
 * 3. Transforma cada vértice do triângulo pela matriz de shadow daquela face
 * 4. Emite o triângulo transformado para a face correspondente
 *
 * Resultado: Um único draw call renderiza a geometria em todas as 6 faces simultaneamente,
 * criando o depth cubemap usado para sombras omnidirecionais.
 */

layout (triangles) in;                           // Entrada: triângulos da geometria
layout (triangle_strip, max_vertices = 18) out; // Saída: 6 triângulos (3 vértices × 6 faces)

uniform mat4 shadowMatrices[6]; // Matrizes view-projection para cada face do cubemap

out vec4 FragPos; // Posição do vértice passada ao fragment shader

void main()
{
    // Renderiza o triângulo em todas as 6 faces do cubemap
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // Seleciona qual face do cubemap (0-5) será renderizada

        // Processa os 3 vértices do triângulo atual
        for (int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;                    // Posição original do vértice
            gl_Position = shadowMatrices[face] * FragPos;      // Transforma pela matriz da face atual
            EmitVertex();                                      // Emite vértice para a face selecionada
        }
        EndPrimitive(); // Finaliza o triângulo na face atual
    }
}


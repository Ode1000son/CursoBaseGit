#version 330 core

// Geometry shader para shadow mapping de luz pontual
// Renderiza cada triângulo em todas as 6 faces do cubemap simultaneamente
// Usa gl_Layer para direcionar saída para cada face do cubemap

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;  // 3 vértices × 6 faces

uniform mat4 shadowMatrices[6];  // Matrizes de projeção para cada face do cubo (+X, -X, +Y, -Y, +Z, -Z)

out vec4 FragPos;  // Posição do fragmento (passada para fragment shader)

void main()
{
    // Para cada face do cubemap
    for (int face = 0; face < 6; ++face)
    {
        gl_Layer = face;  // Define qual face do cubemap receberá este triângulo
        // Emite os 3 vértices do triângulo transformados pela matriz desta face
        for (int i = 0; i < 3; ++i)
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}


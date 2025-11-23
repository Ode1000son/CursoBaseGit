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
    vec4 worldPosition = model * vec4(aPos, 1.0);
    fragPos = worldPosition.xyz;

    // Transformação correta das normais (sem translação)
    normal = normalize(mat3(transpose(inverse(model))) * aNormal);

    texCoord = aTexCoord;
    gl_Position = projection * view * worldPosition;
}

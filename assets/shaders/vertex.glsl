#version 330 core

// Atributos do modelo importado
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 aInstanceModel;

// Matrizes de transformação
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;
uniform int uUseInstanceTransform = 0;

// Dados enviados ao fragment shader
out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;
out vec4 fragPosLightSpace;

void main()
{
    mat4 finalModel = model;
    if (uUseInstanceTransform == 1)
    {
        finalModel = aInstanceModel;
    }

    vec4 worldPosition = finalModel * vec4(aPos, 1.0);
    fragPos = worldPosition.xyz;

    // Transformação correta das normais (sem translação)
    normal = normalize(mat3(transpose(inverse(finalModel))) * aNormal);

    texCoord = aTexCoord;
    fragPosLightSpace = lightSpaceMatrix * worldPosition;
    gl_Position = projection * view * worldPosition;
}

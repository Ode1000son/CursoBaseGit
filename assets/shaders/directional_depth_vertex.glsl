#version 330 core

// Vertex shader para shadow mapping direcional (directional light shadows)
// Transforma vértices para o espaço da luz direcional usando lightSpaceMatrix
// Suporta instancing para renderização eficiente

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceModel;  // Matriz de instância

uniform mat4 lightSpaceMatrix;  // Matriz de projeção ortográfica da luz direcional
uniform mat4 model;              // Matriz modelo padrão
uniform int uUseInstanceTransform = 0;  // Flag para usar instancing

void main()
{
    mat4 finalModel = (uUseInstanceTransform == 1) ? aInstanceModel : model;
    // Transforma para espaço da luz (projeção ortográfica)
    gl_Position = lightSpaceMatrix * finalModel * vec4(aPos, 1.0);
}


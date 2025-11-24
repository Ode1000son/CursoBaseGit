#version 330 core

// Vertex shader para shadow mapping de luz pontual (point light shadows)
// Renderiza geometria no espaço da luz pontual para gerar cubemap de profundidade
// Suporta instancing para renderização eficiente de múltiplas instâncias

layout (location = 0) in vec3 aPos;
layout (location = 3) in mat4 aInstanceModel;  // Matriz de instância

uniform mat4 model;                            // Matriz modelo padrão
uniform int uUseInstanceTransform = 0;          // Flag para usar instancing

void main()
{
    mat4 finalModel = (uUseInstanceTransform == 1) ? aInstanceModel : model;
    // Posição no espaço da luz (será transformada pelo geometry shader para cada face do cubo)
    gl_Position = finalModel * vec4(aPos, 1.0);
}


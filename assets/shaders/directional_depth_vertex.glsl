#version 330 core

/**
 * @brief Vertex Shader para Directional Shadow Mapping - Transformação para light space
 *
 * Função: Transforma vértices da geometria diretamente para o espaço da luz
 * (light space) combinando model matrix e light space matrix em um único passo.
 *
 * Técnica: Shadow mapping direcional clássico. A luz direcional tem uma única
 * matriz de view-projection que cobre toda a cena visível, diferente das
 * luzes pontuais que precisam de 6 matrizes (cubemap).
 */

layout (location = 0) in vec3 aPos; // Posição do vértice no espaço do modelo

uniform mat4 lightSpaceMatrix; // Matriz combinada: light_projection * light_view
uniform mat4 model;           // Matriz de transformação do modelo

void main()
{
    // Transforma vértice diretamente para o espaço da luz
    // Combina: model space -> world space -> light view space -> light clip space
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}


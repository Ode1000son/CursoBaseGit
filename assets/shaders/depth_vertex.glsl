#version 330 core

/**
 * @brief Vertex Shader para Point Light Shadow Mapping - Transformação básica
 *
 * Função: Parte inicial do pipeline de shadow mapping omnidirecional.
 * Transforma vértices da geometria para o espaço do mundo usando apenas
 * a matriz de modelo (sem view/projection, pois isso é feito no geometry shader).
 *
 * Contexto: Este shader trabalha em conjunto com o geometry shader para
 * renderizar a cena 6 vezes (uma por face do cubemap) em um único draw call.
 */

layout (location = 0) in vec3 aPos; // Posição do vértice no espaço do modelo

uniform mat4 model; // Matriz de transformação do modelo (model space -> world space)

void main()
{
    // Transforma vértice para espaço do mundo (world space)
    // Não aplica view/projection aqui - isso é feito no geometry shader
    // para cada uma das 6 faces do cubemap separadamente
    gl_Position = model * vec4(aPos, 1.0);
}


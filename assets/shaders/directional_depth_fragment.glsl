#version 330 core

/**
 * @brief Fragment Shader para Directional Shadow Mapping - Depth-only rendering
 *
 * Técnica: Depth-only pass. Não escreve cor, apenas profundidade.
 * O shader vazio força o hardware a executar apenas o depth test,
 * armazenando as profundidades mais próximas no shadow map 2D.
 *
 * Funcionamento:
 * - Nenhum código necessário - o hardware automaticamente escreve gl_FragCoord.z
 * - O fragment shader serve apenas para indicar que queremos depth-only rendering
 * - Resultado: Shadow map 2D com profundidades normalizadas [0,1] da cena
 *   vista da perspectiva da luz direcional
 */

void main()
{
    // Depth-only pass: não escreve cor, apenas profundidade
    // O hardware automaticamente armazena gl_FragCoord.z no depth attachment
}


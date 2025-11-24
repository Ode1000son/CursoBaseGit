#version 330 core

// Fragment shader para shadow mapping direcional
// Pass-through vazio: apenas o depth buffer é necessário
// A profundidade é escrita automaticamente pelo pipeline

void main()
{
    // Depth-only pass: não precisa calcular cor, apenas profundidade
}


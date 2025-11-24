#pragma once

// Controlador de atalhos de teclado para o renderer
// Processa teclas de atalho (1/2/3 para modos de textura, F1/F2 para overlay)

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "renderer.h"

// Gerencia atalhos de teclado relacionados ao renderer
class RendererController
{
public:
    void Initialize(Renderer* renderer);
    void ProcessShortcuts(GLFWwindow* window);

private:
    Renderer* m_renderer = nullptr;
    bool m_key1Held = false;
    bool m_key2Held = false;
    bool m_key3Held = false;
    bool m_f1Held = false;
    bool m_f2Held = false;
};


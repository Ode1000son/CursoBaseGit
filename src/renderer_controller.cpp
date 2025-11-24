// Implementação do controlador de atalhos do renderer
// Processa entrada de teclado para modos de textura e controles de overlay

#include "renderer_controller.h"

void RendererController::Initialize(Renderer* renderer)
{
    m_renderer = renderer;
    m_key1Held = false;
    m_key2Held = false;
    m_key3Held = false;
    m_f1Held = false;
    m_f2Held = false;
}

void RendererController::ProcessShortcuts(GLFWwindow* window)
{
    if (!m_renderer || !window)
    {
        return;
    }

    auto handleKey = [&](int key, bool& heldState, TextureOverrideMode mode) {
        bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;
        if (isPressed && !heldState && m_renderer->GetOverrideMode() != mode)
        {
            m_renderer->SetOverrideMode(mode);
        }
        heldState = isPressed;
    };

    handleKey(GLFW_KEY_1, m_key1Held, TextureOverrideMode::Imported);
    handleKey(GLFW_KEY_2, m_key2Held, TextureOverrideMode::Checker);
    handleKey(GLFW_KEY_3, m_key3Held, TextureOverrideMode::Highlight);

    auto handleToggle = [&](int key, bool& heldState, auto&& action) {
        bool pressed = glfwGetKey(window, key) == GLFW_PRESS;
        if (pressed && !heldState)
        {
            action();
        }
        heldState = pressed;
    };

    handleToggle(GLFW_KEY_F1, m_f1Held, [&]() {
        m_renderer->ToggleMetricsOverlay();
    });

    handleToggle(GLFW_KEY_F2, m_f2Held, [&]() {
        m_renderer->ClearDebugMessages();
    });
}


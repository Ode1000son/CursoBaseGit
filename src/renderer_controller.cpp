#include "renderer_controller.h"

void RendererController::Initialize(Renderer* renderer)
{
    m_renderer = renderer;
    m_key1Held = false;
    m_key2Held = false;
    m_key3Held = false;
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
}


#include "input_controller.h"

#include <iostream>

InputController* InputController::s_instance = nullptr;

InputController::InputController() = default;

void InputController::Initialize(Camera* camera)
{
    m_camera = camera;
}

void InputController::AttachWindow(GLFWwindow* window)
{
    m_window = window;
    s_instance = this;

    if (m_window)
    {
        glfwSetCursorPosCallback(m_window, MouseCallback);
        glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void InputController::ProcessInput(float deltaTime)
{
    if (!m_window || !m_camera)
    {
        return;
    }

    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_window, true);
    }

    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_camera->ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
        m_camera->ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
        m_camera->ProcessKeyboard(UP, deltaTime);
}

void InputController::FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void InputController::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_instance || !s_instance->m_camera)
    {
        return;
    }

    bool rightButtonPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (rightButtonPressed)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (s_instance->m_firstMouse)
        {
            s_instance->m_lastX = static_cast<float>(xpos);
            s_instance->m_lastY = static_cast<float>(ypos);
            s_instance->m_firstMouse = false;
        }

        float xoffset = static_cast<float>(xpos) - s_instance->m_lastX;
        float yoffset = s_instance->m_lastY - static_cast<float>(ypos);

        s_instance->m_camera->ProcessMouseMovement(xoffset, yoffset);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        s_instance->m_firstMouse = true;
    }

    s_instance->m_lastX = static_cast<float>(xpos);
    s_instance->m_lastY = static_cast<float>(ypos);
}


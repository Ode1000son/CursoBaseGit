#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"

class InputController
{
public:
    InputController();

    void Initialize(Camera* camera);
    void AttachWindow(GLFWwindow* window);
    void ProcessInput(float deltaTime);

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

private:
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    static InputController* s_instance;

    Camera* m_camera = nullptr;
    GLFWwindow* m_window = nullptr;
    bool m_firstMouse = true;
    float m_lastX = 400.0f;
    float m_lastY = 300.0f;
};


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "camera.h"
#include "renderer.h"

Camera camera(glm::vec3(0.0f, 2.0f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -25.0f);
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window, float deltaTime);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void HandleOverrideShortcuts(GLFWwindow* window, Renderer& renderer);

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Aula 8.2 - Sistema de Renderizacao Modular", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, MouseCallback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Renderer renderer;
    if (!renderer.Initialize())
    {
        std::cerr << "Falha ao inicializar o renderer modular." << std::endl;
        renderer.Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window, deltaTime);
        HandleOverrideShortcuts(window, renderer);
        renderer.RenderFrame(window, camera, currentFrame);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void FramebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    static_cast<void>(window);

    bool rightButtonPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (rightButtonPressed)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (firstMouse)
        {
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }

        float xoffset = static_cast<float>(xpos) - lastX;
        float yoffset = lastY - static_cast<float>(ypos);

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }

    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
}

void HandleOverrideShortcuts(GLFWwindow* window, Renderer& renderer)
{
    static bool key1Held = false;
    static bool key2Held = false;
    static bool key3Held = false;

    auto handleKey = [&](int key, bool& heldState, TextureOverrideMode mode) {
        bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;
        if (isPressed && !heldState && renderer.GetOverrideMode() != mode)
        {
            renderer.SetOverrideMode(mode);
        }
        heldState = isPressed;
    };

    handleKey(GLFW_KEY_1, key1Held, TextureOverrideMode::Imported);
    handleKey(GLFW_KEY_2, key2Held, TextureOverrideMode::Checker);
    handleKey(GLFW_KEY_3, key3Held, TextureOverrideMode::Highlight);
}

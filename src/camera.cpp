#include "camera.h"

/// @brief Construtor da câmera
/// @param position Posição inicial da câmera
/// @param up Vetor "up" do mundo
/// @param yaw Ângulo horizontal inicial (em graus)
/// @param pitch Ângulo vertical inicial (em graus)
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position)
    , m_worldUp(up)
    , m_yaw(yaw)
    , m_pitch(pitch)
    , m_movementSpeed(2.5f)
    , m_mouseSensitivity(0.1f)
    , m_zoom(45.0f)
{
    updateCameraVectors();
}

/// @brief Retorna a matriz de visão (View Matrix) usando LookAt
/// @return Matriz 4x4 de visão
glm::mat4 Camera::GetViewMatrix() const
{
    // glm::lookAt(posição_da_câmera, alvo, vetor_up)
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

/// @brief Processa entrada do teclado para mover a câmera
/// @param direction Direção do movimento (FORWARD, BACKWARD, LEFT, RIGHT)
/// @param deltaTime Tempo decorrido desde o último frame (para movimento suave)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = m_movementSpeed * deltaTime;

    switch (direction) {
        case FORWARD:
            m_position += m_front * velocity;
            break;
        case BACKWARD:
            m_position -= m_front * velocity;
            break;
        case LEFT:
            m_position -= m_right * velocity;
            break;
        case RIGHT:
            m_position += m_right * velocity;
            break;
        case UP:
            m_position += m_worldUp * velocity;
            break;
        case DOWN:
            m_position -= m_worldUp * velocity;
            break;
    }
}

/// @brief Processa movimento do mouse para rotacionar a câmera
/// @param xoffset Deslocamento horizontal do mouse
/// @param yoffset Deslocamento vertical do mouse
/// @param constrainPitch Se deve limitar o pitch para evitar capotamento
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;

    m_yaw += xoffset;
    m_pitch += yoffset;

    // Limita o pitch para evitar que a câmera faça "capotamento"
    if (constrainPitch) {
        if (m_pitch > 89.0f)
            m_pitch = 89.0f;
        if (m_pitch < -89.0f)
            m_pitch = -89.0f;
    }

    // Atualiza os vetores da câmera com os novos ângulos
    updateCameraVectors();
}

/// @brief Atualiza os vetores da câmera (front, right, up) baseado nos ângulos
void Camera::updateCameraVectors()
{
    // Calcula o vetor frontal baseado nos ângulos yaw e pitch
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));

    // Normaliza o vetor frontal
    m_front = glm::normalize(front);

    // Recalcula os vetores right e up usando produto vetorial
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

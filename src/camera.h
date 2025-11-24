#pragma once

// Sistema de câmera FPS-style com controle de mouse e teclado
// Suporta movimento livre, rotação, zoom e configuração dinâmica de parâmetros.
// Usado para visão do jogador e como listener do sistema de áudio 3D.

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Direções de movimento da câmera
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Classe que representa uma câmera FPS-style
class Camera {
public:
    // === CONSTRUTOR ===
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    // === MATRIZ DE VISÃO ===
    // Retorna a matriz View (LookAt) da câmera
    glm::mat4 GetViewMatrix() const;

    // === CONTROLES ===
    // Processa entrada do teclado (WASD)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    // Processa movimento do mouse
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // === GETTERS ===
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetFront() const { return m_front; }
    glm::vec3 GetUp() const { return m_up; }
    float GetZoom() const { return m_zoom; }

    // === CONFIGURAÇÃO DINÂMICA ===
    void SetPosition(const glm::vec3& position);
    void SetUp(const glm::vec3& up);
    void SetOrientation(float yaw, float pitch);
    void SetMovementSpeed(float speed);
    void SetMouseSensitivity(float sensitivity);
    void SetZoom(float zoom);

private:
    // === VETORES DA CÂMERA ===
    glm::vec3 m_position;    // Posição da câmera no espaço 3D
    glm::vec3 m_front;       // Vetor frontal (direção para onde olha)
    glm::vec3 m_up;          // Vetor "para cima" da câmera
    glm::vec3 m_right;       // Vetor direito (perpendicular ao frontal e up)
    glm::vec3 m_worldUp;     // Vetor up do mundo (normalmente 0,1,0)

    // === ÂNGULOS DE ROTAÇÃO ===
    float m_yaw;      // Rotação horizontal (em graus)
    float m_pitch;    // Rotação vertical (em graus)

    // === CONFIGURAÇÕES ===
    float m_movementSpeed;    // Velocidade de movimento (unidades/segundo)
    float m_mouseSensitivity; // Sensibilidade do mouse
    float m_zoom;             // FOV da câmera (zoom)

    // === MÉTODOS PRIVADOS ===
    // Atualiza os vetores da câmera baseado nos ângulos
    void updateCameraVectors();
};

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera();

    void Update(float deltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void ProcessKeyboard(int direction, float deltaTime);

    glm::mat4 GetViewMatrix() const { return m_viewMatrix; }
    glm::mat4 GetProjectionMatrix() const { return m_projectionMatrix; }
    glm::vec3 GetPosition() const { return m_position; }

    void SetPosition(const glm::vec3& position) { m_position = position; UpdateViewMatrix(); }
    void SetProjection(float fov, float aspect, float near, float far);

    void Process(float DeltaTime);
private:
    void UpdateCameraVectors();
    void UpdateViewMatrix();

    // Camera attributes
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    // Euler angles
    float m_yaw;
    float m_pitch;

    // Camera options
    float m_movementSpeed;
    float m_mouseSensitivity;
    float m_zoom;

    // Matrices
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
};

// Defines several possible options for camera movement
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};
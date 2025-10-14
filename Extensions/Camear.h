#pragma once

class OrbitalCamera {
public:
    OrbitalCamera(float width, float height) {
        m_Width = width;
        m_Height = height;
        m_UpdateProjection();
        m_UpdateView();
    }

    void Orbit(float deltaYaw, float deltaPitch) {
        m_Yaw += deltaYaw * m_RotationSpeed;
        m_Pitch += deltaPitch * m_RotationSpeed;
        m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);
        m_UpdateView();
    }

    void Zoom(float amount) {
        m_Radius -= amount * m_ZoomSpeed;
        m_Radius = glm::clamp(m_Radius, m_MinRadius, m_MaxRadius);
        m_UpdateView();
    }

    void Pan(float right, float up) {
        m_Target += m_Right * right * m_PanSpeed;
        m_Target += m_Up * up * m_PanSpeed;
        m_UpdateView();
    }

    void SetTarget(const glm::vec3& target) {
        m_Target = target;
        m_UpdateView();
    }

private:
    void m_UpdateView() {
        // Calculate position based on spherical coordinates
        m_Position.x = m_Target.x + m_Radius * cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        m_Position.y = m_Target.y + m_Radius * sin(glm::radians(m_Pitch));
        m_Position.z = m_Target.z + m_Radius * sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

        m_View = glm::lookAt(m_Position, m_Target, m_WorldUp);

        // Update right and up vectors
        m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
        m_Up = glm::normalize(glm::cross(m_Right, m_Front));

        m_ViewProjection = m_Projection * m_View;
    }

private:
    glm::vec3 m_Target = glm::vec3(0.0f);
    glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 m_Right, m_Up;
    glm::vec3 m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_Radius = 5.0f;
    float m_Yaw = -90.0f;
    float m_Pitch = 0.0f;
    float m_MinRadius = 1.0f;
    float m_MaxRadius = 50.0f;
    float m_PanSpeed = 0.1f;
    float m_ZoomSpeed = 0.5f;
};
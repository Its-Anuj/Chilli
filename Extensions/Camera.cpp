#include "Camera.h"
#include "Core/Input/Input.h"

Camera::Camera()
	: m_position(glm::vec3(0.0f, 0.0f, 3.0f)),
	m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
	m_worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
	m_yaw(-90.0f),
	m_pitch(0.0f),
	m_movementSpeed(2.5f),
	m_mouseSensitivity(0.1f),
	m_zoom(45.0f) {
	UpdateCameraVectors();
}

void Camera::Update(float deltaTime) {
	// Update any camera animations or smooth movements here
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
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
	}

	UpdateViewMatrix();
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch) {
	xOffset *= m_mouseSensitivity;
	yOffset *= m_mouseSensitivity;

	m_yaw += xOffset;
	m_pitch += yOffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch) {
		if (m_pitch > 89.0f)
			m_pitch = 89.0f;
		if (m_pitch < -89.0f)
			m_pitch = -89.0f;
	}

	// Update front, right and up vectors using the updated Euler angles
	UpdateCameraVectors();
}

void Camera::SetProjection(float fov, float aspect, float near, float far) {
	m_projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
}

void Camera::Process(float DeltaTime)
{
	if (Chilli::Input::IsKeyPressed(Chilli::Input_key_W) == Chilli::InputResult::INPUT_PRESS
		|| Chilli::Input::IsKeyPressed(Chilli::Input_key_W) == Chilli::InputResult::INPUT_REPEAT)
		ProcessKeyboard(FORWARD, DeltaTime);
	else if (Chilli::Input::IsKeyPressed(Chilli::Input_key_S) == Chilli::InputResult::INPUT_PRESS
		|| Chilli::Input::IsKeyPressed(Chilli::Input_key_S) == Chilli::InputResult::INPUT_REPEAT)
		ProcessKeyboard(BACKWARD, DeltaTime);

	if (Chilli::Input::IsKeyPressed(Chilli::Input_key_A) == Chilli::InputResult::INPUT_PRESS
		|| Chilli::Input::IsKeyPressed(Chilli::Input_key_A) == Chilli::InputResult::INPUT_REPEAT)
		ProcessKeyboard(LEFT, DeltaTime);
	else if (Chilli::Input::IsKeyPressed(Chilli::Input_key_D) == Chilli::InputResult::INPUT_PRESS
		|| Chilli::Input::IsKeyPressed(Chilli::Input_key_D) == Chilli::InputResult::INPUT_REPEAT)
		ProcessKeyboard(RIGHT, DeltaTime);
}

void Camera::UpdateCameraVectors() {
	// Calculate the new front vector
	glm::vec3 newFront;
	newFront.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	newFront.y = sin(glm::radians(m_pitch));
	newFront.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_front = glm::normalize(newFront);

	// Also re-calculate the right and up vector
	m_right = glm::normalize(glm::cross(m_front, m_worldUp));
	m_up = glm::normalize(glm::cross(m_right, m_front));

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix() {
	m_viewMatrix = glm::lookAt(m_position, m_position + m_front, m_up);
}
#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace Chilli
{
	class Transform
	{
	public:
		// Constructors
		Transform()
			: m_position(0.0f), m_rotation(0.0f), m_scale(1.0f), m_dirty(true)
		{
		}

		Transform(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f))
			: m_position(position), m_rotation(rotation), m_scale(scale), m_dirty(true)
		{
		}

		// Getters
		const glm::vec3& GetPosition() const { return m_position; }
		const glm::vec3& GetRotation() const { return m_rotation; }
		const glm::vec3& GetScale() const { return m_scale; }

		// Setters that mark as dirty
		void SetPosition(const glm::vec3& position)
		{
			m_position = position;
			m_dirty = true;
		}

		void SetRotation(const glm::vec3& rotation)
		{
			m_rotation = rotation;
			m_dirty = true;
		}

		void SetScale(const glm::vec3& scale)
		{
			m_scale = scale;
			m_dirty = true;
		}

		// Transformation methods
		void Move(const glm::vec3& translation)
		{
			m_position += translation;
			m_dirty = true;
		}

		void MoveX(float x)
		{
			Move({ x, 0.0f, 0.0f });
		}
		void MoveY(float y)
		{
			Move({ 0.0, y, 0.0f });
		}
		void MoveZ(float z)
		{
			Move({ 0.0, 0.0f, z });
		}

		void Move(float x, float y, float z)
		{
			m_position += glm::vec3(x, y, z);
			m_dirty = true;
		}

		void Rotate(const glm::vec3& rotation)
		{
			m_rotation += rotation;
			m_dirty = true;
		}

		void Rotate(float x, float y, float z)
		{
			m_rotation += glm::vec3(x, y, z);
			m_dirty = true;
		}

		void Scale(const glm::vec3& scale)
		{
			m_scale *= scale;
			m_dirty = true;
		}

		void Scale(float x, float y, float z)
		{
			m_scale *= glm::vec3(x, y, z);
			m_dirty = true;
		}

		void Scale(float uniformScale)
		{
			m_scale *= uniformScale;
			m_dirty = true;
		}

		// Get transformation matrix (lazy evaluation)
		const glm::mat4& GetTransformationMatrix()
		{
			if (m_dirty)
			{
				UpdateTransformationMatrix();
				m_dirty = false;
			}
			return m_transformationMat;
		}

		// Force matrix update
		void ForceUpdate()
		{
			UpdateTransformationMatrix();
			m_dirty = false;
		}

		// Check if transform is dirty
		bool IsDirty() const { return m_dirty; }


	private:
		void UpdateTransformationMatrix()
		{
			// Create transformation matrix: Scale * Rotation * Translation
			m_transformationMat = glm::mat4(1.0f);

			// Translation
			m_transformationMat = glm::translate(m_transformationMat, m_position);

			// Rotation (using yaw-pitch-roll order: Y * X * Z)
			m_transformationMat = glm::rotate(m_transformationMat, m_rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw (Y)
			m_transformationMat = glm::rotate(m_transformationMat, m_rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch (X)
			m_transformationMat = glm::rotate(m_transformationMat, m_rotation.z, glm::vec3(0.0f, 0.0f, 1.0f)); // Roll (Z)

			// Scale
			m_transformationMat = glm::scale(m_transformationMat, m_scale);
		}

	private:
		glm::vec3 m_position;
		glm::vec3 m_rotation; // Euler angles in radians (pitch, yaw, roll)
		glm::vec3 m_scale;

		glm::mat4 m_transformationMat = glm::mat4(1.0f);
		bool m_dirty;
	};
}
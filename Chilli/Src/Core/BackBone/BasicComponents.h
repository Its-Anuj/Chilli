#pragma once

#include "Maths.h"
#include "Events/Events.h"
#include "Window\Window.h"
#include "Renderer/RenderClasses.h"
#include "Renderer/Mesh.h"
#include "Material.h"

namespace Chilli
{
	// Specifies that the entity with this component is visible
	struct VisibilityComponent;

	struct RenderSurfaceComponent
	{
		// --- Surface / Swapchain handles ---
		void* NativeSurface = nullptr;             // Platform-specific surface (VkSurfaceKHR, etc.)
		void* SwapchainHandle = nullptr;           // Backend-managed swapchain (can be stored as a backend handle)

		// --- Dimensions / Viewport ---
		IVec2 Size = { 0, 0 };
		bool  Resized = false;

		// --- Synchronization / Frame info ---
		uint32_t CurrentFrame = 0;
		uint32_t MaxFramesInFlight = 2;

		// --- Optional: Callbacks or flags ---
		bool EnableVSync = true;
	};

	struct SceneRenderSurfaceComponent
	{
		BackBone::AssetHandle<Mesh> ScreenMeshHandle;
		BackBone::AssetHandle<Material> ScreenMaterialHandle;
	};

	struct PresentRenderSurfaceComponent
	{
		BackBone::AssetHandle<Mesh> Mesh;
		BackBone::AssetHandle<Material> Mat;
	};

	struct MaterialComponent
	{
		Vec4 AlbedoColor;
	};

	struct MeshComponent
	{
		BackBone::AssetHandle<Mesh> MeshHandle;
		BackBone::AssetHandle<Material> MaterialHandle;
	};

	struct TransformComponent
	{
	public:
		TransformComponent(
			const Vec3& Pos = { 0, 0, 0 },
			const Vec3& Scle = { 1, 1, 1 },
			const Vec3& Rot = { 0, 0, 0 },
			const BackBone::Entity Parent = BackBone::npos
		)
			: _Position(Pos),
			_Scale(Scle),
			_Version(1),
			_LocalWorldMatrix(1.0f),
			_WorldMatrix(1.0f),
			_Parent(Parent)
		{
			SetEulerRotation(Rot);
		}

		// --- Absolute Setters ---
		void SetPosition(const Vec3& Pos)
		{
			if (_Position != Pos) {
				_Position = Pos;
				IncrementVersion();
			}
		}

		void SetScale(const Vec3& Scle)
		{
			if (_Scale != Scle) {
				_Scale = Scle;
				IncrementVersion();
			}
		}


		// --- Relative Movement ---
		void Move(const Vec3& Delta) { _Position += Delta; IncrementVersion(); }
		void MoveX(float Delta) { _Position.x += Delta; IncrementVersion(); }
		void MoveY(float Delta) { _Position.y += Delta; IncrementVersion(); }
		void MoveZ(float Delta) { _Position.z += Delta; IncrementVersion(); }

		// --- Relative Scaling ---
		void AddScale(const Vec3& Delta) { _Scale += Delta; IncrementVersion(); }
		void ScaleX(float Delta) { _Scale.x += Delta; IncrementVersion(); }
		void ScaleY(float Delta) { _Scale.y += Delta; IncrementVersion(); }
		void ScaleZ(float Delta) { _Scale.z += Delta; IncrementVersion(); }

		// Rotate around X axis by degrees
		void RotateX(float Degrees)
		{
			glm::quat Delta = glm::angleAxis(glm::radians(Degrees), glm::vec3(1, 0, 0));
			_Rotation = glm::normalize(_Rotation * Delta);
			IncrementVersion();
		}

		// Rotate around Y axis by degrees
		void RotateY(float Degrees)
		{
			glm::quat Delta = glm::angleAxis(glm::radians(Degrees), glm::vec3(0, 1, 0));
			_Rotation = glm::normalize(_Rotation * Delta);
			IncrementVersion();
		}

		// Rotate around Z axis by degrees
		void RotateZ(float Degrees)
		{
			glm::quat Delta = glm::angleAxis(glm::radians(Degrees), glm::vec3(0, 0, 1));
			_Rotation = glm::normalize(_Rotation * Delta);
			IncrementVersion();
		}

		// Set absolute rotation from euler angles — replaces current rotation
		void SetEulerRotation(float PitchX, float YawY, float RollZ)
		{
			_Rotation = glm::normalize(glm::quat(glm::radians(glm::vec3(PitchX, YawY, RollZ))));
			IncrementVersion();
		}

		// Set absolute rotation from euler angles — replaces current rotation
		void SetEulerRotation(const Vec3& Angles)
		{
			SetEulerRotation(Angles.x, Angles.y, Angles.z);
		}

		// Get euler angles back (degrees) — for UI display only
		Chilli::Vec3 GetEulerAngles()
		{
			glm::vec3 Euler = glm::degrees(glm::eulerAngles(_Rotation));
			return { Euler.x, Euler.y, Euler.z };
		}

		// Get forward vector — where is this object facing
		Chilli::Vec3 GetForward()
		{
			glm::vec3 F = _Rotation * glm::vec3(0, 0, -1);
			return { F.x, F.y, F.z };
		}

		// Get right vector
		Chilli::Vec3 GetRight()
		{
			glm::vec3 R = _Rotation * glm::vec3(1, 0, 0);
			return { R.x, R.y, R.z };
		}

		// Get up vector
		Chilli::Vec3 GetUp()
		{
			glm::vec3 U = _Rotation * glm::vec3(0, 1, 0);
			return { U.x, U.y, U.z };
		}

		// --- Getters ---
		const Vec3& GetPosition() const { return _Position; }
		const Vec3& GetScale()    const { return _Scale; }
		uint32_t    GetVersion()  const { return _Version; }

		void UpdateWorldMatrix(const glm::mat4& ParentWorldMat, bool IsParentDirty)
		{
			if (HasParent() && IsParentDirty)
				IncrementVersion();

			if (_LastUpdatedVersion != _Version ||
				(HasParent() && IsParentDirty))
				CalculateWorldMatrix(ParentWorldMat);
		}

		const glm::mat4& GetLocalWorldMatrix() {
			return _LocalWorldMatrix;
		}

		const glm::mat4& GetWorldMatrix() {
			return _WorldMatrix;
		}

		// --- System Internal ---
		void SetWorldMatrix(const glm::mat4& Matrix)
		{
			_WorldMatrix = Matrix;
			IncrementVersion();
		}

		bool IsDirty()
		{
			return _Version != _LastUpdatedVersion;
		}

		bool HasParent() { return _Parent != BackBone::npos; }
		BackBone::Entity GetParent() { return _Parent; }
		void SetParent(BackBone::Entity Parent) { _Parent = Parent; }
		void SetVersion(uint32_t NewVersion) {
			_Version = NewVersion;
		}

	private:
		void CalculateWorldMatrix(const glm::mat4& ParentWorldMat)
		{
			CalculateLocalMatrix();
			if (HasParent())
				_WorldMatrix = ParentWorldMat * _LocalWorldMatrix;
			else
				_WorldMatrix = _LocalWorldMatrix;
			_WorldPosition = { _WorldMatrix[3].x, _WorldMatrix[3].y, _WorldMatrix[3].z };

			// Extract scale
			_WorldScale.x = glm::length(glm::vec3(_WorldMatrix[0]));
			_WorldScale.y = glm::length(glm::vec3(_WorldMatrix[1]));
			_WorldScale.z = glm::length(glm::vec3(_WorldMatrix[2]));

			// Extract rotation — remove scale from matrix first
			glm::mat4 RotMat = _WorldMatrix;
			RotMat[0] /= _WorldScale.x;
			RotMat[1] /= _WorldScale.y;
			RotMat[2] /= _WorldScale.z;
			_WorldRotation = glm::quat_cast(RotMat); // matrix → quat

			_LastUpdatedVersion = _Version;
		}

		void CalculateLocalMatrix()
		{
			glm::mat4 Transform(1.0f);

			Transform = glm::translate(
				Transform,
				glm::vec3(_Position.x, _Position.y, _Position.z)
			);

			glm::mat4 Rotation = glm::mat4_cast(_Rotation);
			Transform = Transform * Rotation;

			Transform = glm::scale(
				Transform,
				glm::vec3(_Scale.x, _Scale.y, _Scale.z)
			);

			_LocalWorldMatrix = Transform;
		}

		void IncrementVersion()
		{
			++_Version;
			if (_Version == 0) _Version = 1;
		}

	private:
		// Local
		Vec3 _Position;
		Vec3 _Scale;
		glm::quat _Rotation = glm::quat(1, 0, 0, 0);
		glm::mat4 _LocalWorldMatrix;

		Vec3 _WorldPosition;
		Vec3 _WorldScale;
		glm::quat _WorldRotation = glm::quat(1, 0, 0, 0);

		BackBone::Entity _Parent = BackBone::npos;

		uint32_t _Version = 1;
		uint32_t _LastUpdatedVersion = 0;
		glm::mat4 _WorldMatrix;
	};
}

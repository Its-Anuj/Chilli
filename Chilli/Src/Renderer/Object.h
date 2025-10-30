#pragma once

#include "Mesh.h"
#include "Material.h"
#include "glm/glm.hpp"

namespace Chilli
{
	struct Transform
	{
	public:
		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;

		glm::mat4 TransformationMat;
	private:
	};

	struct Object
	{
		UUID MeshIndex;
		UUID MaterialIndex;

		UUID ID;

		Transform Transform;
	};

	// Data structure of how it is in the Shader
	struct ObjectShaderData
	{
		glm::mat4 TransformationMat;
	};
}

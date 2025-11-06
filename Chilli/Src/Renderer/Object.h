#pragma once

#include "Mesh.h"
#include "Material.h"
#include "Transform.h"
#include "ObjectShapes.h"

namespace Chilli
{
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

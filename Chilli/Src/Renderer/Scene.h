#pragma once

#include "glm\glm.hpp"
#include "Object.h"
#include "Maths.h"

namespace Chilli
{
	struct Scene
	{
		glm::mat4 ViewProjMatrix;
		glm::vec3 CameraPos;

		std::vector<Object> Objects;
	};


	struct SceneShaderData
	{
		glm::mat4 ViewProjMatrix;
		glm::vec4 CameraPos;
	};
}

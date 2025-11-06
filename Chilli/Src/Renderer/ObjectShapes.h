#pragma once
#include "glm/glm.hpp"

namespace Chilli
{
	struct Vertex {
		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec3 Normals;
	};

	// ---- Square ----

	// returns required count for Vertices and for Indicies
	constexpr inline std::pair<uint32_t, uint32_t> GetSquareMeshSpec()
	{
		return { 4, 6 };
	}

	constexpr inline void FillSquareMeshData(Vertex* Vertices, uint16_t* Indicies)
	{
		// Front face
		Vertices[0] = { glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };  // bottom-left
		Vertices[1] = { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };  // bottom-right
		Vertices[2] = { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };  // top-right
		Vertices[3] = { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f) };  // top-left

		Indicies[0] = 0;
		Indicies[1] = 1;
		Indicies[2] = 2;
		Indicies[3] = 2;
		Indicies[4] = 3;
		Indicies[5] = 0;
	}

    // ---- Cube ----

    // returns required count for Vertices and for Indicies
    constexpr std::pair<uint32_t, uint32_t> GetCubeMeshSpec()
    {
        return { 24, 36 };
    }

    constexpr inline void FillCubeMeshData(Vertex* Vertices, uint16_t* Indices)
    {
        // Vertex positions for a cube (-0.5 to 0.5)
        // 24 vertices (4 per face × 6 faces) for unique normals/texcoords per face

        // Front face (Z = 0.5)
        Vertices[0] = { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f,  1.0f) };
        Vertices[1] = { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f,  1.0f) };
        Vertices[2] = { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f,  1.0f) };
        Vertices[3] = { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f,  1.0f) };

        // Back face (Z = -0.5)
        Vertices[4] = { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
        Vertices[5] = { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
        Vertices[6] = { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f) };
        Vertices[7] = { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f) };

        // Right face (X = 0.5)
        Vertices[8] = { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
        Vertices[9] = { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
        Vertices[10] = { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) };
        Vertices[11] = { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) };

        // Left face (X = -0.5)
        Vertices[12] = { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f) };
        Vertices[13] = { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f) };
        Vertices[14] = { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) };
        Vertices[15] = { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f) };

        // Top face (Y = 0.5)
        Vertices[16] = { glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f,  1.0f, 0.0f) };
        Vertices[17] = { glm::vec3(0.5f,  0.5f,  0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f,  1.0f, 0.0f) };
        Vertices[18] = { glm::vec3(0.5f,  0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f,  1.0f, 0.0f) };
        Vertices[19] = { glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f,  1.0f, 0.0f) };

        // Bottom face (Y = -0.5)
        Vertices[20] = { glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) };
        Vertices[21] = { glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f) };
        Vertices[22] = { glm::vec3(0.5f, -0.5f,  0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) };
        Vertices[23] = { glm::vec3(-0.5f, -0.5f,  0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f) };

        // Indices for 12 triangles (2 per face × 6 faces)
        // Each face uses counter-clockwise winding

        // Front face
        Indices[0] = 0;  Indices[1] = 1;  Indices[2] = 2;
        Indices[3] = 2;  Indices[4] = 3;  Indices[5] = 0;

        // Back face
        Indices[6] = 4;  Indices[7] = 5;  Indices[8] = 6;
        Indices[9] = 6;  Indices[10] = 7; Indices[11] = 4;

        // Right face
        Indices[12] = 8;  Indices[13] = 9;  Indices[14] = 10;
        Indices[15] = 10; Indices[16] = 11; Indices[17] = 8;

        // Left face
        Indices[18] = 12; Indices[19] = 13; Indices[20] = 14;
        Indices[21] = 14; Indices[22] = 15; Indices[23] = 12;

        // Top face
        Indices[24] = 16; Indices[25] = 17; Indices[26] = 18;
        Indices[27] = 18; Indices[28] = 19; Indices[29] = 16;

        // Bottom face
        Indices[30] = 20; Indices[31] = 21; Indices[32] = 22;
        Indices[33] = 22; Indices[34] = 23; Indices[35] = 20;
    }
}

#pragma once

#include <GL\glew.h>

#include <array>
#include <vector>

class MarchingCubes

{
public:

	MarchingCubes();
	~MarchingCubes();

	void SetIsolevel(const GLfloat& Iso);
	void CreateMesh(const std::vector<std::vector<std::vector<GLfloat>>>& Grid);

	std::vector<GLuint> GetIndices() const;
	std::vector<glm::vec3> GetVertices() const;
	std::vector<glm::vec3> GetNormals() const;

private:

	GLfloat GetOffset(const GLfloat& v1, const GLfloat& v2) const;
	void MakeCube(const std::vector<std::vector<std::vector<GLfloat>>>& Grid, const GLuint& x, const GLuint& y, const GLuint& z);
	void MarchCube(const GLuint& x, const GLuint& y, const GLuint& z);
	void CalculateNormals(const std::vector<std::vector<std::vector<GLfloat>>>& Grid);

	// Tables
	static std::array<std::array<GLfloat, 3>, 8> VertexOffset;
	static std::array<std::array<GLint, 2>, 12> EdgeConnection;
	static std::array<std::array<GLfloat, 3>, 12> EdgeDirection;
	static std::array<GLint, 256> EdgeTable;
	static std::array<std::array<GLint, 16>, 256> TriangleTable;

	// Buffers to refrain from allocating new memory every frame...
	std::array<glm::vec3, 12> NewVerts;
	std::array<GLfloat, 8> Cube;

	std::vector<GLuint> Indices;
	std::vector<glm::vec3> Vertices, Normals;

	GLfloat Isolevel;
};


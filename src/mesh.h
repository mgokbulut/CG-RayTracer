#pragma once
#include "disable_all_warnings.h"
// Suppress warnings in third-party code.
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gsl-lite/gsl-lite.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <vector>

struct Vertex {
    glm::vec3 p; // Position.
    glm::vec3 n; // Normal.
};

struct Material {
    glm::vec3 kd; // Diffuse color.
    glm::vec3 ks { 0.0f };
    float shininess { 1.0f };

    float transparency { 1.0f };
};

using Triangle = glm::uvec3;

struct Mesh {
    // Vertices contain the vertex positions and normals of the mesh.
    std::vector<Vertex> vertices;
    // Triangles are the indices of the vertices involved in a triangle.
    // A triangle, thus, contains a triplet of values corresponding to the 3 vertices of a triangle.
    std::vector<Triangle> triangles;

    Material material;
};

[[nodiscard]] std::vector<Mesh> loadMesh(const std::filesystem::path& file, bool normalize = false);

#pragma once
#include "mesh.h"
#include "ray.h"
#include "scene.h"

enum class DrawMode {
    Filled,
    Wireframe
};

void drawMesh(const Mesh& mesh);
void drawSphere(const Sphere& sphere);
void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f));
void drawAABB(const AxisAlignedBox& box, DrawMode drawMode = DrawMode::Filled, const glm::vec3& color = glm::vec3(1.0f), float transparency = 1.0f);

void drawScene(const Scene& scene);

extern bool enableDrawRay;
void drawRay(const Ray& ray, const glm::vec3& color = glm::vec3(1.0f));

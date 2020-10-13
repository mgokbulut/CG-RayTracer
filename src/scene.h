#pragma once
#include "disable_all_warnings.h"
#include "mesh.h"
#include "ray.h"
// Suppress warnings in third-party code.
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <optional>
#include <vector>

enum SceneType {
    SingleTriangle,
    Cube,
    CornellBox,
    CornellBoxSphericalLight,
    Monkey,
    Dragon,
    //AABBs,
    Spheres,
    //Mixed,
    Custom
};

struct Plane {
    float D = 0.0f;
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
};

struct AxisAlignedBox {
    glm::vec3 lower { 0.0f };
    glm::vec3 upper { 1.0f };
};

struct Sphere {
    glm::vec3 center { 0.0f };
    float radius = 1.0f;
    Material material;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
};

struct SphericalLight {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
};

struct Scene {
    std::vector<Mesh> meshes;
    std::vector<Sphere> spheres;
    //std::vector<AxisAlignedBox> boxes;

    std::vector<PointLight> pointLights;
    std::vector<SphericalLight> sphericalLight;
};

// Load a prebuilt scene.
Scene loadScene(SceneType type, const std::filesystem::path& dataDir);

#include "scene.h"
#include <iostream>

Scene loadScene(SceneType type, const std::filesystem::path& dataDir)
{
    Scene scene;
    switch (type) {
    case SingleTriangle: {
        // Load a 3D model with a single triangle
        auto subMeshes = loadMesh(dataDir / "triangle.obj");
        subMeshes[0].material.kd = glm::vec3(1.0f);
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.pointLights.push_back(PointLight { glm::vec3(-1, 1, -1), glm::vec3(1) });
    } break;
    case Cube: {
        // Load a 3D model of a cube with 12 triangles
        auto subMeshes = loadMesh(dataDir / "cube.obj");
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.pointLights.push_back(PointLight { glm::vec3(-1, 1, -1), glm::vec3(1) });
    } break;
    case CornellBox: {
        // Load a 3D model of a Dragon
        auto subMeshes = loadMesh(dataDir / "CornellBox-Mirror-Rotated.obj", true);
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.pointLights.push_back(PointLight { glm::vec3(0, 0.58f, 0), glm::vec3(1) }); // Light at the top of the box
    } break;
    case CornellBoxSphericalLight: {
        // Load a 3D model of a Dragon
        auto subMeshes = loadMesh(dataDir / "CornellBox-Mirror-Rotated.obj", true);
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.sphericalLight.push_back(SphericalLight { glm::vec3(0, 0.45f, 0), 0.1f, glm::vec3(1) }); // Light at the top of the box
    } break;
    case Monkey: {
        // Load a 3D model of a Dragon
        auto subMeshes = loadMesh(dataDir / "monkey-rotated.obj", true);
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.pointLights.push_back(PointLight { glm::vec3(-1, 1, -1), glm::vec3(1) });
        scene.pointLights.push_back(PointLight { glm::vec3(1, -1, -1), glm::vec3(1) });
    } break;
    case Dragon: {
        // Load a 3D model of a Dragon
        auto subMeshes = loadMesh(dataDir / "dragon.obj", true);
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        scene.pointLights.push_back(PointLight { glm::vec3(-1, 1, -1), glm::vec3(1) });
    } break;
    /*case AABBs: {
        //scene.boxes.push_back(AxisAlignedBox { glm::vec3(-2.0f, -2.0f, 5.0f), glm::vec3(-1.0f, -1.0f, 6.0f) });
        //scene.boxes.push_back(AxisAlignedBox { glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(1.5f, 1.5f, 7.0f) });
        //scene.boxes.push_back(AxisAlignedBox { glm::vec3(0.5f, 0.5f, 2.0f), glm::vec3(0.9f, 0.9f, 2.5f) });
    } break;*/
    case Spheres: {
        scene.spheres.push_back(Sphere { glm::vec3(3.0f, -2.0f, 10.2f), 1.0f, Material { glm::vec3(0.8f, 0.2f, 0.2f) } });
        scene.spheres.push_back(Sphere { glm::vec3(-2.0f, 2.0f, 4.0f), 2.0f, Material { glm::vec3(0.6f, 0.8f, 0.2f) } });
        scene.spheres.push_back(Sphere { glm::vec3(0.0f, 0.0f, 6.0f), 0.75f, Material { glm::vec3(0.2f, 0.2f, 0.8f) } });
        scene.pointLights.push_back(PointLight { glm::vec3(3, 0, 3), glm::vec3(15) });
    } break;
    case Custom: {
        // === Replace custom.obj by your own 3D model (or call your 3D model custom.obj) ===
        auto subMeshes = loadMesh(dataDir / "custom.obj");
        std::move(std::begin(subMeshes), std::end(subMeshes), std::back_inserter(scene.meshes));
        // === CHANGE THE LIGHTING IF DESIRED ===
        scene.pointLights.push_back(PointLight { glm::vec3(-1, 1, -1), glm::vec3(1) });
        // Spherical light: position, radius, color
        //scene.sphericalLight.push_back(SphericalLight{ glm::vec3(0, 1.5f, 0), 0.2f, glm::vec3(1) });
    } break;
    };

    return scene;
}

#pragma once
#include "scene.h"

struct HitInfo
{
    glm::vec3 normal;
    Material material;
};

bool intersectRayWithPlane(const Plane &plane, Ray &ray);

// Returns true if the point p is inside the triangle spanned by v0, v1, v2 with normal n.
bool pointInTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &n, const glm::vec3 &p);

Plane trianglePlane(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2);

bool intersectRayWithTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Ray &ray, HitInfo &hitInfo, const glm::vec3 &n1, const glm::vec3 &n2,const glm::vec3 &n3);
bool intersectRayWithShape(const Sphere &sphere, Ray &ray, HitInfo &hitInfo);
bool intersectRayWithShape(const AxisAlignedBox &box, Ray &ray);
bool intersectRayWithShape(const Mesh &mesh, Ray &ray, HitInfo &hitInfo);

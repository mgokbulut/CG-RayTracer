#include "ray_tracing.h"
#include "disable_all_warnings.h"
// Suppress warnings in third-party code.
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <iostream>
#include <limits>

bool pointInTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &n, const glm::vec3 &p)
{
    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v1v2 = v2 - v1;
    glm::vec3 v2v0 = v0 - v2;

    glm::vec3 v0p = p - v0;
    glm::vec3 v1p = p - v1;
    glm::vec3 v2p = p - v2;

    if (glm::dot(n, glm::cross(v0v1, v0p)) >= 0 && glm::dot(n, glm::cross(v1v2, v1p)) >= 0 && glm::dot(n, glm::cross(v2v0, v2p)) >= 0)
    {
        return true;
    }
    return false;
}

bool intersectRayWithPlane(const Plane &plane, Ray &ray)
{
    // there is an intersection if the ray origin lies inside the plane
    if (glm::dot(ray.origin, plane.normal) == plane.D)
    {
        ray.t = 0;
        return true;
    }

    // the ray and the plane are parallel and the ray's origin isn't inside the plane
    float denominator = glm::dot(ray.direction, plane.normal);
    if (denominator == 0)
    {
        return false;
    }

    // the ray and the plane will have an intersection other than the origin
    float numerator = plane.D - glm::dot(ray.origin, plane.normal);
    float t = numerator / denominator;
    if (t < 0)
    { // the point is behind the camera
        return false;
    }

    // the new point is further than the point that is currently closest
    if (t >= ray.t)
    {
        return false;
    }

    ray.t = t;
    return true;
}

Plane trianglePlane(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2)
{
    Plane plane;
    glm::vec3 u = v1 - v0;
    glm::vec3 v = v2 - v0;
    plane.normal = glm::normalize(glm::cross(u, v));
    plane.D = glm::dot(v0, plane.normal);
    return plane;
}

/// Input: the three vertices of the triangle
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, Ray &ray, HitInfo &hitInfo)
{
    Plane plane = trianglePlane(v0, v1, v2);
    float prevT = ray.t;
    if (intersectRayWithPlane(plane, ray))
    {
        if (pointInTriangle(v0, v1, v2, plane.normal, ray.origin + ray.direction * ray.t))
        {
            // update the hitinfo for further calculations
            if (glm::dot(plane.normal, -ray.direction) > 0)
            {
                hitInfo.normal = plane.normal;
            }
            else
            {
                hitInfo.normal = -plane.normal;
            }
            return true;
        }
        // rollback the value of t in case there was a plane intersection but no triangle intersection
        ray.t = prevT;
    }

    return false;
}

/// Input: a sphere with the following attributes: sphere.radius, sphere.center
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const Sphere &sphere, Ray &ray, HitInfo &hitInfo)
{
    Ray centeredRay = {ray.origin - sphere.center, ray.direction, ray.t};

    float a = glm::dot(centeredRay.direction, centeredRay.direction);
    float b = 2 * glm::dot(centeredRay.direction, centeredRay.origin);
    float c = glm::dot(centeredRay.origin, centeredRay.origin) - sphere.radius * sphere.radius;

    float D = b * b - 4 * a * c;
    if (D < 0)
    {
        return false;
    }

    float smallerT = (-b - sqrt(D)) / (2 * a);
    float biggerT = (-b + sqrt(D)) / (2 * a);

    float currentT;
    if (smallerT >= 0)
    { // we are in front of the sphere
        currentT = smallerT;
    }
    else if (biggerT >= 0)
    { // we are inside the sphere
        currentT = biggerT;
    }
    else
    { // the sphere is behind us
        return false;
    }

    if (currentT >= ray.t)
    { // some object we intersected earlier is closer
        return false;
    }

    ray.t = currentT;
    // update the hitinfo for further calculations
    hitInfo.normal = glm::normalize(ray.origin + ray.direction * ray.t - sphere.center);
    return true;
}

/// Input: an axis-aligned bounding box with the following parameters: minimum coordinates box.lower and maximum coordinates box.upper
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const AxisAlignedBox &box, Ray &ray)
{
    glm::vec3 tMin = (box.lower - ray.origin) / ray.direction;
    glm::vec3 tMax = (box.upper - ray.origin) / ray.direction;

    float tInX = tMin.x < tMax.x ? tMin.x : tMax.x;
    float tOutX = tMin.x > tMax.x ? tMin.x : tMax.x;
    float tInY = tMin.y < tMax.y ? tMin.y : tMax.y;
    float tOutY = tMin.y > tMax.y ? tMin.y : tMax.y;
    float tInZ = tMin.z < tMax.z ? tMin.z : tMax.z;
    float tOutZ = tMin.z > tMax.z ? tMin.z : tMax.z;

    // max (tInX, tInY, tInZ)
    float tIn = tInX > tInY ? (tInX > tInZ ? tInX : tInZ) : (tInY > tInZ ? tInY : tInZ);
    // min (tOutX, tOutY, tOutZ)
    float tOut = tOutX < tOutY ? (tOutX < tOutZ ? tOutX : tOutZ) : (tOutY < tOutZ ? tOutY : tOutZ);

    float currentT;
    if (tIn > tOut || tOut < 0)
    { // we miss the box or it is behind us
        return false;
    }
    else if (tIn < 0)
    { // we are inside the box
        currentT = tOut;
    }
    else
    { // the box is in front of us
        currentT = tIn;
    }

    if (currentT >= ray.t)
    {
        return false;
    }

    ray.t = currentT;
    return true;
}

bool intersectRayWithShape(const Mesh &mesh, Ray &ray, HitInfo &hitInfo)
{
    bool hit = false;
    for (const auto &tri : mesh.triangles)
    {
        const auto v0 = mesh.vertices[tri[0]];
        const auto v1 = mesh.vertices[tri[1]];
        const auto v2 = mesh.vertices[tri[2]];
        hit |= intersectRayWithTriangle(v0.p, v1.p, v2.p, ray, hitInfo);
    }
    return hit;
}

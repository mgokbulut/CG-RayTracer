#include "bounding_volume_hierarchy.h"
#include "draw.h"

AxisAlignedBox getRootBoundingBox(std::vector<Mesh> &meshes);

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene *pScene)
    : m_pScene(pScene)
{
    std::vector<Mesh> meshes = pScene->meshes;
    AxisAlignedBox root = getRootBoundingBox(meshes);
    Node n{false, root, {}, {}};
    nodes.push_back(n);
    // as an example of how to iterate over all meshes in the scene, look at the intersect method below
}

void getSubNodes(Node node)
{
    glm::vec3 mins = node.AABB.lower;
    glm::vec3 maxs = node.AABB.upper;
    float x = maxs.x - mins.x;
    float y = maxs.y - mins.y;
    float z = maxs.z - mins.z;
    int longestAxis = (x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2);

    // --- TODO List --- //
    // we want to sort the triangles based on their centers to get the median triangle
    // after that, devide the vector of triangles into two vectors
    // fill createTree method to construct tree of those nodes.
    // make node a referance.

    // switch (longestAxis)
    // {
    // case 0:

    //     break;

    // default:
    //     break;
    // }
}

void createTree(Node root)
{
}

AxisAlignedBox getRootBoundingBox(std::vector<Mesh> &meshes)
{
    float max_x = meshes[0].vertices[0].p.x;
    float max_y = meshes[0].vertices[0].p.y;
    float max_z = meshes[0].vertices[0].p.z;
    float min_x = meshes[0].vertices[0].p.x;
    float min_y = meshes[0].vertices[0].p.y;
    float min_z = meshes[0].vertices[0].p.z;

    for (Mesh mesh : meshes)
    {
        std::vector<Vertex> vertecies = mesh.vertices;

        for (Vertex vertex : vertecies)
        {
            glm::vec3 p = vertex.p;
            min_x = (p.x < min_x) ? p.x : min_x;
            min_y = (p.y < min_y) ? p.y : min_y;
            min_z = (p.z < min_z) ? p.z : min_z;

            max_x = (p.x > max_x) ? p.x : max_x;
            max_y = (p.y > max_y) ? p.y : max_y;
            max_z = (p.z > max_z) ? p.z : max_z;
        }
    }
    return AxisAlignedBox{glm::vec3{min_x, min_y, min_z}, glm::vec3{max_x, max_y, max_z}};
}

// Use this function to visualize your BVH. This can be useful for debugging. Use the functions in
// draw.h to draw the various shapes. We have extended the AABB draw functions to support wireframe
// mode, arbitrary colors and transparency.
void BoundingVolumeHierarchy::debugDraw(int level)
{

    // Draw the AABB as a transparent green box.
    //AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    //drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);

    // Draw the AABB as a (white) wireframe box.
    //AxisAlignedBox aabb{glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f)};
    AxisAlignedBox aabb = nodes[0].node;
    //drawAABB(aabb, DrawMode::Wireframe);
    drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1);
}

int BoundingVolumeHierarchy::numLevels() const
{
    return 5;
}

// Return true if something is hit, returns false otherwise. Only find hits if they are closer than t stored
// in the ray and if the intersection is on the correct side of the origin (the new t >= 0). Replace the code
// by a bounding volume hierarchy acceleration structure as described in the assignment. You can change any
// file you like, including bounding_volume_hierarchy.h .
bool BoundingVolumeHierarchy::intersect(Ray &ray, HitInfo &hitInfo) const
{
    bool hit = false;
    // Intersect with all triangles of all meshes.
    for (const auto &mesh : m_pScene->meshes)
    {
        for (const auto &tri : mesh.triangles)
        {
            const auto v0 = mesh.vertices[tri[0]];
            const auto v1 = mesh.vertices[tri[1]];
            const auto v2 = mesh.vertices[tri[2]];
            if (intersectRayWithTriangle(v0.p, v1.p, v2.p, ray, hitInfo))
            {
                hitInfo.material = mesh.material;
                hit = true;
            }
        }
    }
    // Intersect with spheres.
    for (const auto &sphere : m_pScene->spheres)
        hit |= intersectRayWithShape(sphere, ray, hitInfo);
    return hit;
}

// void _max_(float x, float y, float z, bool (&arr)[3])
// {
//     if (x < y)
//     {
//         if (y > z)
//         {
//             arr[1] = true;
//         }
//         else
//         {
//             arr[2] = true;
//         }
//     }
//     else
//     {
//         if (x > z)
//         {
//             arr[0] = true;
//         }
//         else
//         {
//             arr[2] = true;
//         }
//     }
// }

#include "bounding_volume_hierarchy.h"
#include "draw.h"

AxisAlignedBox getRootBoundingBox(std::vector<Mesh> &meshes);
void sortTrianglesByCentres(std::vector<Triangle> &triangles, Mesh &onlyMesh, int longestAxis);
AxisAlignedBox getBoundingBoxFromMeshes(std::vector<Mesh> &meshes);

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene *pScene)
    : m_pScene(pScene)
{
    std::vector<Mesh> meshes = pScene->meshes;
    AxisAlignedBox rootAABB = getBoundingBoxFromMeshes(meshes);

    Node root{
        (numLevels() == 0),
        0,
        rootAABB,
        {},
        meshes,
    };
    createTree(root);

    // as an example of how to iterate over all meshes in the scene, look at the intersect method below
}

void sortMeshesByCentres(std::vector<Mesh> &meshes, int longestAxis)
{
    std::sort(meshes.begin(), meshes.end(),
              [longestAxis](Mesh &m1, Mesh &m2) {
                  std::vector<Triangle> triangles1 = m1.triangles;
                  sortTrianglesByCentres(triangles1, m1, longestAxis); // triangles1 is now sorted
                  std::vector<Triangle> triangles2 = m2.triangles;
                  sortTrianglesByCentres(triangles2, m2, longestAxis); // triangles2 is now sorted

                  // get the middle triangle from the sorted triangles
                  Triangle middle1 = triangles1[triangles1.size() / 2];
                  Triangle middle2 = triangles2[triangles2.size() / 2];

                  glm::vec3 c1 = (m1.vertices[middle1[0]].p + m1.vertices[middle1[1]].p + m1.vertices[middle1[2]].p) / 3.0f;
                  glm::vec3 c2 = (m2.vertices[middle2[0]].p + m2.vertices[middle2[1]].p + m2.vertices[middle2[2]].p) / 3.0f;

                  float coord1 = (longestAxis == 0) ? c1.x : ((longestAxis == 1) ? c1.y : (longestAxis == 2) ? c1.z : -1);
                  float coord2 = (longestAxis == 0) ? c2.x : ((longestAxis == 1) ? c2.y : (longestAxis == 2) ? c2.z : -1);

                  return coord1 < coord2;
              });
}

void sortTrianglesByCentres(std::vector<Triangle> &triangles, Mesh &onlyMesh, int longestAxis)
{
    std::sort(triangles.begin(), triangles.end(),
              [&onlyMesh, longestAxis](const auto &t1, const auto &t2) {
                  glm::vec3 c1 = (onlyMesh.vertices[t1[0]].p + onlyMesh.vertices[t1[1]].p + onlyMesh.vertices[t1[2]].p) / 3.0f;
                  glm::vec3 c2 = (onlyMesh.vertices[t2[0]].p + onlyMesh.vertices[t2[1]].p + onlyMesh.vertices[t2[2]].p) / 3.0f;

                  float coord1 = (longestAxis == 0) ? c1.x : ((longestAxis == 1) ? c1.y : (longestAxis == 2) ? c1.z : -1);
                  float coord2 = (longestAxis == 0) ? c2.x : ((longestAxis == 1) ? c2.y : (longestAxis == 2) ? c2.z : -1);

                  return coord1 < coord2;
              });
}

void getVerticesFromTriangles(std::vector<Vertex> &vertices, std::vector<Triangle> &triangles, Mesh onlyMesh)
{
    for (Triangle t : triangles)
    {
        vertices.push_back(onlyMesh.vertices[t[0]]);
        vertices.push_back(onlyMesh.vertices[t[1]]);
        vertices.push_back(onlyMesh.vertices[t[2]]);
    }
}

void getChildMeshesMultipleMeshes(std::vector<Mesh> &leftChild, std::vector<Mesh> &rightChild,
                                  std::vector<Mesh> meshesCopy, int longestAxis)
{

    sortMeshesByCentres(meshesCopy, longestAxis);

    // split the meshes for the 2 child nodes
    // note: the middle element is always assigned to the left child
    std::vector<Mesh> left(meshesCopy.begin(), meshesCopy.begin() + meshesCopy.size() / 2);
    std::vector<Mesh> right(meshesCopy.begin() + meshesCopy.size() / 2, meshesCopy.end());
    leftChild = left;
    rightChild = right;
}

void getChildMeshesOneMesh(std::vector<Mesh> &leftChild, std::vector<Mesh> &rightChild, Mesh &onlyMesh, int longestAxis)
{
    std::vector<Triangle> triangles = onlyMesh.triangles; // copy of the triangles
    std::vector<Vertex> &allVertices = onlyMesh.vertices;
    sortTrianglesByCentres(triangles, onlyMesh, longestAxis);

    // split the triangles for the 2 child nodes
    // note: the middle element is always assigned to the left child
    std::vector<Triangle> leftTriangles(triangles.begin(), triangles.begin() + triangles.size() / 2);
    std::vector<Triangle> rightTriangles(triangles.begin() + triangles.size() / 2, triangles.end());

    leftChild.push_back(Mesh{allVertices, leftTriangles, onlyMesh.material});
    rightChild.push_back(Mesh{allVertices, rightTriangles, onlyMesh.material});
}

int BoundingVolumeHierarchy::numLevels() const
{
    return 10;
}

AxisAlignedBox getBoundingBoxFromMeshes(std::vector<Mesh> &meshes)
{

    float firstTriangleVertex = meshes[0].triangles[0].x;
    //                                  modify vvvvvv if it does not work!!!
    float max_x = meshes[0].vertices[firstTriangleVertex].p.x;
    float max_y = meshes[0].vertices[firstTriangleVertex].p.y;
    float max_z = meshes[0].vertices[firstTriangleVertex].p.z;

    float min_x = meshes[0].vertices[firstTriangleVertex].p.x;
    float min_y = meshes[0].vertices[firstTriangleVertex].p.y;
    float min_z = meshes[0].vertices[firstTriangleVertex].p.z;

    for (Mesh mesh : meshes)
    {
        std::vector<Triangle> &meshTriangles = mesh.triangles;
        for (Triangle t : meshTriangles)
        {
            for (int i = 0; i < 3; i++)
            {
                Vertex current = mesh.vertices[(i == 0) ? t.x : ((i == 1) ? t.y : t.z)];
                glm::vec3 p = current.p;
                min_x = (p.x < min_x) ? p.x : min_x;
                min_y = (p.y < min_y) ? p.y : min_y;
                min_z = (p.z < min_z) ? p.z : min_z;

                max_x = (p.x > max_x) ? p.x : max_x;
                max_y = (p.y > max_y) ? p.y : max_y;
                max_z = (p.z > max_z) ? p.z : max_z;
            }
        }
    }
    return AxisAlignedBox{glm::vec3{min_x, min_y, min_z}, glm::vec3{max_x, max_y, max_z}};
}

void BoundingVolumeHierarchy::getSubNodes(Node &node)
{
    if (node.meshes.size() == 1)
    {
        if (node.meshes[0].triangles.size() == 1)
        {
            node.isLeaf = true;
            return;
        }
    }
    glm::vec3 mins = node.AABB.lower;
    glm::vec3 maxs = node.AABB.upper;
    float x = maxs.x - mins.x;
    float y = maxs.y - mins.y;
    float z = maxs.z - mins.z;
    int longestAxis = (x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2);

    // --- TODO List --- //
    // we want to sort the triangles based on their centers to get the median triangle --> done
    // after that, devide the vector of triangles into two vectors --> done
    // fill createTree method to construct tree of those nodes.
    // make node a referance. --> done
    // implement dividing the node when it contains multiple meshes
    // create the two subnodes in getSubNodes (this function)
    // check the passing by value/reference

    std::vector<Mesh> leftChild;
    std::vector<Mesh> rightChild;
    AxisAlignedBox AABB_left;
    AxisAlignedBox AABB_right;

    if (node.meshes.size() > 1)
    {
        //std::cout << node.meshes.size() << std::endl;

        // divide the meshes into groups
        //std::cout << node.meshes.size() << std::endl;
        //std::cout << right.size() << " " << rightChild.size() << std::endl;
        getChildMeshesMultipleMeshes(leftChild, rightChild, node.meshes, longestAxis);
        //std::cout << node.meshes.size() << " " << leftChild.size() << " " << rightChild.size() << std::endl;
    }
    else
    {
        //std::cout << "hi " << std::endl;

        Mesh onlyMesh = node.meshes[0];
        // onlyMesh, leftChild and rightChild are all passed by reference
        getChildMeshesOneMesh(leftChild, rightChild, onlyMesh, longestAxis);
    }

    AABB_left = getBoundingBoxFromMeshes(leftChild);
    AABB_right = getBoundingBoxFromMeshes(rightChild);
    Node leftNode;
    Node rightNode;

    bool areLeaf = (node.level + 1 == numLevels());

    std::vector<Node> children;
    children.push_back(Node{areLeaf, node.level + 1, AABB_left, {}, leftChild});
    children.push_back(Node{areLeaf, node.level + 1, AABB_right, {}, rightChild});
    node.subTree = children;
}

// recursive function that will create the whole tree
// this method will get a node and create/return its subtrees
void BoundingVolumeHierarchy::createTree(Node &node)
{

    nodes.push_back(node);
    // ending condition is when the node is leaf.
    if (node.isLeaf)
    {
        return;
    }
    else
    {
        // make the recursive call
        getSubNodes(node);

        for (Node subNode : node.subTree)
        {
            createTree(subNode);
        }
    }
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

void getNodesAtLevel(Node &node, std::vector<AxisAlignedBox> &result, int level)
{

    if (node.level == level)
    {
        result.push_back(node.AABB);
        return;
    }
    if (node.isLeaf)
    {
        return;
    }

    for (Node child : node.subTree)
    {
        getNodesAtLevel(child, result, level);
    }
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
    // AxisAlignedBox aabb = nodes[nodes.size() - 1].AABB;
    glm::vec3 color = glm::vec3(0.05f, 1.0f, 0.05f);
    //glm::vec3(1.0f, 1.0f, 1.0f);
    // for (Node node : nodes)
    // {
    //     drawAABB(node.AABB, DrawMode::Filled, color, 1);
    //     //color -= 0.1f;
    // }
    // std::vector<AxisAlignedBox> results;
    // getNodesAtLevel(nodes[0], results, level);

    //std::cout << nodes.size() << std::endl;

    // std::cout << results.size() << std::endl;
    // for (AxisAlignedBox AABB : results)
    // {
    //     drawAABB(AABB, DrawMode::Filled, color, 1);
    // }
    //int howManyTimes = 0;
    for (Node n : nodes)
    {
        if (n.level == level)
        {
            //howManyTimes++;
            drawAABB(n.AABB, DrawMode::Filled, color, 0.9);
        }
    }
    //std::cout << howManyTimes << std::endl;
    //drawAABB(aabb, DrawMode::Wireframe);
    //drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 1);
}

bool intersectRecursive(Ray& ray, HitInfo& hitInfo, const Node& current) {
    AxisAlignedBox AABB = current.AABB;

    float originalT = ray.t;
    if (current.isLeaf) {
        for (const auto& mesh : current.meshes)
        {
            for (const auto& tri : mesh.triangles)
            {
                const auto v0 = mesh.vertices[tri[0]];
                const auto v1 = mesh.vertices[tri[1]];
                const auto v2 = mesh.vertices[tri[2]];
                if (intersectRayWithTriangle(v0.p, v1.p, v2.p, ray, hitInfo))
                {
                    hitInfo.material = mesh.material;
                    return true;
                }
            }
        }
        return false;
    }

    std::vector<float> t;
    std::vector<Node> intersected;

    for (const Node& child : current.subTree) {
        if (intersectRayWithShape(child.AABB, ray)) {
            t.push_back(ray.t);
            intersected.push_back(child);
            ray.t = originalT;
        }
    }

    if (t.size() == 0) {
        return false;
    }
    else if (t.size() == 1) {
        return intersectRecursive(ray, hitInfo, intersected[0]);
    }
    else if (t[0] < t[1]) {
        if (intersectRecursive(ray, hitInfo, intersected[0])) {
            return true;
        }
        return intersectRecursive(ray, hitInfo, intersected[1]);
    }
    else {
        if (intersectRecursive(ray, hitInfo, intersected[1])) {
            return true;
        }
        return intersectRecursive(ray, hitInfo, intersected[0]);
    }
}

bool intersectDataStructure (Ray &ray, HitInfo &hitInfo, const Node &root) {
    AxisAlignedBox AABB = root.AABB;

    float originalT = ray.t;
    if (intersectRayWithShape(AABB, ray)) {
        ray.t = originalT;
        return intersectRecursive(ray, hitInfo, root);
    }

    return false;
}

// Return true if something is hit, returns false otherwise. Only find hits if they are closer than t stored
// in the ray and if the intersection is on the correct side of the origin (the new t >= 0). Replace the code
// by a bounding volume hierarchy acceleration structure as described in the assignment. You can change any
// file you like, including bounding_volume_hierarchy.h .
bool BoundingVolumeHierarchy::intersect(Ray &ray, HitInfo &hitInfo) const
{
    bool hit = false;
    //// Intersect with all triangles of all meshes.
    //for (const auto &mesh : m_pScene->meshes)
    //{
    //    for (const auto &tri : mesh.triangles)
    //    {
    //        const auto v0 = mesh.vertices[tri[0]];
    //        const auto v1 = mesh.vertices[tri[1]];
    //        const auto v2 = mesh.vertices[tri[2]];
    //        if (intersectRayWithTriangle(v0.p, v1.p, v2.p, ray, hitInfo))
    //        {
    //            hitInfo.material = mesh.material;
    //            hit = true;
    //        }
    //    }
    //}
    // Intersect with spheres.
    hit = intersectDataStructure(ray, hitInfo, nodes[0]);
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

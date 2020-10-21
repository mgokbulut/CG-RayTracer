#include "bounding_volume_hierarchy.h"
#include "draw.h"

AxisAlignedBox getRootBoundingBox(std::vector<Mesh> &meshes);
void sortTrianglesByCentres(std::vector<Triangle> &triangles, Mesh &onlyMesh, int longestAxis);
AxisAlignedBox getBoundingBoxFromMeshes(std::vector<Mesh> &meshes);

/**
 * Constructor for the bvh. 
 * 
 * Create the root bounding box that will contain all the meshes, 
 * use it to create the root node and call createTree to
 * recursively create the rest of the nodes. 
 * 
 * @param *pScene Scene pointer with all relevant information for this scene
 */
BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene *pScene)
    : m_pScene(pScene)
{
    //--- TODO ---//
    // implement the division criteria with SAH+binning (in case)
    // 
    
    std::vector<Mesh> meshes = pScene->meshes;
    AxisAlignedBox rootAABB = getBoundingBoxFromMeshes(meshes);

    root = Node {
        (numLevels() == 0),
        0,
        rootAABB,
        {},
        meshes,
    };
    createTree(root);
}

/**
 * Sort multiple meshes by their centres. 
 * 
 * The "centre" of a mesh is the centre of its middle triangle
 * --> use the sortTrianglesByCentres to first sort the triangles inside meshes. 
 * We always only care about the coordinate defined by longestAxis. 
 * 
 * @param &meshes std::vector reference to the meshes of a node
 * @param longestAxis int deciding which axis to sort by
 */
void sortMeshesByCentres(std::vector<Mesh> &meshes, int longestAxis)
{
    std::sort(meshes.begin(), meshes.end(),
              [longestAxis](Mesh &m1, Mesh &m2) {
                  std::vector<Triangle> triangles1 = m1.triangles;
                  sortTrianglesByCentres(triangles1, m1, longestAxis); // triangles1 is now sorted
                  std::vector<Triangle> triangles2 = m2.triangles;
                  sortTrianglesByCentres(triangles2, m2, longestAxis); // triangles2 is now sorted

                  // get the middle triangle from the sorted triangles of this mesh
                  Triangle middle1 = triangles1[triangles1.size() / 2];
                  Triangle middle2 = triangles2[triangles2.size() / 2];

                  // get the centres of these middle triangles
                  glm::vec3 c1 = (m1.vertices[middle1[0]].p + m1.vertices[middle1[1]].p + m1.vertices[middle1[2]].p) / 3.0f;
                  glm::vec3 c2 = (m2.vertices[middle2[0]].p + m2.vertices[middle2[1]].p + m2.vertices[middle2[2]].p) / 3.0f;

                  float coord1 = (longestAxis == 0) ? c1.x : ((longestAxis == 1) ? c1.y : (longestAxis == 2) ? c1.z : -1);
                  float coord2 = (longestAxis == 0) ? c2.x : ((longestAxis == 1) ? c2.y : (longestAxis == 2) ? c2.z : -1);

                  return coord1 < coord2;
              });
}

/**
 * Sort triangles by their centres. 
 * 
 * The centre of a triangle is the average of its 3 vertices. 
 * We always only care about the coordinate defined by longestAxis. 
 * 
 * @param &triangles std::vector reference to the triangles of a mesh
 * @param &onlyMesh Mesh reference to the mesh of which we want to sort the triangles
 * @param longestAxis int deciding which axis to sort by
 */
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

/**
 * !Not used! Get a list of vertices from triangles. 
 * 
 * Because triangles store indices for their vertices, 
 * getting the vertices of a mesh defined only by triangles is annoying.
 * 
 * @param &vertices std::vector reference to vertices
 * @param &triangles std::vector reference to triangles
 * @param onlyMesh Mesh needed to retrieve all the vertices of the scene (should maybe be a reference?)
 */
void getVerticesFromTriangles(std::vector<Vertex> &vertices, std::vector<Triangle> &triangles, Mesh onlyMesh)
{
    for (Triangle t : triangles)
    {
        vertices.push_back(onlyMesh.vertices[t[0]]);
        vertices.push_back(onlyMesh.vertices[t[1]]);
        vertices.push_back(onlyMesh.vertices[t[2]]);
    }
}

/**
 * Split meshes into two groups for a node with multiple meshes. 
 * 
 * When a node contains multiple meshes, these meshes must be split
 * into two groups based on their centres (see sortMeshesByCentres).
 * 
 * @param &leftChild std::vector reference of meshes to add to the left child of this node
 * @param &rightChild std::vector reference of meshes to add to the right child of this node
 * @param meshesCopy std::vector copy of the meshes from the parent
 *  --> must be a copy because we will be sorting it
 * @param longestAxis int determining the axis which will be split
 */
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

/**
 * Split a mesh into two meshes for a node with a single mesh. 
 *
 * When a node contains a single mesh, this mesh must be split into two new
 * meshes by sorting and splitting the meshe's triangles. 
 *
 * @param &leftChild std::vector reference of the mesh to add to the left child of this node
 * @param &rightChild std::vector reference of the mesh to add to the right child of this node
 * @param &onlyMesh Mesh reference to the parent mesh
 * @param longestAxis int determining the axis which will be split
 */
void getChildMeshesOneMesh(std::vector<Mesh> &leftChild, std::vector<Mesh> &rightChild, Mesh &onlyMesh, int longestAxis)
{
    // COPY of the triangles - we will be sorting them in sortTrianglesByCentres
    std::vector<Triangle> triangles = onlyMesh.triangles;
    std::vector<Vertex> &allVertices = onlyMesh.vertices;
    sortTrianglesByCentres(triangles, onlyMesh, longestAxis);

    // split the triangles for the 2 child nodes
    // note: the middle element is always assigned to the left child
    std::vector<Triangle> leftTriangles(triangles.begin(), triangles.begin() + triangles.size() / 2);
    std::vector<Triangle> rightTriangles(triangles.begin() + triangles.size() / 2, triangles.end());

    // create meshes from the triangles that were split and append them to the vector references
    leftChild.push_back(Mesh{allVertices, leftTriangles, onlyMesh.material});
    rightChild.push_back(Mesh{allVertices, rightTriangles, onlyMesh.material});
}

/**
 * Give the maximum number of levels of the bvh tree.
 * 
 * @return int number of levels
 */
int BoundingVolumeHierarchy::numLevels() const
{
    return 10;
}

/**
 * Create a bounding box from meshes. 
 * 
 * Traverse all the meshes, all their triangles and the three vertices of each triangle
 * to determine the min and max values for each coordinate.
 * 
 * @param &meshes std::vector reference to the meshes of a node
 * @return an AABB for the inputted meshes
 */
AxisAlignedBox getBoundingBoxFromMeshes(std::vector<Mesh> &meshes)
{
    // the vertex of the first triangle of the first mesh
    float firstTriangleVertex = meshes[0].triangles[0].x;

    // min and max values for each coordinate will
    // initially be the coordinates of the first point
    float min_x, max_x, min_y, max_y, min_z, max_z;
    min_x = max_x = meshes[0].vertices[firstTriangleVertex].p.x;
    min_y = max_y = meshes[0].vertices[firstTriangleVertex].p.y;
    min_z = max_z = meshes[0].vertices[firstTriangleVertex].p.z;

    for (Mesh &mesh : meshes)
    {
        std::vector<Triangle> &meshTriangles = mesh.triangles;
        for (Triangle &t : meshTriangles)
        {
            // traverse the three vertices of a triangle
            for (int i = 0; i < 3; i++)
            {
                Vertex &current = mesh.vertices[(i == 0) ? t.x : ((i == 1) ? t.y : t.z)];
                glm::vec3 &p = current.p;
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

/**
 * Create two subnodes for this node iff it is not a leaf.
 * 
 * The function differentiates between a node with a single mesh
 * and a node with multiple meshes to be able to split the meshes
 * and triangles accordingly.
 * 
 * @param &node Node reference to the node from which we are getting
 * AND to which we are adding the children
 */
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

    // determine the longest axis by which we will be splitting
    // by taking the bounding box from the parent node
    glm::vec3 mins = node.AABB.lower;
    glm::vec3 maxs = node.AABB.upper;
    float x = maxs.x - mins.x;
    float y = maxs.y - mins.y;
    float z = maxs.z - mins.z;
    int longestAxis = (x > y) ? ((x > z) ? 0 : 2) : ((y > z) ? 1 : 2);

    // --- TODO List --- //
    // check the passing by value/reference
    
    // Create the fields that will be passed by reference to
    // and modified by other functions
    std::vector<Mesh> leftChild, rightChild;
    AxisAlignedBox AABB_left, AABB_right;

    if (node.meshes.size() > 1)
    {
        //std::cout << node.meshes.size() << std::endl;

        // divide the meshes into groups
        //std::cout << node.meshes.size() << std::endl;
        //std::cout << right.size() << " " << rightChild.size() << std::endl;

        // leftChild and rightChild are passed by reference
        getChildMeshesMultipleMeshes(leftChild, rightChild, node.meshes, longestAxis);
        //std::cout << node.meshes.size() << " " << leftChild.size() << " " << rightChild.size() << std::endl;
    }
    else
    {
        //std::cout << "hi " << std::endl;

        // leftChild, rightChild and onlyMesh are all passed by reference
        Mesh onlyMesh = node.meshes[0];
        getChildMeshesOneMesh(leftChild, rightChild, onlyMesh, longestAxis);
    }

    AABB_left = getBoundingBoxFromMeshes(leftChild);
    AABB_right = getBoundingBoxFromMeshes(rightChild);
    Node leftNode, rightNode;

    bool areLeaf = (node.level + 1 == numLevels());

    std::vector<Node> children;
    children.push_back(Node{areLeaf, node.level + 1, AABB_left, {}, leftChild});
    children.push_back(Node{areLeaf, node.level + 1, AABB_right, {}, rightChild});
    node.subTree = children;
}

/**
 * Recursive function creating the whole tree. 
 * 
 * In one call, the function will add the node to the vector of nodes
 * belonging to this class, get the subnodes of this node and call itself
 * for the two children created (iff it is not a leaf) 
 * --> in the foreach loop, the node must be a reference!!
 * 
 * @param &node Node reference to a node from an incomplete tree
 */
void BoundingVolumeHierarchy::createTree(Node &node)
{
    // ending condition is when the node is leaf.
    if (node.isLeaf)
    {
        return;
    }
    else
    {
        getSubNodes(node);

        // make the recursive call
        for (Node &subNode : node.subTree)
        {
            createTree(subNode);
        }
    }
}

/**
 * !Not used! Get the bounding box of the root node. 
 * 
 * In other words, get the bounding box that will contain all the triangles of this scene.
 * 
 * @param &meshes std::vector reference to all meshes in the scene
 * @return AABB AxisAlignedBox containing all the triangles
 */
AxisAlignedBox getRootBoundingBox(std::vector<Mesh> &meshes)
{
    float min_x, max_x, min_y, max_y, min_z, max_z;
    min_x = max_x = meshes[0].vertices[0].p.x;
    min_y = max_y = meshes[0].vertices[0].p.y;
    min_z = max_z = meshes[0].vertices[0].p.z;

    for (Mesh &mesh : meshes)
    {
        std::vector<Vertex> &vertices = mesh.vertices;

        for (Vertex &vertex : vertices)
        {
            glm::vec3 &p = vertex.p;
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

/**
 * !Not used and probably not working! Recursively get all bounding boxes at a certain level (e.g. 0 = only the root AABB). 
 * 
 * Breadth-first search through a tree.
 * 
 * @param &node Node reference to a node in the tree
 * @param &result std::vector reference to the resulting vector of AABBs
 * @param level int of the level we want to retrieve
 */
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

    for (Node &child : node.subTree)
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

    std::vector<AxisAlignedBox> result;
    getNodesAtLevel(root, result, level);

    for (AxisAlignedBox AABB : result) {
        drawAABB(AABB, DrawMode::Filled, color, 0.8f);
    }
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
    //for (Node &n : nodes)
    //{
    //    if (n.level == level)
    //    {
    //        //howManyTimes++;
    //        drawAABB(n.AABB, DrawMode::Filled, color, 0.9);
    //    }
    //}
    //std::cout << howManyTimes << std::endl;
    //drawAABB(aabb, DrawMode::Wireframe);
    //drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 1);
}

/**
 * Recursively traverse the tree and see if a given ray intersects the structure or not. 
 * 
 * Should be split into two functions for a better readability!!!
 * 
 * @param &ray reference to the currently shot ray
 * @param &hitInfo reference to HitInfo
 * @param &current reference to the node we are currently at
 * @return intersected bool stating whether some triangle was intersected or not
 */
bool intersectRecursive(Ray &ray, HitInfo &hitInfo, const Node &current) {
    AxisAlignedBox AABB = current.AABB;

    float originalT = ray.t; // CANNOT be a reference!!
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

/**
 * Initial method of a ray-triangle intersection using a data structure. 
 * 
 * Check whether the root AABB was intersected and if that is the case, 
 * call the recursive intersection function.
 * 
 * @param &ray Ray reference to the currently shot ray
 * @param &hitInfo HitInfo reference of the current ray
 * @param &root Node reference to the root node
 * @return intersected bool stating whether the root AABB was intersected or not
 */
bool intersectDataStructure (Ray &ray, HitInfo &hitInfo, const Node &root) {
    AxisAlignedBox AABB = root.AABB;

    float originalT = ray.t; // CANNOT be a reference
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
    hit = intersectDataStructure(ray, hitInfo, root);
    for (const auto &sphere : m_pScene->spheres)
        hit |= intersectRayWithShape(sphere, ray, hitInfo);
    return hit;
}

 //void _max_(float x, float y, float z, bool (&arr)[3])
 //{
 //    if (x < y)
 //    {
 //        if (y > z)
 //        {
 //            arr[1] = true;
 //        }
 //        else
 //        {
 //            arr[2] = true;
 //        }
 //    }
 //    else
 //    {
 //        if (x > z)
 //        {
 //            arr[0] = true;
 //        }
 //        else
 //        {
 //            arr[2] = true;
 //        }
 //    }
 //}

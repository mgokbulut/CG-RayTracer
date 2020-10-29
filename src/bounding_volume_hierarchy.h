#pragma once
#include "ray_tracing.h"
#include "scene.h"
#include <iostream>

struct Node
{
    bool isLeaf;
    int level;
    AxisAlignedBox AABB;
    std::vector<int> indices;
    std::vector<Mesh> meshes;
};

class BoundingVolumeHierarchy
{

private:
    Scene *m_pScene;

    std::vector<Node> nodes;
    //Node root;
    void getSubNodes(Node &node, Node &leftNode, Node &rightNode);
    void createTree(Node root);

    Node getLeftChild(Node &node);
    Node getRightChild(Node &node);
    void setTriangleIndices(Node& node);
    void getNodesAtLevel(Node& node, std::vector<Node>& result, int level);

    //bool intersectRayThatStartsOutsideBoxes(Ray& ray, HitInfo& hitInfo, const Node& leftChild,
    //    const Node& rightChild, float& tLeft, float& tRight);
    //bool intersectDeeper(Ray& ray, HitInfo& hitInfo, const Node& leftChild, const Node& rightChild, float& tLeft, float& tRight);
    //bool intersectNonLeaf(Ray& ray, HitInfo& hitInfo, const Node& current);
    //bool intersectRecursive(Ray& ray, HitInfo& hitInfo, const Node& current);
    //bool intersectDataStructure(Ray& ray, HitInfo& hitInfo, const Node& root);
public:
    BoundingVolumeHierarchy(Scene *pScene);

    // Use this function to visualize your BVH. This can be useful for debugging.
    void debugDraw(int level);
    int numLevels() const;

    // Return true if something is hit, returns false otherwise.
    // Only find hits if they are closer than t stored in the ray and the intersection
    // is on the correct side of the origin (the new t >= 0).
    bool intersect(Ray &ray, HitInfo &hitInfo) const;

    // void addToNodes(const Node &node)
    // {
    //     nodes.push_back(node);
    // }

    //std::vector<Axis>
};

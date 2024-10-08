#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <glm.hpp>
#include <cstdlib>
#include <vector>
#include <map>

#include "TileManager.h"

class Node
{
public:
    int gValue; //movement
    int hValue; //heuristic
    int fValue; //total cost

    glm::vec2 nodePosition;

    Node *parentNode;
    Node *endNode;

    Node()
    {
        parentNode = nullptr;
    }

    Node( glm::vec2 position, int gCost, Node *parent, Node *finish)
    {
        nodePosition = position;

        setParent(parent);
        endNode = finish;

        if(endNode != nullptr)
        {
            gValue = gCost;
            hValue = findHValue();
            fValue = gValue + hValue;
        }
    }

    Node* getParent()
    {
        return parentNode;
    }

    void setParent(Node *node)
    {
        parentNode = node;
    }

    int findHValue()
    {


        float a = (endNode->nodePosition.x - nodePosition.x);
        float b = (endNode->nodePosition.y - nodePosition.y);
        float a1 = std::max(abs(a), abs(b)) / 16;
        float a2 = std::min(abs(a), abs(b)) / 16;
      //  float c = a1 + 0 * a2;
        float c = (abs(a) + abs(b)) / 16 ;
        return (int)c;
    }

    bool operator<(const Node& aNode) const
    {
        if(fValue < aNode.fValue)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator>(const Node& aNode) const
    {
        if(fValue > aNode.fValue)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool operator==(const Node& aNode) const
    {
        if(nodePosition == aNode.nodePosition)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

};

class PathFinder
{
public:
    std::vector<glm::ivec2> findPath(const glm::vec2& start, const glm::vec2& end, int range, int movementType);

protected:
private:
    static const int ORTHOGONAL_COST = 10;
    static const int DIAGONAL_COST = 15;

    int moveType;

    std::vector<Node> openSet;
    std::unordered_map<glm::vec2, bool, vec2Hash> nodeStatus;
    std::unordered_map<glm::vec2, int, vec2Hash> nodeFCost;
   /* std::map<glm::vec2, bool, classcomp> nodeStatus;
    std::map<glm::vec2, int, classcomp> nodeFCost;*/

    void addToOpenSet(Node* node);
    void removeFromOpenList();
    void findChildren(Node *currentNode);
    void addChild(const glm::vec2& position, int cost, Node *parent);
};

#endif // PATHFINDER_H

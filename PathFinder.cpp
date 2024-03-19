#include "PathFinder.h"
#include <cmath>
#include <gtc/type_ptr.hpp>
#include<gtx/rotate_vector.hpp>
#include <list>
#include <iostream>
#include "Tile.h"

void PathFinder::addToOpenSet(Node *node)
{
    int position;
    openSet.push_back(*node);
    nodeStatus [node->nodePosition] = true;
    nodeFCost[node->nodePosition] = node->fValue;

    position = openSet.size() - 1;
    while(position != 0)
    {
        if(openSet[position].fValue < openSet[position / 2].fValue)
        {
            Node tempNode = openSet[position];
            openSet[position] = openSet[position/2];
            openSet[position/2] = tempNode;
            position/=2;
        }
        else
        {
            break;
        }
    }
}

void PathFinder::removeFromOpenList()
{
    int position = 1;
    int position2;
    Node temp;
    std::vector<Node>::iterator it=openSet.end() - 1;
    openSet[0] = openSet[openSet.size() - 1];
    openSet.erase(it);
    while(position < openSet.size())
    {
        position2 = position;
        if(2*position2+1 <=openSet.size())
        {
            if(openSet[position2 - 1].fValue > openSet[2*position2 - 1].fValue)
            {
                position = 2*position2;
            }
            if(openSet[position - 1].fValue >openSet[(2*position2+1) - 1].fValue)
            {
                position = 2*position2+1;
            }
        }
        else if(2*position2 <= openSet.size())
        {
            if(openSet[position2 - 1].fValue > openSet[(2*position2) - 1].fValue)
            {
                position = 2*position2;
            }
        }
        if(position2 != position)
        {
            temp = openSet[position2 -1];
            openSet[position2 -1] = openSet[position-1];
            openSet[position-1] = temp;
        }
        else
        {
            break;
        }
    }
}

std::vector<glm::ivec2> PathFinder::findPath(const glm::vec2& start, const glm::vec2& end, int range)
{
    std::vector<glm::ivec2> thePath;

    //Clear the Sets
    openSet.clear();
    nodeStatus.clear();
    nodeFCost.clear();
    std::vector <Node*> theNodes;

    Node endNode(end, 0, nullptr, nullptr);
    Node startNode(start, 0, nullptr, &endNode);
    //Add the start node to the open set. Right now it is the only node in the set
    addToOpenSet(&startNode);
    //While there are nodes in the open set to check
    while (openSet.size() > 0)
    {
        //Get the first node from the set, which should be the one with the lowest f value

        //Set a new node equal to the first node in the set
        Node* currentNode = new Node();
        *currentNode = openSet[0];
        theNodes.push_back(currentNode);
        //if the node with the lowest f cost is the end node
        if (*currentNode == endNode)
        {
            //Need the full path so I know the optimal path, but then I need to only allow travel up to the max move of the unit
            while (currentNode->gValue > range)
            {
                currentNode = currentNode->parentNode;
            }
            //In the case that the end of the path is occupied by another unit, we instead want to end on the next node on the path
            //I am not entirely sure if this is the best way of handling this, nor am I sure how frequent an occurance this is.
            //At the least, this will prevent two approaching enemies from occupying the same space.
            bool blocked = true;
            while (blocked)
            {
                blocked = false;
                auto thisTile = TileManager::tileManager.getTile(currentNode->nodePosition.x, currentNode->nodePosition.y);
                if (thisTile)
                {
                    if (thisTile->occupiedBy)
                    {
                        currentNode = currentNode->parentNode;
                        blocked = true;
                    }
                }
            }
            //Trace the path back through the parent nodes. Since the start node has no parent, once the
            //parent node is equal to null, we know we have reached the start and can return a path.
            while (currentNode->parentNode != nullptr)
            {
                glm::vec2 pathPoint;
                pathPoint = currentNode->nodePosition;

                thePath.push_back(pathPoint);
                currentNode = currentNode->parentNode;
            }
            thePath.push_back(start);
            for (int i = 0; i < theNodes.size(); i++)
            {
                delete theNodes[i];
            }

            theNodes.clear();
            openSet.clear();
            nodeFCost.clear();
            nodeStatus.clear();
            return thePath;
        }
        //if the current node is not the end node.
        //remove the current node from the open set so it's not checked again.
        removeFromOpenList();
        //add the current node to the closed set so that it won't be added to the openSet again
        nodeStatus[currentNode->nodePosition] = false;
        //find any child nodes of the current node
        findChildren(currentNode);
    }

    openSet.clear();
    nodeFCost.clear();
    nodeStatus.clear();
    return thePath;
}

void PathFinder::findChildren(Node* currentNode)
{
    //get the position of the current node
    glm::vec2 position = currentNode->nodePosition;

    int tileSize = TileManager::TILE_SIZE;

    //left
    addChild(glm::vec2(position.x - tileSize, position.y), currentNode->gValue, currentNode);
    //right
    addChild(glm::vec2(position.x + tileSize, position.y), currentNode->gValue, currentNode);
    //down
    addChild(glm::vec2(position.x, position.y - tileSize), currentNode->gValue, currentNode);
    //up
    addChild(glm::vec2(position.x, position.y + tileSize), currentNode->gValue, currentNode);
}

void PathFinder::addChild(const glm::vec2& position, int cost, Node *parent)
{
    auto thisTile = TileManager::tileManager.getTile(position.x, position.y);
    if (thisTile)
    {
        //Want to treat tiles occupied by another team as impassable.
        //Tiles occupied by the same team are passable, so they can be added to the path, though they cannot be the final node
        //(that is handled above when returning the path)
        //Only the enemies are using this class at all right now, so I can just check for the player team. In future,
        //may want to pass in the unit so I can know what team to check against.
        //This is currently not in use, as I only use this class for enemies that are not in attacking range of player units,
        //Which means there won't be a case where there is a tile occupied by an unfriendly unit on their path anyway.
        bool cool = true;
       /* if (thisTile->occupiedBy && thisTile->occupiedBy->team == 0)
        {
            //Do need to be able to recognize the end node however
            glm::vec2 tilePos = glm::vec2(thisTile->x, thisTile->y);
            if (tilePos != position)
            {
                cool = false;
            }
        }*/
        if(cool)
        {
            //create a new node
            Node newNode(position, thisTile->properties.movementCost + cost, parent, parent->endNode);
            if (nodeStatus.count(position) == 1)
            {
                //check if the new node is in the closed set. If it is, return.
                if (nodeStatus[position] == false)
                {
                    return;
                }
                //check if the new node is in the open set
                if (nodeStatus[position] == true)
                {
                    //If the fvalue of the new node is greater or equal to the node already in the open list
                    //return
                    if (newNode.fValue >= nodeFCost[position])
                    {
                        return;
                    }
                    else
                    {
                        //nothing here
                        //ideally, the priority of the node would be reset to the new lower fvalue
                        //as I cannot think of a fast way to do this, the node ends up being checked twice

                    }

                }
            }
            //add the new node to the open set
            addToOpenSet(&newNode);
        }
    }
}
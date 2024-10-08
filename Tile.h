#ifndef TILE_H
#define TILE_H
#include <glm.hpp>
#include <string>
#include "Unit.h"

struct VisitObject
{
    //Key is the ID the unit that activates this visit, the value will be the index in the scene vector of the scene to play
    glm::ivec2 position;
    std::unordered_map<int, class Scene*> sceneMap;
    bool toDelete = false;
};

struct TileProperties
{
    //Not sure if this should be a string but it is for now
    std::string name;
    //Avoid bonus for units on this tile
    int avoid;
    //Defensive bonus for units on this tile
    int defense;
    //Cost of movement on this tile. Should have a value that means nothing can move on it
    int movementCost[3];
    //Need this for healing tiles, not sure how this will work though.
    int bonus;
    //The color displayed on the minimap. Want to move this to a separate structure I think
    glm::vec3 miniMapColor;

    TileProperties() {};
    TileProperties(std::string name, int avoid, int defense, int bonus, glm::vec3 miniMapColor, int footCost, int horseCost, int flyCost)
        : name(name), avoid(avoid), defense(defense), bonus(bonus), miniMapColor(miniMapColor)
    {
        movementCost[0] = footCost;
        movementCost[1] = horseCost; 
        movementCost[2] = flyCost;
    }
};

struct Tile
{
    //We'll see about this one
    int x;
    int y;
    int uvID;
    TileProperties properties;
    Unit* occupiedBy = nullptr;
    VisitObject* visitSpot = nullptr;
    class Vendor* vendor = nullptr;
    bool seizePoint = false;
};

#endif // TILE_H

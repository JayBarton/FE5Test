#ifndef TILE_H
#define TILE_H
#include <glm.hpp>
#include <string>
#include "Unit.h"
struct TileProperties
{
    //The tile type/sprite index
    int theType;
    //Not sure if this should be a string but it is for now
    std::string name;
    //Avoid bonus for units on this tile
    int avoid;
    //Defensive bonus for units on this tile
    int defense;
    //Cost of movement on this tile. Should have a value that means nothing can move on it
    int movementCost;
    //Need this for healing tiles, not sure how this will work though.
    int bonus;
};

struct Tile
{
    //We'll see about this one
    int x;
    int y;
    TileProperties properties;
    Unit* occupiedBy = nullptr;
};

#endif // TILE_H

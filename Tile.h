#ifndef TILE_H
#define TILE_H
#include <glm.hpp>
#include <string>

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
};

/*class Tile
{
private:

    //The attributes of the tile
  //  SDL_Rect box;
	glm::vec4 box;
    //The tile type/sprite index
    int theType;
    //Not sure if this should be a string but it is for now
    std::string name;
    //Defensive bonus for units on this tile
    int defense;
    //Avoid bonus for units on this tile
    int avoid;
    //Cost of movement on this tile. Should have a value that means nothing can move on it
    int movementCost;
    //Need this for healing tiles, not sure how this will work though.
    int bonus;

public:
    Tile();
    //Initializes the variables
    Tile( int x, int y, int tileWidth, int tileHeight, int tileType);

    //Get the tile type
    int getType();

    //Get the collision box
	glm::vec4 getBox();

    void setType(int newType);

};*/

#endif // TILE_H

#ifndef TILE_H
#define TILE_H
#include <glm.hpp>

class Tile
{
private:

    //The attributes of the tile
  //  SDL_Rect box;
	glm::vec4 box;
    //The tile type
    int theType;
    //the tile attribuite
    int theAttribuite;

public:


    Tile();
    //Initializes the variables
    Tile( int x, int y, int tileWidth, int tileHeight, int tileType);

    //Shows the tile
   // void Show(SDL_Rect *tile);

    //Get the tile type
    int getType();
    //get the tile attribuite
    int getAttribuite();

    //Get the collision box
	glm::vec4 getBox();

    //set a new tile attibuite overwriting the previous one
    void setAttribuite(int newAttribuite);
    void setType(int newType);
    bool isOccupied;

};

#endif // TILE_H

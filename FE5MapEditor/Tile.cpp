#include "Tile.h"


Tile::Tile()
{
}

Tile::Tile( int x, int y, int tileWidth, int tileHeight, int tileType)
{
    //Get the offsets
    box.x = x;
    box.y = y;

    //Set the collision box
    box.z = tileWidth;
    box.w = tileHeight;

    //Get the tile type and attribuite
    theType = tileType;

    isOccupied = false;
}

/*
void Tile::Show(SDL_Rect* tile)
{
    //Show the tile
    if(CollidedWithCamera(camera, box))
    {
        apply_surface( box.x - camera.x , box.y - camera.y, tileSheet, screen, tile );
    }
}
*/
void Tile::setType(int newType)
{
    theType = newType;
}

void Tile::setAttribuite(int newAttribuite)
{
    theAttribuite = newAttribuite;
}

int Tile::getType()
{
    return theType;
}

int Tile::getAttribuite()
{
    return theAttribuite;
}

glm::vec4 Tile::getBox()
{
    return box;
}



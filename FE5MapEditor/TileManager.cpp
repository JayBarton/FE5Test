#include "TileManager.h"
#include <fstream>
#include <cmath>
#include <string>
#include <sstream>

#include <iostream>

#include <gtc/matrix_transform.hpp>
#include "Camera.h"

TileManager TileManager::tileManager;

TileManager::TileManager()
{
//    clipTiles();
}

void TileManager::setUp(int width, int height)
{
    rowTiles = width;
    columnTiles = height;
    totalTiles = rowTiles * columnTiles;

    levelWidth = rowTiles * TILE_SIZE;
    levelHeight = columnTiles * TILE_SIZE;

    tiles = new Tile[totalTiles];
}

void TileManager::newMap(int width, int height)
{
    uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);

    setUp(width, height);

    //The tile offsets
    int startX = LEVEL_START_X ;
    int startY = LEVEL_START_Y;
    int x = startX;
    int y = startY;
    for(int i = 0; i < totalTiles; i ++)
    {
        //Determines what kind of tile will be made
        int tileType = 0;

        //If the number is a valid tile number

        tiles[i] = Tile(x, y, TILE_SIZE, TILE_SIZE, tileType);

        //Move to next tile spot
        x += TILE_SIZE;

        //If we've gone too far
        if (x >= levelWidth + startX)
        {
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
    }
}

void TileManager::setBackground(int backgroundTile)
{
    int startX = LEVEL_START_X ;
    int startY = LEVEL_START_Y;
    int x = startX;
    int y = startY;

    for(int i = 0; i < totalTiles; i ++)
    {
        //Determines what kind of tile will be made
        int tileType = backgroundTile;
        int layer = 1;

        //If the number is a valid tile number

        tiles[i] = Tile(x, y, TILE_SIZE, TILE_SIZE, tileType);

        //Move to next tile spot
        x += TILE_SIZE;

        //If we've gone too far
        if (x >= levelWidth + startX)
        {
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
    }
}

void TileManager::replaceTile(int oldTile, int newTile)
{
    int startX = LEVEL_START_X ;
    int startY = LEVEL_START_Y;
    int x = startX;
    int y = startY;

    for(int i = 0; i < totalTiles; i ++)
    {
        //Determines what kind of tile will be made
        int tileType = newTile;

        //If the number is a valid tile number

        //tiles[columns][rows] = new Tile( x, y, TILE_SIZE, TILE_SIZE, tileType);
        if(tiles[i].getType() == oldTile)
        {
            tiles[i] = Tile(x, y, TILE_SIZE, TILE_SIZE, newTile);
        }


        //Move to next tile spot
        x += TILE_SIZE;

        //If we've gone too far
        if (x >= levelWidth + startX)
        {
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
    }
}


bool TileManager::resizeMap(int newWidth, int newHeight)
{
    std::stringstream tileMap(saveTiles());

    clearTiles();

    if(newWidth == rowTiles && newHeight == columnTiles)
    {
        return false;
    }
    else if(newWidth < rowTiles || newHeight < columnTiles)
    {
        setUp(newWidth, newHeight);
        int startX = LEVEL_START_X ;
        int startY = LEVEL_START_Y;
        int x = startX;
        int y = startY;
        int oldWidth;
        int oldHeight;

        tileMap >> oldWidth >> oldHeight;

        int xOffset = oldWidth - newWidth;
        int yOffset = oldHeight - newHeight;

        tileMap.ignore(1);

        //Comment this out to shift up
        /*    for(int i = 0; i < yOffset; i ++)
            {
                for(int c = 0; c < oldWidth; c++)
                {
                    int tileType = 0;
                    tileMap.ignore(1);
                    tileMap >> tileType;
                    tileMap.ignore(2);
                }
            }*/


        int currentTile = 0;
        //swaping these two fors will shift the map down
        // for(int c = 0; c < newWidth; c++)
        for(int i = 0; i < newHeight; i ++)
        {
            for(int d = 0; d < xOffset; d++)
            {
                int tileType = 0;
                tileMap.ignore(1);
                tileMap >> tileType;
                tileMap.ignore(2);
            }
            //for(int i = 0; i < newHeight; i ++)
            for(int c = xOffset; c < oldWidth; c++)
            {
                //Determines what kind of tile will be made
                int tileType = 0;

                //Read tile from map file
                //ignore the first "
                tileMap.ignore(1);
                tileMap >> tileType;

                //ignore the second "
                tileMap.ignore(2);

                //If the number is a valid tile number
                if(tileType >= -1  &&  tileType < TILE_SPRITES)
                {
                    tiles[currentTile]= Tile( x, y, TILE_SIZE, TILE_SIZE, tileType);
                }
                //If we don't recognize the tile type
                else
                {
                    std::cout << "KOOGLER!\n";
                    std::cout << tileType << "\n";
                    //Stop loading map
                    return false;
                }

                //Move to next tile spot
                x += TILE_SIZE;
                currentTile ++;
            }
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
        //return false; //just make bigger for now
    }
    else
    {
        //probably don't need all of this in this if branch
        int xOffset = newWidth - rowTiles;
        int yOffset = newHeight - columnTiles;
        int oldWidth = rowTiles;
        int oldHeight = columnTiles;

        setUp(newWidth, newHeight);

        //The tile offsets
        int startX = LEVEL_START_X ;
        int startY = LEVEL_START_Y;
        int x = startX;
        int y = startY;

        tileMap >> oldWidth >> oldHeight;

        tileMap.ignore(1);

        int currentTile = 0;
        //swaping these two fors will shift the map down
        //for(int c = 0; c < oldWidth; c++)
        for(int i = 0; i < oldHeight; i ++)
        {
            // for(int i = 0; i < oldHeight; i ++)
            for(int c = 0; c < oldWidth; c++)
            {
                //Determines what kind of tile will be made
                int tileType = 0;

                //Read tile from map file
                //ignore the first "
                tileMap.ignore(1);
                tileMap >> tileType;

                //ignore the second "
                tileMap.ignore(2);

                //If there was a problem in reading the map
                /*     if( map.fail() == true )
                	{
                		//Stop loading map
                	 	map.close();
                	 	return false;
                 	}*/

                //If the number is a valid tile number
                if(tileType >= -1  &&  tileType < TILE_SPRITES)
                {
                    tiles[currentTile]= Tile( x, y, TILE_SIZE, TILE_SIZE, tileType);
                }
                //If we don't recognize the tile type
                else
                {
                    std::cout << "KOOGLER!\n";
                    std::cout << tileType << "\n";
                    //Stop loading map
                    return false;
                }

                //Move to next tile spot
                x += TILE_SIZE;
                currentTile ++;
            }
            for(int c = 0; c < xOffset; c ++)
            {
                int tileType = 0;
                tiles[currentTile] = Tile(x, y, TILE_SIZE, TILE_SIZE, tileType);
                x+= TILE_SIZE;
                currentTile++;
            }
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
        for(int i = 0; i < yOffset; i ++)
        {
            for(int c = 0; c < newWidth; c ++)
            {
                int tileType = 0;

                //If the number is a valid tile number

                tiles[currentTile] = Tile(x, y, TILE_SIZE, TILE_SIZE, tileType);

                //Move to next tile spot
                x += TILE_SIZE;

                //If we've gone too far
                if (x >= levelWidth + startX)
                {
                    //Move back
                    x = startX;

                    //Move to the next row
                    y += TILE_SIZE;
                }
                currentTile++;
            }
        }
    }
    return true;
}

bool TileManager::setTiles(std::ifstream& tileMap, int width, int height)
{
    uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);
    setUp(width, height);

    //The tile offsets
    int startX = LEVEL_START_X ;
    int startY = LEVEL_START_Y;
    int x = startX;
    int y = startY;

    std::string afe;
    tileMap.ignore(1);
    //tileMap >> afe;

    //std::cout << afe;
    //Initialize the tiles
    for(int i = 0; i < totalTiles; i ++)
    {
        //Determines what kind of tile will be made
        int tileType = 0;

        //Read tile from map file
        //ignore the first "
        tileMap.ignore(1);
        tileMap >> tileType;

        //ignore the second "
        tileMap.ignore(2);

        //If the number is a valid tile number
        if(tileType >= -1  &&  tileType < TILE_SPRITES)
        {
            tiles[i]= Tile( x, y, TILE_SIZE, TILE_SIZE, tileType );
        }
        //If we don't recognize the tile type
        else
        {
            std::cout << "KOOGLER!\n";
            std::cout << tileType << "\n";
            //Stop loading map
            return false;
        }

        //Move to next tile spot
        x += TILE_SIZE;

        //If we've gone too far
        if( x >= levelWidth + startX )
        {
            //Move back
            x = startX;

            //Move to the next row
            y += TILE_SIZE;
        }
    }
    return true;
}


/*void TileManager:: clipTiles()
{
    //Clip the sprite sheet
    for(int i = 0; i < totalTiles; i ++)
    {
        clips[i].x = 0;
        clips[i].y = i * TILE_SIZE;
        clips[i].w = TILE_SIZE;
        clips[i].h = TILE_SIZE;
    }
}*/

void TileManager::showTiles(SpriteRenderer * renderer, Camera& camera)
{
    for(int i = 0; i < totalTiles; i ++)
    {
        glm::vec4 box = tiles[i].getBox();
        if(camera.onScreen(glm::vec2(box.x, box.y), glm::vec2(box.z, box.w)))
        {
            glm::mat4 model = glm::mat4();
            glm::vec4 box = tiles[i].getBox();

            model = glm::translate(model, glm::vec3(box.x, box.y, 0.0f));
            model = glm::scale(model, glm::vec3(box.z, box.w, 0.0f));

            renderer->setUVs(uvs[tiles[i].getType()]);
            Texture2D displayTexture = ResourceManager::GetTexture("tiles");
            renderer->DrawSprite( displayTexture, glm::vec2(box.x, box.y), 0.0f, glm::vec2(box.z, box.w));

        }
    }
}


bool TileManager::outOfBounds(int x, int y)
{
    bool out = false;
    if(x < LEVEL_START_X  || x > levelWidth + TILE_SIZE || y < LEVEL_START_Y || y > levelHeight + TILE_SIZE)
    {
        out = true;
    }

    return out;
}

int TileManager::getType(int x, int y)
{
    return getTile(x, y)->getType();
}

int TileManager::xPositionToIndex(int x)
{
    return x/TILE_SIZE;
}

int TileManager::yPositionToIndex(int y)
{
    return y/TILE_SIZE;
}

Tile * TileManager::getTile(int x, int y)
{
    return &tiles[xPositionToIndex(x) + yPositionToIndex(y) * rowTiles];
}

glm::vec4 TileManager::getTileBox(int xPosition, int yPosition)
{
    return getTile(xPosition, yPosition)->getBox();
}

void TileManager::PositionToTile(int &xass, int &yass)
{
    float xTemp = xass;
    float yTemp = yass;
    xTemp /=TILE_SIZE;
    yTemp /= TILE_SIZE;
    xTemp = floor(xTemp + 0.5f);
    yTemp =floor (yTemp + 0.5f);
    xTemp *= TILE_SIZE;
    yTemp *= TILE_SIZE;
    yTemp *= TILE_SIZE;
    xass = xTemp;
    yass = yTemp;
}

void TileManager::placeTile(int x, int y, int type)
{
    Tile * t = getTile(x, y);
    t->setType(type);
}

std::string TileManager::saveTiles()
{
    //Open the map
    //std::ofstream map( "test.map" );
    std::stringstream map;
    map << rowTiles << " " << columnTiles << "\n";
    //Go through the tiles
    for(int i = 0; i < totalTiles; i ++)
    {
        //Write tile type to file
        //writing like this gives us the " X X" format
        map << "\"" << tiles[i].getType() << "\"" << " ";
    }

    return map.str();
    //map.close();
}

void TileManager::clearTiles()
{
    //Free the tiles
    for(int i = 0; i < totalTiles; i ++)
    {
        //   delete tiles[i];
    }
    delete [] tiles;
}

void TileManager::cleanUp()
{
    clearTiles();
}

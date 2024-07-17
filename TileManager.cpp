#include "TileManager.h"
#include <fstream>
#include <cmath>
#include <string>
#include <sstream>

#include <iostream>

#include <gtc/matrix_transform.hpp>
#include "Camera.h"
#include "Unit.h"
#include "Vendor.h"

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

    tileTypes.resize(10);
    tileTypes[0] = { "Plains", 0, 0, 1, 0, glm::vec3(0.321, 0.667, 0.321)};
    tileTypes[1] = { "Forest", 20, 2, 2, 0, glm::vec3(0.129, 0.411, 0.192) };
    tileTypes[2] = { "Thicket", 30, 5, 2, 0, glm::vec3(0.129, 0.411, 0.192) };
    tileTypes[3] = { "Road", 0, 0, 1, 0, glm::vec3(0.9, 0.63, 0.41) };
    tileTypes[4] = { "Cliff", 0, 0, 20, 0, glm::vec3(0.129, 0.411, 0.192) };
    tileTypes[5] = { "Peak", 40, 5, 20, 0, glm::vec3(0.388, 0.188, 0.03) };
    tileTypes[6] = { "--", 20, 5, 20, 0, glm::vec3(0.29, 0.286, 0.29)};
    tileTypes[7] = { "Gate", 30, 10, 1, 1, glm::vec3(1, 0.38, 0.709) };
    tileTypes[8] = { "Vendor", 10, 1, 1, 0, glm::vec3(1, 0.38, 0.709)};
    tileTypes[9] = { "House", 10, 1, 1, 0, glm::vec3(1, 0.38, 0.709)};

}

bool TileManager::setTiles(std::ifstream& tileMap, int width, int height)
{
    uvs = ResourceManager::GetTexture("tiles").GetUVs(TILE_SIZE, TILE_SIZE);
    setUp(width, height);

    //The tile offsets
    int startX = LEVEL_START_X;
    int startY = LEVEL_START_Y;
    int x = startX;
    int y = startY;

    std::string afe;
    tileMap.ignore(1);
    //tileMap >> afe;
    spriteBatch.init();
    spriteBatch.begin();
    Texture2D displayTexture = ResourceManager::GetTexture("tiles");
    //Initialize the tiles
    for (int i = 0; i < totalTiles; i++)
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
        if (tileType >= -1 && tileType < TILE_SPRITES)
        {
            int ID = -1;
            if (tileType < 70)
            {
                ID = 0;
            }
            else if (tileType < 78)
            {
                ID = 1;
            }
            else if (tileType < 89)
            {
                ID = 2;
            }
            else if (tileType < 108)
            {
                ID = 3;
            }
            else if (tileType < 125)
            {
                ID = 4;
            }
            else if (tileType < 154)
            {
                ID = 5;
            }
            else if (tileType < 181)
            {
                ID = 6;
            }
            else if (tileType < 182)
            {
                ID = 7;
            }
            else if (tileType < 183)
            {
                ID = 8;
            }
            else if (tileType < 184)
            {
                ID = 9;
            }
            auto properties = tileTypes[ID];
            tiles[i] = { x, y, tileType, properties };
            spriteBatch.addToBatch(displayTexture.ID, glm::vec2(x, y), TILE_SIZE, TILE_SIZE, uvs[tileType]);
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
    spriteBatch.end();
    return true;
}

void TileManager::showTiles(SpriteRenderer * renderer, Camera& camera)
{
    spriteBatch.renderBatch();
  /*  for (int i = 0; i < totalTiles; i++)
    {
        glm::vec4 box = glm::vec4(glm::vec2(tiles[i].x, tiles[i].y), glm::vec2(TILE_SIZE, TILE_SIZE));
        if(camera.onScreen(glm::vec2(box.x, box.y), glm::vec2(box.z, box.w)))
        {
            glm::mat4 model = glm::mat4();

            model = glm::translate(model, glm::vec3(box.x, box.y, 0.0f));
            model = glm::scale(model, glm::vec3(box.z, box.w, 0.0f));

            renderer->setUVs(uvs[tiles[i].properties.theType]);
            Texture2D displayTexture = ResourceManager::GetTexture("tiles");
            renderer->DrawSprite( displayTexture, glm::vec2(box.x, box.y), 0.0f, glm::vec2(box.z, box.w));

        }
    }*/
}

//Using this to redraw tiles when their uvs change, as is the case with visit tiles
//I don't know if this would be an acceptable solution if tiles were animated, but we don't currently have any that are.
void TileManager::reDraw()
{
    spriteBatch.init();
    spriteBatch.begin();
    Texture2D displayTexture = ResourceManager::GetTexture("tiles");
    for (int i = 0; i < totalTiles; i++)
    {
        int tileType = tiles[i].uvID;

        if (tileType >= -1 && tileType < TILE_SPRITES)
        {
            spriteBatch.addToBatch(displayTexture.ID, glm::vec2(tiles[i].x, tiles[i].y), TILE_SIZE, TILE_SIZE, uvs[tileType]);
        }
    }
    spriteBatch.end();
}

void TileManager::placeUnit(int x, int y, Unit* unit)
{
    getTile(x, y)->occupiedBy = unit;
}

void TileManager::removeUnit(int x, int y)
{
    getTile(x, y)->occupiedBy = nullptr;
}

void TileManager::placeVisit(int x, int y, VisitObject* visit)
{
    getTile(x, y)->visitSpot = visit;
}

void TileManager::placeVendor(int x, int y, Vendor* vendor)
{
    getTile(x, y)->vendor = vendor;
}

void TileManager::placeSeizePoint(int x, int y)
{
    getTile(x, y)->seizePoint = true;
}

bool TileManager::outOfBounds(int x, int y)
{
    bool out = false;
    if(x < TILE_SIZE || x >= levelWidth - TILE_SIZE || y < TILE_SIZE || y >= levelHeight - TILE_SIZE)
    {
        out = true;
    }

    return out;
}

//Not sure if this is neccessary
int TileManager::getType(int x, int y)
{
    return 1;
 //   return getTile(x, y)->properties.theType;
}

int TileManager::xPositionToIndex(int x)
{
    return x/TILE_SIZE;
}

int TileManager::yPositionToIndex(int y)
{
    return y/TILE_SIZE;
}
//Need error checking here
Tile * TileManager::getTile(int x, int y)
{
    if (outOfBounds(x, y))
    {
        return nullptr;
    }
    return &tiles[xPositionToIndex(x) + yPositionToIndex(y) * rowTiles];
}

Unit* TileManager::getUnit(int x, int y)
{
    if (Tile* tile = getTile(x, y))
    {
        return tile->occupiedBy;
    }
    return nullptr;
}

Unit* TileManager::getUnitOnTeam(int x, int y, int team)
{
    Unit* unit = getUnit(x, y);
    if (unit && unit->team == team)
    {
        return unit;
    }
    return nullptr;
}

VisitObject* TileManager::getVisit(int x, int y)
{
    if (Tile* tile = getTile(x, y))
    {
        return tile->visitSpot;
    }
    return nullptr;
}

Vendor* TileManager::getVendor(int x, int y)
{
    if (Tile* tile = getTile(x, y))
    {
        return tile->vendor;
    }
    return nullptr;
}

//Not sure if this function is even neccessary
glm::vec4 TileManager::getTileBox(int xPosition, int yPosition)
{
    return glm::vec4(xPosition, yPosition, TILE_SIZE, TILE_SIZE);
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

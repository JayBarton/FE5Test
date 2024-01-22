#ifndef TILEMANAGER_H
#define TILEMANAGER_H

#include "Tile.h"
#include <vector>
#include "Shader.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include <GL/glew.h>


#include <vector>
class Camera;
class TileManager
{
private :

    //Tile constants
    static const int TILE_SPRITES = 6;

    static const int LEVEL_START_X = 0;
    static const int LEVEL_START_Y = 0;

public :
	/*static const int TILE_WIDTH = 32;
	static const int TILE_HEIGHT = 32;*/
	static const int TILE_SIZE = 16;

    static TileManager tileManager;
    //the currently selected tile. maybe temporary.
    int currentType;

    int rowTiles;
    int columnTiles;
    int totalTiles;
    int levelWidth;
    int levelHeight;

    //the tiles
    Tile *tiles;
    //Tile *tiles [COLUMN_TILES][ROW_TILES];

    std::vector<glm::vec4> uvs;

    //initilize variables
    TileManager();

    //load and set the tiles
    bool setTiles(std::ifstream &tileMap, int width, int height);

    //check if a position is out of bounds
    bool outOfBounds(int x, int y);

    int xPositionToIndex(int x);
    int yPositionToIndex(int y);

	//probably not keeping this as I don't actually need it probably
	int getType(int x, int y);

	Tile * getTile(int x, int y);
    glm::vec4 getTileBox(int xPosition, int yPosition);

    void PositionToTile(int &x, int &y);

    void setUp(int width, int height);

    void showTiles(SpriteRenderer * renderer, Camera& camera);

    //called when the program stops running, clears the tiles
    void clearTiles();
    void cleanUp();

    std::vector<TileProperties> tileTypes;
};

#endif // TILEMANAGER_H

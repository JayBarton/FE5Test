#ifndef TILEMANAGER_H
#define TILEMANAGER_H

#include "Tile.h"

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

    //creates new blank map
    void newMap(int width, int height);
    //Sets all tiles to the passed in type for use as a background
    void setBackground(int tileType);
    void replaceTile(int oldTile, int newTile);
    //load and set the tiles
    bool setTiles(std::ifstream &tileMap, int width, int height);
    bool resizeMap(int newWidth, int newHeight);

    //check if a position is out of bounds
    bool outOfBounds(int x, int y);

    int xPositionToIndex(int x);
    int yPositionToIndex(int y);

	//probably not keeping this as I don't actually need it probably
	int getType(int x, int y);

	Tile * getTile(int x, int y);
    glm::vec4 getTileBox(int xPosition, int yPosition);

    void PositionToTile(int &x, int &y);

	void placeTile(int x, int y, int type);

	std::string saveTiles();

    void setUp(int width, int height);

    //get the tiles from a tile sheet
  //  void clipTiles();

  //  void clipAttribuites();

    //show the tiles on screen
    void showTiles(Shader shader, GLuint shapeVAO);
    void showTiles(SpriteRenderer * renderer, Camera& camera);


    //called when the program stops running, clears the tiles
    void clearTiles();
    void cleanUp();

};

#endif // TILEMANAGER_H

#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include <SDL.h>
#include <SDL_Image.h>
#include <SDL_mixer.h>
#include <glm.hpp>
#include <iostream>
#include <sstream>

#include "TileManager.h"

struct Object
{
    int type;
    glm::vec2 position;
    glm::vec2 dimensions = glm::vec2(TileManager::tileManager.TILE_SIZE, TileManager::tileManager.TILE_SIZE);
    glm::vec4 uvs;

    int level;
    int growthRateID;

    int inventorySize = 0;
    std::vector<int> inventory;
};

struct vec2Hash
{
    size_t operator()(const glm::vec2& vec) const
    {
        return ((std::hash<float>()(vec.x) ^ (std::hash<float>()(vec.y) << 1)) >> 1);
    }
};

struct EditMode
{
    const static int TILE = 0;
    const static int PICKUP = 1;
    const static int ENEMY = 2;
    int currentElement;
    int maxElement;
    //The current edit mode
    int type = -1;
    Object* dObject = nullptr;

    EditMode(Object* obj)
    {
        dObject = obj;
        currentElement = 0;
        dObject->type = currentElement;
    }

    virtual void rightClick(int x, int y) {}
    virtual void leftClick(int x, int y) {}
    virtual void switchElement(int next)
    {
        currentElement += next;
        if (currentElement < 0)
        {
            currentElement = maxElement;
        }
        if (currentElement > maxElement)
        {
            currentElement = 0;
        }

        dObject->type = currentElement;
    }


    virtual ~EditMode() {}
    //texture to use
};

struct TileMode : public EditMode
{
    TileMode(Object* obj) : EditMode(obj)
    {
        maxElement = 5;
        type = TILE;
    }
    void rightClick(int x, int y)
    {
        //Remove tile
        TileManager::tileManager.placeTile(x, y, -1);
    }
    //TODO need a method to set a tile and attribute
    void leftClick(int x, int y)
    {
        TileManager::tileManager.placeTile(x, y, dObject->type);
    }

    ~TileMode()
    {
    }
};


struct EnemyMode : public EditMode
{
    //1 when facing right, 2 when facing left
    //will double the type for uvs
    int facing;
    const static int RIGHT = 0;
    const static int LEFT = 1;

    const static int NUMBER_OF_ENEMIES = 2;

    EnemyMode(Object* obj, std::unordered_map<glm::vec2, Object, vec2Hash>& objects,
    std::unordered_map<glm::vec2, std::string, vec2Hash>& objectStrings,
    std::unordered_map<glm::vec2, int, vec2Hash>& objectWriteTypes,
        int& numEnemies, std::vector<glm::vec4>& enemyUVs) : EditMode(obj), numberOfEnemies(numEnemies), uvs(enemyUVs), objects(objects), objectStrings(objectStrings), objectWriteTypes(objectWriteTypes)
    {
        facing = RIGHT;
        maxElement = NUMBER_OF_ENEMIES - 1;
        updateDisplay();
        type = ENEMY;
    }

    std::unordered_map<glm::vec2, Object, vec2Hash>& objects;
    std::unordered_map<glm::vec2, std::string, vec2Hash>& objectStrings;
    std::unordered_map<glm::vec2, int, vec2Hash>& objectWriteTypes;
    std::vector<glm::vec4>& uvs;

    int& numberOfEnemies;
    void rightClick(int x, int y)
    {
        glm::vec2 mousePosition(x, y);
        if (objects.count(mousePosition) == 1)
        {
            objects.erase(mousePosition);
            objectStrings.erase(mousePosition);
            objectWriteTypes.erase(mousePosition);
            numberOfEnemies--;
        }
    }

    void leftClick(int x, int y);

    void placeEnemy(int level, int growthRateID, const std::vector<int>& inventory)
    {
        glm::vec2 startPosition = dObject->position;
        dObject->inventory = inventory;
        dObject->level = level;
        dObject->growthRateID = growthRateID;
        objects[startPosition] = *dObject;

        std::stringstream objectStream;
        objectStream << dObject->type << " " << startPosition.x << " " << startPosition.y << " " << level << " " << growthRateID << " " << inventory.size();
        for(int i = 0; i < inventory.size(); i ++)
        {
            objectStream << " " << inventory[i];
        }

        objectWriteTypes[startPosition] = 1; //not sure which of these I'm using

        numberOfEnemies++;

        objectStrings[startPosition] = objectStream.str();

        std::cout << objectStrings[startPosition] << std::endl;
    }

    void updateEnemy(int level, int growthRateID, const std::vector<int>& inventory, int type)
    {
        glm::vec2 startPosition = dObject->position;
        dObject->inventory = inventory;
        dObject->level = level;
        dObject->growthRateID = growthRateID;
        objects[startPosition] = *dObject;
        std::stringstream objectStream;
        objectStream << type << " " << startPosition.x << " " << startPosition.y << " " << level << " " << growthRateID << " " << inventory.size();
        for (int i = 0; i < inventory.size(); i++)
        {
            objectStream << " " << inventory[i];
        }
        objectWriteTypes[startPosition] = 1; //not sure which of these I'm using

        objectStrings[startPosition] = objectStream.str();

        std::cout << objectStrings[startPosition] << std::endl;
    }

    void switchElement(int next)
    {
        EditMode::switchElement(next);
        updateDisplay();
    }

    void swapFacing()
    {
        if (facing == RIGHT)
        {
            facing = LEFT;
        }
        else
        {
            facing = RIGHT;
        }
        updateDisplay();
    }

    void updateDisplay()
    {
        dObject->uvs = uvs[currentElement];

        dObject->dimensions = glm::vec2(16, 16);
    }

    ~EnemyMode()
    {
    }
};
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
#include "../Vendor.h"


struct Object
{
    int type;
    glm::vec2 position;
    glm::vec2 dimensions = glm::vec2(TileManager::tileManager.TILE_SIZE, TileManager::tileManager.TILE_SIZE);
    std::vector<glm::vec4> uvs;

    int level;
    int growthRateID;
    bool editedStats = false;
    bool editedProfs = false;
    bool stationary = false;
    bool bossBonus = false;
    bool sprite = true;
    int activationType = 0;
    int sceneID = -1;

    std::vector<int> stats;
    std::vector<int> profs;

    int inventorySize = 0;
    std::vector<int> inventory;
};

struct vec2Hash2
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
    const static int STARTS = 3;
    const static int VENDORS = 4;
    const static int ENEMY_ESCAPE = 5;
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
        maxElement = 29;
        type = TILE;
    }
    void rightClick(int x, int y)
    {
        //Remove tile
        // Not currently in use
      //  TileManager::tileManager.placeTile(x, y, -1);
    }
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

    const static int NUMBER_OF_ENEMIES = 3;

    EnemyMode(Object* obj, std::unordered_map<glm::vec2, Object, vec2Hash2>& objects,
    std::unordered_map<glm::vec2, std::string, vec2Hash2>& objectStrings,
    std::unordered_map<glm::vec2, int, vec2Hash2>& objectWriteTypes,
        int& numEnemies, std::vector<std::vector<glm::vec4>>& enemyUVs) : EditMode(obj), numberOfEnemies(numEnemies), uvs(enemyUVs), objects(objects), objectStrings(objectStrings), objectWriteTypes(objectWriteTypes)
    {
        facing = RIGHT;
        maxElement = NUMBER_OF_ENEMIES - 1;
        updateDisplay();
        type = ENEMY;
        dObject->sprite = true;
    }

    std::unordered_map<glm::vec2, Object, vec2Hash2>& objects;
    std::unordered_map<glm::vec2, std::string, vec2Hash2>& objectStrings;
    std::unordered_map<glm::vec2, int, vec2Hash2>& objectWriteTypes;
    std::vector<std::vector<glm::vec4>>& uvs;

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

    void placeEnemy(int level, int growthRateID, const std::vector<int>& inventory, std::vector<int>& stats, bool editedStats, std::vector<int>& profs, bool editedProfs, int attackActivation, bool stationary, bool bossBonus, int sceneID)
    {
        updateEnemy(level, growthRateID, inventory, dObject->type, stats, editedStats, profs, editedProfs, attackActivation, stationary, bossBonus, sceneID);
        numberOfEnemies++;
    }

    void updateEnemy(int level, int growthRateID, const std::vector<int>& inventory, int type, std::vector<int>& stats, bool editedStats, std::vector<int>& profs, bool editedProfs, int attackActivation, bool stationary, bool bossBonus, int sceneID)
    {
        glm::vec2 startPosition = dObject->position;
        dObject->type = type;
        dObject->inventory = inventory;
        dObject->level = level;
        dObject->growthRateID = growthRateID;
        dObject->stats = stats;
        dObject->editedStats = editedStats;
        dObject->profs = profs;
        dObject->editedProfs = editedProfs;
        dObject->activationType = attackActivation;
        dObject->stationary = stationary;
        dObject->bossBonus = bossBonus;
        dObject->sceneID = sceneID;
        objects[startPosition] = *dObject;
        std::stringstream objectStream;
        objectStream << type << " " << startPosition.x << " " << startPosition.y << " " << level << " " << growthRateID << " " << inventory.size();
        for (int i = 0; i < inventory.size(); i++)
        {
            objectStream << " " << inventory[i];
        }
        objectStream << " " << editedStats;
        if (editedStats)
        {
            for (int i = 0; i < stats.size(); i++)
            {
                objectStream << " " << stats[i];
            }
        }
        objectStream << " " << editedProfs;
        if (editedProfs)
        {
            for (int i = 0; i < profs.size(); i++)
            {
                objectStream << " " << profs[i];
            }
        }
        objectStream << " " << attackActivation << " " << stationary << " " << bossBonus;

        objectStream << " " << sceneID;
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

struct PlayerStartMode : public EditMode
{
    PlayerStartMode(Object* obj, std::unordered_map<glm::vec2, Object, vec2Hash2>& objects,
        std::unordered_map<glm::vec2, std::string, vec2Hash2>& objectStrings,
        std::unordered_map<glm::vec2, int, vec2Hash2>& objectWriteTypes, 
        int& numberOfStarts) : EditMode(obj), numberOfStarts(numberOfStarts), objects(objects), objectStrings(objectStrings), objectWriteTypes(objectWriteTypes)
    {
        maxElement = 99;
        type = STARTS;
        dObject->sprite = false;

    }

    std::unordered_map<glm::vec2, Object, vec2Hash2>& objects;
    std::unordered_map<glm::vec2, std::string, vec2Hash2>& objectStrings;
    std::unordered_map<glm::vec2, int, vec2Hash2>& objectWriteTypes;
    int& numberOfStarts;
    void rightClick(int x, int y)
    {
        glm::vec2 mousePosition(x, y);
        if (objects.count(mousePosition) == 1)
        {
            objects.erase(mousePosition);
            objectStrings.erase(mousePosition);
            objectWriteTypes.erase(mousePosition);
            numberOfStarts--;
        }
    }
    void leftClick(int x, int y)
    {
        glm::vec2 startPosition = dObject->position;
        objects[startPosition] = *dObject;
        std::stringstream objectStream;
        objectStream << dObject->type << " " << startPosition.x << " " << startPosition.y;

        objectWriteTypes[startPosition] = STARTS; //not sure which of these I'm using

        objectStrings[startPosition] = objectStream.str();

        std::cout << objectStrings[startPosition] << std::endl;
        numberOfStarts++;
    }
    ~PlayerStartMode()
    {
    }
};

struct VendorMode : public EditMode
{
    VendorMode(Object* obj, std::unordered_map<glm::vec2, class Vendor, vec2Hash2>& vendors, int& numberOfVendors) : EditMode(obj), vendors(vendors), numberOfVendors(numberOfVendors)
    {
        type = VENDORS;
    }

    std::unordered_map<glm::vec2, Vendor, vec2Hash2>& vendors;
    int& numberOfVendors;

    virtual void rightClick(int x, int y) 
    {
        glm::vec2 mousePosition(x, y);
        if (vendors.count(mousePosition) == 1)
        {
            vendors.erase(mousePosition);
            numberOfVendors--;
        }
    }
    virtual void leftClick(int x, int y);

};

struct EnemyEscapeMode : public EditMode
{
    EnemyEscapeMode(Object* obj, glm::ivec2& position) : EditMode(obj), position(position)
    {
        type = ENEMY_ESCAPE;
    }

    //In the future, it is possible and quite likely I will have more than one enemy escape point. 
    // In that case, I would probably use a map like I do for the other placement modes. 
    // For this project however, I only have the one.
    glm::ivec2& position;

    virtual void leftClick(int x, int y);
};
#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"
#include "TileManager.h"

void EnemyManager::GetPriority()
{
    auto enemy = enemies[1];
    auto weapon = enemy->GetWeaponData(enemy->GetEquippedItem());
    std::unordered_map<glm::vec2, pathPoint, vec2Hash> path = enemy->FindUnitMoveRange();

    /*
    * 	float attackDistance = abs(enemy->sprite.getPosition().x - unit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - unit->sprite.getPosition().y);
    attackDistance /= TileManager::TILE_SIZE;
    auto enemyWeapon = enemy->GetWeaponData(enemy->GetEquippedItem());
    if (enemyWeapon.maxRange >= attackDistance && enemyWeapon.minRange <= attackDistance)
    {
        enemyCanCounter = true;
    }
    */
    std::vector<Unit*> otherUnits;
    otherUnits.reserve(6);
    for (auto const& it : enemy->attackTiles)
    {
        auto point = it;
        if (Unit* unit = TileManager::tileManager.getUnitOnTeam(point.x, point.y, 0))
        {
            otherUnits.push_back(unit);
        }
    }
    std::vector<Target> targets;
    targets.resize(otherUnits.size());
    for (int i = 0; i < otherUnits.size(); i++)
    {
        auto otherUnit = otherUnits[i];
        auto test = ValidAttackPosition(otherUnit, path, enemy->minRange, enemy->maxRange);
        int a = 2;
        //	if(otherUnit)
    }

}

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}

//This can be improved by only checking points that are on the path
std::vector<glm::vec2> EnemyManager::ValidAttackPosition(Unit* toAttack, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path, int minRange, int maxRange)
{
    auto position = toAttack->sprite.getPosition();
    std::vector<glm::vec2> foundTiles;
    std::vector<glm::vec2> rangeTiles;
    std::vector<searchCell> checking;
    std::vector<std::vector<bool>> checked;

    checked.resize(TileManager::tileManager.levelWidth);
    for (int i = 0; i < TileManager::tileManager.levelWidth; i++)
    {
        checked[i].resize(TileManager::tileManager.levelHeight);
    }
    std::vector<std::vector<int>> costs;
    costs.resize(TileManager::tileManager.levelWidth);
    for (int i = 0; i < TileManager::tileManager.levelWidth; i++)
    {
        costs[i].resize(TileManager::tileManager.levelHeight);
    }
    for (int i = 0; i < TileManager::tileManager.levelWidth; i++)
    {
        for (int c = 0; c < TileManager::tileManager.levelHeight; c++)
        {
            costs[i][c] = 50;
        }
    }
    glm::ivec2 normalPosition = glm::ivec2(position) / TileManager::TILE_SIZE;
    costs[normalPosition.x][normalPosition.y] = 0;
    searchCell first = { normalPosition, 0 };
    addToOpenSet(first, checking, checked, costs);
    while (checking.size() > 0)
    {
        auto current = checking[0];
        removeFromOpenList(checking);
        int cost = current.moveCost;
        glm::vec2 checkPosition = current.position;

        glm::vec2 up = glm::vec2(checkPosition.x, checkPosition.y - 1);
        glm::vec2 down = glm::vec2(checkPosition.x, checkPosition.y + 1);
        glm::vec2 left = glm::vec2(checkPosition.x - 1, checkPosition.y);
        glm::vec2 right = glm::vec2(checkPosition.x + 1, checkPosition.y);

        CheckAdjacentTiles(up, checked, checking, current, costs, foundTiles, rangeTiles, path);
        CheckAdjacentTiles(down, checked, checking, current, costs, foundTiles, rangeTiles, path);
        CheckAdjacentTiles(right, checked, checking, current, costs, foundTiles, rangeTiles, path);
        CheckAdjacentTiles(left, checked, checking, current, costs, foundTiles, rangeTiles, path);
    }
    return rangeTiles;
}

void EnemyManager::addToOpenSet(searchCell newCell, std::vector<searchCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs)
{
    int position;
    checking.push_back(newCell);
    checked[newCell.position.x][newCell.position.y] = true;
    costs[newCell.position.x][newCell.position.y] = newCell.moveCost;

    position = checking.size() - 1;
    while (position != 0)
    {
        if (checking[position].moveCost < checking[position / 2].moveCost)
        {
            searchCell tempNode = checking[position];
            checking[position] = checking[position / 2];
            checking[position / 2] = tempNode;
            position /= 2;
        }
        else
        {
            break;
        }
    }
}

void EnemyManager::removeFromOpenList(std::vector<searchCell>& checking)
{
    int position = 1;
    int position2;
    searchCell temp;
    std::vector<searchCell>::iterator it = checking.end() - 1;
    checking[0] = checking[checking.size() - 1];
    checking.erase(it);
    while (position < checking.size())
    {
        position2 = position;
        if (2 * position2 + 1 <= checking.size())
        {
            if (checking[position2 - 1].moveCost > checking[2 * position2 - 1].moveCost)
            {
                position = 2 * position2;
            }
            if (checking[position - 1].moveCost > checking[(2 * position2 + 1) - 1].moveCost)
            {
                position = 2 * position2 + 1;
            }
        }
        else if (2 * position2 <= checking.size())
        {
            if (checking[position2 - 1].moveCost > checking[(2 * position2) - 1].moveCost)
            {
                position = 2 * position2;
            }
        }
        if (position2 != position)
        {
            temp = checking[position2 - 1];
            checking[position2 - 1] = checking[position - 1];
            checking[position - 1] = temp;
        }
        else
        {
            break;
        }
    }
}

void EnemyManager::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<glm::vec2>& rangeTiles, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path)
{
    glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
    if (path.find(tilePosition) != path.end())
  //  if (!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
    {
        int mCost = startCell.moveCost;
        auto thisTile = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y);
        int movementCost = mCost + 1;

        auto distance = costs[checkingTile.x][checkingTile.y];
        if (!checked[checkingTile.x][checkingTile.y])
        {
            //This is a weird thing that is only needed to get the attack range, I hope to remove it at some point.
            if (movementCost < distance)
            {
                costs[checkingTile.x][checkingTile.y] = movementCost;
            }
            auto something = enemies[1];
            if (movementCost <= something->maxRange)
            {
                if ((something->minRange == something->maxRange && movementCost == something->maxRange) ||
                    (something->minRange < something->maxRange && movementCost <= something->maxRange))
                {
                    rangeTiles.push_back(tilePosition);
                }
                searchCell newCell{ checkingTile, movementCost };
                addToOpenSet(newCell, checking, checked, costs);
                foundTiles.push_back(tilePosition);
            }
        }
    }
}

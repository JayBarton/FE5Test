#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"
#include "TileManager.h"

#include <algorithm>

bool compareTargets(const Target& a, const Target& b)
{
    return a.priority > b.priority;
}

void EnemyManager::GetPriority()
{
    otherUnit = nullptr;
    canCounter = true;
    auto enemy = enemies[1];
    auto position = enemy->sprite.getPosition();
    //Assme this unit moves
    TileManager::tileManager.removeUnit(position.x, position.y);

    BattleStats battleStats;
    std::unordered_map<glm::vec2, pathPoint, vec2Hash> path = enemy->FindUnitMoveRange();
    
    int cannotCounterBonus = 50;

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
    if (otherUnits.size() == 0)
    {
        enemies[1]->placeUnit(position.x, position.y); 
        enemies[1]->hasMoved = true;
    }
    else
    {
        std::vector<Target> targets;
        targets.resize(otherUnits.size());
        for (int i = 0; i < otherUnits.size(); i++)
        {
            auto otherUnit = otherUnits[i];
            auto otherWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
            auto attackPositions = ValidAttackPosition(otherUnit, path, enemy->minRange, enemy->maxRange);

            targets[i].ID = i;
            //First want to determine if the other unit can counter. Very high priority if they cannot
            if (enemy->maxRange > otherWeapon.maxRange)
            {
                //Only worth considering if this unit can actually reach the range being checked
                //For ranges greater than 1, this would suggest every space around the other unit is occupied
                bool canReach = false;
                for (int c = 0; c < attackPositions.size(); c++)
                {
                    if (attackPositions[c].distance == enemy->maxRange)
                    {
                        canReach = true;
                        break;
                    }
                }
                if (canReach)
                {
                    targets[i].range = enemy->maxRange;
                    targets[i].priority += cannotCounterBonus;
                    canCounter = false;
                }
            }
            else if (enemy->minRange < otherWeapon.minRange)
            {
                //Same as above, though with a range of 1 it could also suggest the other unit is 2 or more
                //tiles from the edge of this enemy's movement range
                bool canReach = false;
                for (int c = 0; c < attackPositions.size(); c++)
                {
                    if (attackPositions[c].distance == enemy->minRange)
                    {
                        canReach = true;
                        break;
                    }
                }
                if (canReach)
                {
                    targets[i].range = enemy->minRange;
                    targets[i].priority += cannotCounterBonus;
                    canCounter = false;
                }
            }
            //Next want to check how much damage this enemy can do to the other unit
            //If the enemy is already trying to target an enemy that cannot counter, only want to consider using weapons of the same range
            BattleStats tempStats;
            int maxDamage = 0;
            int rangeToUse = 0;

            for (int c = 0; c < enemy->weapons.size(); c++)
            {
                auto weapon = enemy->GetWeaponData(enemy->weapons[c]);
                if (targets[i].priority >= cannotCounterBonus)
                {
                    if (weapon.maxRange == targets[i].range || weapon.minRange == targets[i].range)
                    {
                        //calculate damage
                        //for right now just going by other's defense
                        tempStats = enemy->CalculateBattleStats(enemy->weapons[c]->ID);

                        int damage = tempStats.attackDamage - otherUnit->defense;
                        if (damage > maxDamage)
                        {
                            maxDamage = damage;
                            battleStats = tempStats;
                            targets[i].weaponToUse = enemy->weapons[c];
                        }
                    }
                }
                else
                {
                    auto weaponData = enemy->GetWeaponData(enemy->weapons[c]);
                    bool canReach = false;

                    //Need to confirm we can reach the unit. Don't need to do this above as it has already been handled
                    //prefer to attack from as far away as possible
                    for (int j = 0; j < attackPositions.size(); j++)
                    {
                        if (attackPositions[j].distance == weaponData.maxRange)
                        {
                            canReach = true;
                            rangeToUse = weaponData.maxRange;
                            break;
                        }
                    }
                    if (!canReach)
                    {
                        for (int j = 0; j < attackPositions.size(); j++)
                        {
                            if (attackPositions[j].distance == weaponData.minRange)
                            {
                                canReach = true;
                                rangeToUse = weaponData.minRange;
                                break;
                            }
                        }
                    }
                    tempStats = enemy->CalculateBattleStats(enemy->weapons[c]->ID);
                    if (canReach)
                    {
                        int damage = tempStats.attackDamage - otherUnit->defense;
                        if (damage > maxDamage)
                        {
                            maxDamage = damage;
                            battleStats = tempStats;
                            //If we hadn't gotten the range from a unit that cannot counter, get it now
                            //Not sure if using min or max or what here
                            targets[i].range = rangeToUse;
                            targets[i].weaponToUse = enemy->weapons[c];
                        }
                    }
                }

            }
            targets[i].priority += maxDamage;
        }
        std::sort(targets.begin(), targets.end(), compareTargets);

        for (int i = 0; i < enemy->inventory.size(); i++)
        {
            if (targets[0].weaponToUse == enemy->inventory[i])
            {
                enemy->equipWeapon(i);
                break;
            }
        }
        //Can probably do this better
        otherUnit = otherUnits[targets[0].ID];
        std::cout << "Attack " << otherUnit->name << " with " << enemy->GetEquippedItem()->name;
        //Definitely don't need to redo this calculation
        auto attackPositions = ValidAttackPosition(otherUnit, path, enemy->minRange, enemy->maxRange);
        glm::vec2 attackPosition;
        int minDistance = 1000;

        for (int i = 0; i < attackPositions.size(); i++)
        {
            auto aPositionn = attackPositions[i].position;
            if (i >= 4)
            {
                int a = 2;
            }
            if (attackPositions[i].distance == targets[0].range)
            {
                int distance = abs(position.x - aPositionn.x) + abs(position.y - aPositionn.y);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    attackPosition = attackPositions[i].position;
                }
            }
        }
        std::vector<glm::ivec2> followPath;
        glm::vec2 pathPoint = attackPosition;
        followPath.push_back(pathPoint);

        while (pathPoint != enemy->sprite.getPosition())
        {
            auto previous = path[pathPoint].previousPosition;
            followPath.push_back(previous);
            pathPoint = previous;
        }

        enemy->movementComponent.startMovement(followPath);
    }
}

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}

void EnemyManager::Update(float deltaTime)
{
    for (int i = 0; i < enemies.size(); i++)
    {
    //    enemies[i]->Update(deltaTime);
    }
    enemies[1]->Update(deltaTime);

}

std::vector<AttackPosition> EnemyManager::ValidAttackPosition(Unit* toAttack, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path, int minRange, int maxRange)
{
    auto position = toAttack->sprite.getPosition();
    std::vector<glm::vec2> foundTiles;
    std::vector<AttackPosition> rangeTiles;
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

void EnemyManager::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path)
{
    glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
    if(!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
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
               // something->minRange = 1;
                if ((something->minRange == something->maxRange && movementCost == something->maxRange) ||
                    (something->minRange < something->maxRange && movementCost <= something->maxRange))
                {
                    if (path.find(tilePosition) != path.end() && !TileManager::tileManager.getUnit(tilePosition.x, tilePosition.y))
                    {
                        rangeTiles.push_back({ tilePosition, movementCost });
                    }
                }
                searchCell newCell{ checkingTile, movementCost };
                addToOpenSet(newCell, checking, checked, costs);
                foundTiles.push_back(tilePosition);
            }
        }
    }
}

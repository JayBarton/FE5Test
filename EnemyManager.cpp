#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"
#include "TileManager.h"
#include "BattleManager.h"

#include <algorithm>
#include <fstream>  
#include "csv.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void EnemyManager::GetPriority(Unit* enemy)
{
    state = GET_TARGET;
    otherUnit = nullptr;
    canCounter = true;
    auto position = enemy->sprite.getPosition();
    //Assme this unit moves
    TileManager::tileManager.removeUnit(position.x, position.y);

    BattleStats battleStats;
    std::unordered_map<glm::vec2, pathCell, vec2Hash> path = enemy->FindUnitMoveRange();
    
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
    //If not in range of any units, enemy remains where they are
    if (otherUnits.size() == 0)
    {
        NoMove(enemy, position);
    }
    else
    {
        Target finalTarget;
        for (int i = 0; i < otherUnits.size(); i++)
        {
            Target currentTarget;

            auto otherUnit = otherUnits[i];
            auto otherWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
            currentTarget.attackPositions = ValidAttackPosition(otherUnit, path, enemy->minRange, enemy->maxRange);

            currentTarget.ID = i;
            //First want to determine if the other unit can counter. Very high priority if they cannot
            if (enemy->maxRange > otherWeapon.maxRange)
            {
                //Only worth considering if this unit can actually reach the range being checked
                //For ranges greater than 1, this would suggest every space around the other unit is occupied
                bool canReach = false;
                for (int c = 0; c < currentTarget.attackPositions.size(); c++)
                {
                    if (currentTarget.attackPositions[c].distance == enemy->maxRange)
                    {
                        canReach = true;
                        break;
                    }
                }
                if (canReach)
                {
                    currentTarget.range = enemy->maxRange;
                    currentTarget.priority += cannotCounterBonus;
                    canCounter = false;
                }
            }
            else if (enemy->minRange < otherWeapon.minRange)
            {
                //Same as above, though with a range of 1 it could also suggest the other unit is 2 or more
                //tiles from the edge of this enemy's movement range
                bool canReach = false;
                for (int c = 0; c < currentTarget.attackPositions.size(); c++)
                {
                    if (currentTarget.attackPositions[c].distance == enemy->minRange)
                    {
                        canReach = true;
                        break;
                    }
                }
                if (canReach)
                {
                    currentTarget.range = enemy->minRange;
                    currentTarget.priority += cannotCounterBonus;
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
                if (currentTarget.priority >= cannotCounterBonus)
                {
                    if (weapon.maxRange == currentTarget.range || weapon.minRange == currentTarget.range)
                    {
                        //calculate damage
                        //for right now just going by other's defense
                        tempStats = enemy->CalculateBattleStats(enemy->weapons[c]->ID);

                        int damage = tempStats.attackDamage - otherUnit->defense;
                        if (damage > maxDamage)
                        {
                            maxDamage = damage;
                            battleStats = tempStats;
                            currentTarget.weaponToUse = enemy->weapons[c];
                        }
                    }
                }
                else
                {
                    auto weaponData = enemy->GetWeaponData(enemy->weapons[c]);
                    bool canReach = false;

                    //Need to confirm we can reach the unit. Don't need to do this above as it has already been handled
                    //prefer to attack from as far away as possible
                    for (int j = 0; j < currentTarget.attackPositions.size(); j++)
                    {
                        if (currentTarget.attackPositions[j].distance == weaponData.maxRange)
                        {
                            canReach = true;
                            rangeToUse = weaponData.maxRange;
                            break;
                        }
                    }
                    if (!canReach)
                    {
                        for (int j = 0; j < currentTarget.attackPositions.size(); j++)
                        {
                            if (currentTarget.attackPositions[j].distance == weaponData.minRange)
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
                            currentTarget.range = rangeToUse;
                            currentTarget.weaponToUse = enemy->weapons[c];
                        }
                    }
                }
            }
            currentTarget.priority += maxDamage;
            if (currentTarget.priority > finalTarget.priority)
            {
                finalTarget = currentTarget;
            }
        }
        //In range of units but cannot reach any of them, stay where you are
        if (finalTarget.priority == 0)
        {
            NoMove(enemy, position);
        }
        else
        {
            for (int i = 0; i < enemy->inventory.size(); i++)
            {
                if (finalTarget.weaponToUse == enemy->inventory[i])
                {
                    enemy->equipWeapon(i);
                    break;
                }
            }
            otherUnit = otherUnits[finalTarget.ID];
            auto attackPositions = finalTarget.attackPositions;
            glm::vec2 attackPosition;
            int minDistance = 100;

            for (int i = 0; i < attackPositions.size(); i++)
            {
                auto aPosition = attackPositions[i].position;
                if (attackPositions[i].distance == finalTarget.range)
                {
                    int distance = path[aPosition].moveCost;
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
            state = ATTACK;
            enemy->movementComponent.startMovement(followPath, path[attackPosition].moveCost, false);
            enemyMoving = true;
        }
    }
}

void EnemyManager::NoMove(Unit* enemy, glm::vec2& position)
{
    enemy->placeUnit(position.x, position.y);
    enemy->hasMoved = true;
    currentEnemy++;
}

void EnemyManager::SetUp(std::ifstream& map, std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
    UVs = ResourceManager::GetTexture("sprites").GetUVs(TileManager::TILE_SIZE, TileManager::TILE_SIZE);
    std::vector<Unit> unitBases;
    unitBases.resize(3);

    std::ifstream f("BaseStats.json");
    json data = json::parse(f);
    json bases = data["enemies"];
    int currentUnit = 0;
    std::unordered_map<std::string, int> weaponNameMap;
    weaponNameMap["Sword"] = WeaponData::TYPE_SWORD;
    weaponNameMap["Axe"] = WeaponData::TYPE_AXE;
    weaponNameMap["Lance"] = WeaponData::TYPE_LANCE;
    weaponNameMap["Bow"] = WeaponData::TYPE_BOW;
    weaponNameMap["Thunder"] = WeaponData::TYPE_THUNDER;
    weaponNameMap["Fire"] = WeaponData::TYPE_FIRE;
    weaponNameMap["Wind"] = WeaponData::TYPE_WIND;
    weaponNameMap["Dark"] = WeaponData::TYPE_DARK;
    weaponNameMap["Light"] = WeaponData::TYPE_LIGHT;
    weaponNameMap["Staff"] = WeaponData::TYPE_STAFF;

    for (const auto& enemy : bases) {
        int ID = enemy["ID"];
        std::string name = enemy["Name"];
        json stats = enemy["Stats"];
        int HP = stats["HP"];
        int str = stats["Str"];
        int mag = stats["Mag"];
        int skl = stats["Skl"];
        int spd = stats["Spd"];
        int lck = stats["Lck"];
        int def = stats["Def"];
        int bld = stats["Bld"];
        int mov = stats["Mov"];
        unitBases[currentUnit] = Unit(name, name, HP, str, mag, skl, spd, lck, def, bld, mov);

        json weaponProf = enemy["WeaponProf"];
        for (auto it = weaponProf.begin(); it != weaponProf.end(); ++it)
        {
            unitBases[currentUnit].weaponProficiencies[weaponNameMap[it.key()]] = int(it.value());
        }
        currentUnit++;
    }

    int ID;
    int HP;
    int str;
    int mag;
    int skl;
    int spd;
    int lck;
    int def;
    int bld;

    io::CSVReader<9, io::trim_chars<' '>, io::no_quote_escape<':'>> in2("EnemyGrowths.csv");
    in2.read_header(io::ignore_extra_column, "ID", "HP", "Str", "Mag", "Skl", "Spd", "Lck", "Def", "Bld");
    currentUnit = 0;
    std::vector<StatGrowths> unitGrowths;
    unitGrowths.resize(6);
    while (in2.read_row(ID, HP, str, mag, skl, spd, lck, def, bld)) {
        unitGrowths[currentUnit] = StatGrowths{ HP, str, mag, skl, spd, lck, def, bld, 0 };
        currentUnit++;
    }

    int numberOfEnemies;
    map >> numberOfEnemies;
    enemies.resize(numberOfEnemies);
    for (int i = 0; i < numberOfEnemies; i++)
    {
        glm::vec2 position;
        int type;
        int level;
        int growthID;
        int inventorySize;
        map >> type >> position.x >> position.y >> level >> growthID >> inventorySize;
        enemies[i] = new Unit(unitBases[type]);
        enemies[i]->team = 1;
        enemies[i]->growths = unitGrowths[growthID];
        for (int c = 0; c < inventorySize; c++)
        {
            int itemID;
            map >> itemID;
            enemies[i]->addItem(itemID);
        }

        enemies[i]->init(gen, distribution);
        enemies[i]->LevelEnemy(level - 1);
        enemies[i]->placeUnit(position.x, position.y);
        enemies[i]->sprite.uv = &UVs;
    }
}

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}

void EnemyManager::Update(BattleManager& battleManager)
{
    if (currentEnemy >= enemies.size())
    {
        EndTurn();
    }
    else
    {
        auto enemy = enemies[currentEnemy];

        if (!enemyMoving)
        {
            GetPriority(enemy);
        }
        else
        {
            if (!enemy->movementComponent.moving)
            {
                enemy->placeUnit(enemy->sprite.getPosition().x, enemy->sprite.getPosition().y);
                if (state == ATTACK)
                {
                    battleManager.SetUp(enemy, otherUnit, enemy->CalculateBattleStats(), otherUnit->CalculateBattleStats(), canCounter);
                }
                else if(state == CANTO)
                {
                    FinishMove();
                }
            }
        }
    }
}

void EnemyManager::CantoMove()
{
    auto enemy = enemies[currentEnemy];
    auto position = enemy->sprite.getPosition();

    auto directionToOtherUnit = glm::normalize(glm::vec2(position - otherUnit->sprite.getPosition()));


    TileManager::tileManager.removeUnit(position.x, position.y);
    state = CANTO;
    auto path = enemy->FindRemainingMoveRange();
    int bestValue = 0;
    glm::ivec2 bestPosition;
    for (auto const& x : path)
    {
        auto tile = TileManager::tileManager.getTile(x.first.x, x.first.y);
        if (!tile->occupiedBy)
        {
            auto tileProperties = tile->properties;
            auto directionToTarget = glm::normalize(glm::vec2(position - x.first));
            int oppositeDirectionBonus = 0;
            //Want to lightly discourage enemies from running past player units, and instead run in the direction they came
            if (directionToOtherUnit.x * directionToTarget.x > 0 || directionToOtherUnit.y * directionToTarget.y > 0)
            {
                oppositeDirectionBonus = -1;
            }
            int tileConsiderations = x.second.moveCost + tileProperties.avoid + (tileProperties.defense * 10) + oppositeDirectionBonus;
            if (tileConsiderations > bestValue)
            {
                bestValue = tileConsiderations;
                bestPosition = x.first;

            }
        }
    }
    if (bestValue == 0)
    {
        NoMove(enemy, position);
    }
    else
    {
        std::vector<glm::ivec2> followPath;
        glm::vec2 pathPoint = bestPosition;
        followPath.push_back(pathPoint);

        while (pathPoint != enemy->sprite.getPosition())
        {
            auto previous = path[pathPoint].previousPosition;
            followPath.push_back(previous);
            pathPoint = previous;
        }
        enemy->movementComponent.startMovement(followPath, path[bestPosition].moveCost, true);
    }
}

void EnemyManager::FinishMove()
{
    enemies[currentEnemy]->hasMoved = true;
    enemyMoving = false;
    currentEnemy++;
    state = GET_TARGET;
}

void EnemyManager::UpdateEnemies(float deltaTime)
{
    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->Update(deltaTime);
    }
}

void EnemyManager::EndTurn()
{
    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->hasMoved = false;
    }
    subject.notify(1);
    std::cout << "Player Turn Start\n";

}

void EnemyManager::Clear()
{
    for (int i = 0; i < enemies.size(); i++)
    {
        delete enemies[i];
    }
    enemies.clear();
}

std::vector<AttackPosition> EnemyManager::ValidAttackPosition(Unit* toAttack, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, int minRange, int maxRange)
{
    auto position = toAttack->sprite.getPosition();
    std::vector<glm::vec2> foundTiles;
    std::vector<AttackPosition> rangeTiles;
    std::vector<pathCell> checking;
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
    pathCell first = { normalPosition, 0 };
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

void EnemyManager::addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs)
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
            pathCell tempNode = checking[position];
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

void EnemyManager::removeFromOpenList(std::vector<pathCell>& checking)
{
    int position = 1;
    int position2;
    pathCell temp;
    std::vector<pathCell>::iterator it = checking.end() - 1;
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

void EnemyManager::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path)
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
                    if (path.find(tilePosition) != path.end() && !thisTile->occupiedBy)
                    {
                        rangeTiles.push_back({ tilePosition, movementCost });
                    }
                }
                pathCell newCell{ checkingTile, movementCost };
                addToOpenSet(newCell, checking, checked, costs);
                foundTiles.push_back(tilePosition);
            }
        }
    }
}

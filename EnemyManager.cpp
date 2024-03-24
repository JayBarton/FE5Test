#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"
#include "TileManager.h"
#include "BattleManager.h"
#include "Camera.h"
#include "PathFinder.h"


#include <algorithm>
#include <fstream>  
#include "InfoDisplays.h"
#include "csv.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void EnemyManager::GetPriority(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, std::vector<Unit*>& otherUnits)
{
    otherUnit = nullptr;
    canCounter = true;
    auto position = enemy->sprite.getPosition();
    //Assme this unit moves
    TileManager::tileManager.removeUnit(position.x, position.y);

    battleStats = BattleStats{};

    int cannotCounterBonus = 50;

    Target finalTarget;
    for (int i = 0; i < otherUnits.size(); i++)
    {
        Target currentTarget;

        auto otherUnit = otherUnits[i];
        auto otherWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
        currentTarget.attackPositions = ValidAdjacentPositions(otherUnit, path, enemy->minRange, enemy->maxRange);
        if (currentTarget.attackPositions.size() > 0)
        {
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
                        tempStats = enemy->CalculateBattleStats(enemy->weapons[c]->ID);
                        enemy->CalculateMagicDefense(weapon, tempStats, currentTarget.range);

                        int otherDefense = tempStats.attackType == 0 ? otherUnit->defense : otherUnit->magic;

                        int damage = tempStats.attackDamage - otherDefense;
                        if (damage > maxDamage)
                        {
                            maxDamage = damage;
                            currentTarget.battleStats = tempStats;
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
                        //Okay, so as this is written, an enemy with a magic sword will prefer to attack from range regardless of if attacking
                        //at a closer ranger would do more damage.
                        //At the very least, I think if a close range attack would kill, the enemy should do that, but it's not in yet.
                        enemy->CalculateMagicDefense(weapon, tempStats, rangeToUse);

                        int otherDefense = tempStats.attackType == 0 ? otherUnit->defense : otherUnit->magic;

                        int damage = tempStats.attackDamage - otherDefense;
                        if (damage > maxDamage)
                        {
                            //prioritize sure kills
                            if (otherUnit->currentHP - damage <= 0)
                            {
                                maxDamage = 50;
                            }
                            else
                            {
                                maxDamage = damage;
                            }
                            currentTarget.battleStats = tempStats;
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
            if (currentTarget.priority == 0)
            {
                int a = 2;
            }
            //The way this is written, an equal priority would suggest this enemy will do the same amount of damage to multiple units.
            //In that case, check which unit will do less damage on counter
            //Priority of 0 at this point would suggest zero damage will be dealt
            //I do want enemies to be able to attack even if they won't do any damage, so I'll need to rethink this.
            else if (currentTarget.priority == finalTarget.priority)
            {
                auto previousUnit = otherUnits[finalTarget.ID];
                auto previousWeapon = previousUnit->GetWeaponData(previousUnit->GetEquippedItem());
                auto foeWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
                auto previousStats = previousUnit->CalculateBattleStats();
                previousUnit->CalculateMagicDefense(previousWeapon, previousStats, finalTarget.range);
                auto otherStats = otherUnit->CalculateBattleStats();
                otherUnit->CalculateMagicDefense(foeWeapon, otherStats, currentTarget.range);

                int previousDamage = previousStats.attackDamage - (previousStats.attackType == 0 ? enemy->defense : enemy->magic);
                int damageTaken = otherStats.attackDamage - (otherStats.attackType == 0 ? enemy->defense : enemy->magic);
                if (damageTaken < previousDamage)
                {
                    finalTarget = currentTarget;
                }
            }
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
        int maxValue = -100;
        for (int i = 0; i < attackPositions.size(); i++)
        {
            auto aPosition = attackPositions[i].position;
            if (attackPositions[i].distance == finalTarget.range)
            {
                auto tileProperties = TileManager::tileManager.getTile(aPosition.x, aPosition.y)->properties;
                int tileValue = tileProperties.avoid + tileProperties.defense - path[aPosition].moveCost;

                int distance = path[aPosition].moveCost;
                if (tileValue > maxValue)
                {
                    maxValue = tileValue;
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
        attackRange = finalTarget.range;
        battleStats = finalTarget.battleStats;
        enemy->movementComponent.startMovement(followPath, path[attackPosition].moveCost, false);
        enemyMoving = true;
    }
}

void EnemyManager::ApproachNearest(glm::vec2& position, Unit* enemy)
{
    state = APPROACHING;
    int minDistance = 1000;
    int index = -1;
    for (int i = 0; i < playerUnits->size(); i++)
    {
        auto playerUnit = (*playerUnits)[i];
        if (!playerUnit->isDead)
        {
            auto playerPosition = playerUnit->sprite.getPosition();

            auto mDistance = abs(position.x - playerPosition.x) + abs(position.y - playerPosition.y);
            if (mDistance < minDistance)
            {
                minDistance = mDistance;
                index = i;
            }
        }
    }
    auto playerUnit = (*playerUnits)[index];
    auto path = pathFinder.findPath(position, playerUnit->sprite.getPosition(), enemy->move);
    enemy->movementComponent.startMovement(path, enemy->move, false);
    enemyMoving = true;
}

//This and finish move are nearly identical...
void EnemyManager::NoMove(Unit* enemy, glm::vec2& position)
{
    enemy->placeUnit(position.x, position.y);
 //   enemy->hasMoved = true;
    currentEnemy++;
}
std::vector<Unit*> EnemyManager::GetOtherUnits(Unit* enemy)
{
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
    return otherUnits;
}

void EnemyManager::SetUp(std::ifstream& map, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, std::vector<Unit*>* playerUnits)
{
    UVs = ResourceManager::GetTexture("sprites").GetUVs(TileManager::TILE_SIZE, TileManager::TILE_SIZE);
    std::vector<Unit> unitBases;
    unitBases.resize(4);
    this->playerUnits = playerUnits;

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
        if (enemy.find("Skills") != enemy.end())
        {
            auto skills = enemy["Skills"];
            for (const auto& skill : skills)
            {
                unitBases[currentUnit].skills.push_back(int(skill));
            }
        }
        if (enemy.find("ClassPower") != enemy.end())
        {
            unitBases[currentUnit].classPower = enemy["ClassPower"];
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
        enemies[i]->init(gen, distribution);

        enemies[i]->team = 1;
        enemies[i]->growths = unitGrowths[growthID];
        std::vector<int> inventory;
        inventory.resize(inventorySize);
        for (int c = 0; c < inventorySize; c++)
        {
            int itemID;
            map >> itemID;
            inventory[c] = itemID;
        }
        int editedStats;
        map >> editedStats;
        if (editedStats)
        {
            int stats[9];
            for (int c = 0; c < 9; c++)
            {
                map >> stats[c];
            }
            enemies[i]->maxHP = stats[0];
            enemies[i]->currentHP = stats[0];
            enemies[i]->strength = stats[1];
            enemies[i]->magic = stats[2];
            enemies[i]->skill = stats[3];
            enemies[i]->speed = stats[4];
            enemies[i]->luck = stats[5];
            enemies[i]->defense = stats[6];
            enemies[i]->build = stats[7];
            enemies[i]->move = stats[8];
            enemies[i]->level = level;
        }
        else
        {
            enemies[i]->LevelEnemy(level - 1);
        }
        int editedProfs;
        map >> editedProfs;
        if (editedProfs)
        {
            int profs[10];
            for (int c = 0; c < 10; c++)
            {
                map >> enemies[i]->weaponProficiencies[c];
            }
        }
        
        map >> enemies[i]->activationType >> enemies[i]->stationary >> enemies[i]->boss;

        for (int c = 0; c < inventorySize; c++)
        {
            enemies[i]->addItem(inventory[c]);
        }

        enemies[i]->placeUnit(position.x, position.y);
        enemies[i]->sprite.uv = &UVs;
        enemies[i]->sprite.color = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    enemies[14]->currentHP = 14;
  //  enemies[0]->move = 6;
  //  enemies[0]->mount = new Mount(Unit::HORSE, 1, 1, 1, 2, 3);
}

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}

void EnemyManager::Update(float deltaTime, BattleManager& battleManager, Camera& camera)
{
    followCamera = false;
    if (currentEnemy >= enemies.size())
    {
        EndTurn();
    }
    else
    {
        auto enemy = enemies[currentEnemy];
        timer += deltaTime;

        if (enemy->stationary)
        {
            if (timer >= turnStartDelay)
            {
                timer = 0.0f;
                StationaryUpdate(enemy, battleManager, camera);
            }
        }
        else
        {
            if (!enemyMoving)
            {
                //Small delay between starting the next enemy's move
                if (timer >= turnStartDelay)
                {
                    timer = 0.0f;
                    //Need to move the camera to the next enemy. Not exactly sure how FE5 handles this
                    //If active
                    if (enemy->active)
                    {
                        std::unordered_map<glm::vec2, pathCell, vec2Hash> path = enemy->FindUnitMoveRange();
                        if (enemy->currentHP <= enemy->maxHP * 0.5f)
                        {
                            healIndex = -1;
                            for (int i = 0; i < enemy->inventory.size(); i++)
                            {
                                if (enemy->inventory[i]->ID == 0)
                                {
                                    //can heal. Move away from closest player unit and heal
                                    healIndex = i;
                                    break;
                                }
                            }
                            if (healIndex >= 0)
                            {
                                HealSelf(enemy, path);
                            }
                            if (healIndex < 0)
                            {
                                FindHealItem(enemy, path);
                            }
                        }
                        else
                        {
                            FindUnitInAttackRange(enemy, path);
                        }
                    }
                    //Else if not active, dijkstra within a specified range, default to movement + max range + 1. 
                    //If units found, set active to true and path towards the closest unit
                    //Should be different activation requirements. Some enemies use as I described above, others will only activate if unit is in attack range
                    //So I could have an "activation type", if it is range I just use the above, if it is "attack" I can just call get priority again.
                    else
                    {
                        if (enemy->activationType > 0)
                        {
                            RangeActivation(enemy);
                            //Move this into range activation if it works
                            if (enemyMoving)
                            {
                          //      camera.SetMove(enemy->sprite.getPosition());
                            }
                        }
                        else
                        {
                            std::unordered_map<glm::vec2, pathCell, vec2Hash> path = enemy->FindUnitMoveRange();
                            std::vector<Unit*> otherUnits = GetOtherUnits(enemy);
                            GetPriority(enemy, path, otherUnits);
                            //So what I am doing here is, if the enemy is able to move/attack, set them to active.
                            //This is only going to work in the case that the enemy actually found a unit to attack, so if all of the attack positions are
                            //occupied, they will remain inactive
                            //Not sure if this is how I want things to work going forward, but it's the plan for now.
                            if (enemyMoving)
                            {
                                enemy->active = true;
                             //   camera.SetMove(enemy->sprite.getPosition());
                            }
                        }
                    }

                }
            }
            else
            {

                followCamera = true;

                timer = 0.0f;
                if (!enemy->movementComponent.moving)
                {
                    enemy->placeUnit(enemy->sprite.getPosition().x, enemy->sprite.getPosition().y);
                    if (state == ATTACK)
                    {
                        //When the enemy attacks, it should show an indicator of what unit it is attacking, and there should be a small delay
                        //before the battle actually starts
                        auto otherStats = otherUnit->CalculateBattleStats();
                        auto weapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
                        otherUnit->CalculateMagicDefense(weapon, otherStats, attackRange);
                        battleManager.SetUp(enemy, otherUnit, battleStats, otherStats, canCounter, camera);
                    }
                    else if (state == CANTO || state == APPROACHING)
                    {
                        FinishMove();
                    }
                    else if (state == HEALING)
                    {
                        displays->EnemyUse(enemy, healIndex);
                    }
                    else if (state == TRADING)
                    {
                        displays->EnemyTrade(this);
                    }
                }
            }
        }
    }
}

void EnemyManager::StationaryUpdate(Unit* enemy, BattleManager& battleManager, Camera& camera)
{
    auto otherUnits = enemy->inRangeUnits(0);
    auto position = enemy->sprite.getPosition();
    if (otherUnits.size() > 0)
    {
        otherUnit = nullptr;

        battleStats = BattleStats{};

        int cannotCounterBonus = 50;

        Target finalTarget;
        for (int i = 0; i < otherUnits.size(); i++)
        {
            Target currentTarget;

            auto otherUnit = otherUnits[i];
            auto otherWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
            currentTarget.ID = i;
            //Next want to check how much damage this enemy can do to the other unit
            //If the enemy is already trying to target an enemy that cannot counter, only want to consider using weapons of the same range
            BattleStats tempStats;
            int maxDamage = 0;

            float attackDistance = abs(enemy->sprite.getPosition().x - otherUnit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - otherUnit->sprite.getPosition().y);
            attackDistance /= TileManager::TILE_SIZE;
            if (!(otherWeapon.maxRange >= attackDistance && otherWeapon.minRange <= attackDistance))
            {
                currentTarget.priority += cannotCounterBonus;
            }
            for (int c = 0; c < enemy->weapons.size(); c++)
            {
                auto weapon = enemy->GetWeaponData(enemy->weapons[c]);

                if (weapon.minRange == attackDistance || weapon.maxRange == attackDistance)
                {
                    auto weaponData = enemy->GetWeaponData(enemy->weapons[c]);


                    tempStats = enemy->CalculateBattleStats(enemy->weapons[c]->ID);

                    //Okay, so as this is written, an enemy with a magic sword will prefer to attack from range regardless of if attacking
                    //at a closer ranger would do more damage.
                    //At the very least, I think if a close range attack would kill, the enemy should do that, but it's not in yet.
                    enemy->CalculateMagicDefense(weapon, tempStats, attackDistance);
                    int otherDefense = tempStats.attackType == 0 ? otherUnit->defense : otherUnit->magic;
                    int damage = tempStats.attackDamage - otherDefense;
                    if (damage > maxDamage)
                    {
                        //prioritize sure kills
                        if (otherUnit->currentHP - damage <= 0)
                        {
                            maxDamage = 50;
                        }
                        else
                        {
                            maxDamage = damage;
                        }
                        currentTarget.battleStats = tempStats;
                        currentTarget.range = attackDistance;
                        currentTarget.weaponToUse = enemy->weapons[c];
                    }
                }
            }
            currentTarget.priority += maxDamage;
            if (currentTarget.priority > finalTarget.priority)
            {
                finalTarget = currentTarget;
            }
            //The way this is written, an equal priority would suggest this enemy will do the same amount of damage to multiple units.
            //In that case, check which unit will do less damage on counter
            //Priority of 0 at this point would suggest zero damage will be dealt
            //I do want enemies to be able to attack even if they won't do any damage, so I'll need to rethink this.
            else if (currentTarget.priority == finalTarget.priority)
            {
                auto previousUnit = otherUnits[finalTarget.ID];
                auto previousWeapon = previousUnit->GetWeaponData(previousUnit->GetEquippedItem());
                auto foeWeapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
                auto previousStats = previousUnit->CalculateBattleStats();
                previousUnit->CalculateMagicDefense(previousWeapon, previousStats, finalTarget.range);
                auto otherStats = otherUnit->CalculateBattleStats();
                otherUnit->CalculateMagicDefense(foeWeapon, otherStats, currentTarget.range);

                int previousDamage = previousStats.attackDamage - (previousStats.attackType == 0 ? enemy->defense : enemy->magic);
                int damageTaken = otherStats.attackDamage - (otherStats.attackType == 0 ? enemy->defense : enemy->magic);
                if (damageTaken < previousDamage)
                {
                    finalTarget = currentTarget;
                }
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

            auto otherStats = otherUnit->CalculateBattleStats();
            auto weapon = otherUnit->GetWeaponData(otherUnit->GetEquippedItem());
            attackRange = finalTarget.range;
            otherUnit->CalculateMagicDefense(weapon, otherStats, attackRange);
            battleStats = finalTarget.battleStats;
            bool canCounter = false;

            if (weapon.maxRange >= attackRange && weapon.minRange <= attackRange)
            {
                canCounter = true;
            }
            battleManager.SetUp(enemy, otherUnit, battleStats, otherStats, canCounter, camera);
        }
    }
    else
    {
        NoMove(enemy, position);
    }
}

void EnemyManager::RangeActivation(Unit* enemy)
{
    std::vector<Unit*> otherUnits;
    //Not crazy about this. Works for now
    int range = enemy->move + enemy->maxRange + 1;
    if (enemy->activationType > 1)
    {
        range = enemy->move + enemy->maxRange;
    }
    auto path = enemy->FindApproachMoveRange(otherUnits, range);
    if (otherUnits.size() > 0)
    {
        //It's possible a unit has moved into the attackable range.
        //Check that by checking if the move cost to them is less than the enemy move cost + max range
        //If they are attackable, attack them and activate
        std::vector<Unit*> attackableUnits;
        attackableUnits.reserve(otherUnits.size());
        for (int i = 0; i < otherUnits.size(); i++)
        {
            auto position = enemy->sprite.getPosition();
            auto p = otherUnits[i]->sprite.getPosition();
            auto distance = (abs(position.x - p.x) + abs(position.y - p.y)) / TileManager::TILE_SIZE;
            if (distance <= enemy->move + enemy->maxRange)
            {
                attackableUnits.push_back(otherUnits[i]);
            }
        }
        if (attackableUnits.size() > 0)
        {
            //Really, really don't like refinding the path here
            //I had initially wanted to reuse the path calculated above, but it's increased range makes that not really tenable
            path = enemy->FindUnitMoveRange();
            std::vector<Unit*> otherUnits = GetOtherUnits(enemy);
            GetPriority(enemy, path, otherUnits);
            enemy->active = true;
        }
        //If enemies were in the activate range, but not in attack range, approach the closest one.
        else
        {
            int closest = 1000;
            glm::vec2 otherPosition;
            for (int i = 0; i < otherUnits.size(); i++)
            {
                auto p = otherUnits[i]->sprite.getPosition();
                auto moveCost = path[p].moveCost;
                if (moveCost < closest)
                {
                    closest = moveCost;
                    otherUnit = otherUnits[i];
                    otherPosition = p;
                }
            }
            std::vector<glm::ivec2> followPath;
            glm::vec2 pathPoint = otherPosition;
            while (path[pathPoint].moveCost > enemy->move)
            {
                pathPoint = path[pathPoint].previousPosition;
            }
            followPath.push_back(pathPoint);

            while (pathPoint != enemy->sprite.getPosition())
            {
                auto previous = path[pathPoint].previousPosition;
                followPath.push_back(previous);
                pathPoint = previous;
            }
            state = APPROACHING;
            auto position = enemy->sprite.getPosition();
            TileManager::tileManager.removeUnit(position.x, position.y);
            enemy->movementComponent.startMovement(followPath, path[otherPosition].moveCost, false);
            enemyMoving = true;
            enemy->active = true;
        }
    }
    //If no enemies are in the active range, do nothing this turn
    else
    {
        auto position = enemy->sprite.getPosition();
        NoMove(enemy, position);
    }
}

void EnemyManager::FindUnitInAttackRange(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path)
{
    std::vector<Unit*> otherUnits = GetOtherUnits(enemy);

    //If not in range of any units, enemy approaches the nearest unit
    if (otherUnits.size() == 0)
    {
        auto position = enemy->sprite.getPosition();
        TileManager::tileManager.removeUnit(position.x, position.y);
        ApproachNearest(position, enemy);
    }
    else
    {
        GetPriority(enemy, path, otherUnits);
    }
}

void EnemyManager::FindHealItem(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path)
{
    auto position = enemy->sprite.getPosition();

    TileManager::tileManager.removeUnit(position.x, position.y);
    //Check nearby friendly units to see if they have a healing item. If they do, trade with them, heal, and end move.
    Unit* closestFriendly = nullptr;
    glm::vec2 closestPosition;
    int minDistance = 100;
    for (int i = 0; i < enemy->tradeUnits.size(); i++)
    {
        int possibleIndex = -1;

        auto tradeUnit = enemy->tradeUnits[i];

        for (int c = 0; c < tradeUnit->inventory.size(); c++)
        {
            if (tradeUnit->inventory[c]->ID == 0)
            {
                possibleIndex = c;
                break;
            }
        }
        if (possibleIndex >= 0)
        {
            auto adjacentPositions = ValidAdjacentPositions(tradeUnit, path, 1, 1);
            for (int c = 0; c < adjacentPositions.size(); c++)
            {
                auto distance = path[adjacentPositions[c].position].moveCost;
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestPosition = adjacentPositions[c].position;
                    closestFriendly = tradeUnit;
                    healIndex = possibleIndex;
                }
            }
        }
    }
    if (closestFriendly)
    {
        std::vector<glm::ivec2> followPath;
        glm::vec2 pathPoint = closestPosition;
        followPath.push_back(pathPoint);

        while (pathPoint != enemy->sprite.getPosition())
        {
            auto previous = path[pathPoint].previousPosition;
            followPath.push_back(previous);
            pathPoint = previous;
        }
        //Add the healing item to this enemy's inventory. If inventory is full, swap last item.
        int tradeIndex = enemy->inventory.size();
        if (tradeIndex > 8)
        {
            tradeIndex--;
        }
        //Swapping here so I don't have to keep track of this data outside of this function
        //Still going to need some way of informing the player that the enemy made a trade though.
        enemy->swapItem(closestFriendly, healIndex, tradeIndex);
        healIndex = tradeIndex;
        enemy->movementComponent.startMovement(followPath, path[closestPosition].moveCost, false);
        enemyMoving = true;
        state = TRADING;
    }
    else
    {
        std::vector<Unit*> otherUnits = GetOtherUnits(enemy);

        //If not in range of any units, enemy approaches the nearest unit
        if (otherUnits.size() == 0)
        {
            auto position = enemy->sprite.getPosition();
            ApproachNearest(position, enemy);
        }
        else
        {
            GetPriority(enemy, path, otherUnits);
        }
    }
}

void EnemyManager::HealSelf(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path)
{
    state = HEALING;
    auto position = enemy->sprite.getPosition();

    TileManager::tileManager.removeUnit(position.x, position.y);
    
    std::vector<Unit*> otherUnits = GetOtherUnits(enemy);

    //want to find the closest player unit and move away from it.
    Unit* closestUnit = nullptr;
    int closestDistance = 20;
    for (int i = 0; i < otherUnits.size(); i++)
    {
        int distance = path[otherUnits[i]->sprite.getPosition()].moveCost;
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestUnit = otherUnits[i];
        }
    }
    if (closestUnit == nullptr)
    {
        //No units nearby, just heal
        NoMove(enemy, position);
    }
    else
    {
        auto directionToOtherUnit = glm::normalize(glm::vec2(position - closestUnit->sprite.getPosition()));

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
                //Try to run away from the closest unit, but prioritize good tiles
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
            //This would mean that all valid tiles to escape to are occupied by allies, so this unit cannot move
            //This is untested right now.
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
            enemy->movementComponent.startMovement(followPath, path[bestPosition].moveCost, false);
            enemyMoving = true;
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
    //No good spot to canto to
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
        enemies[i]->EndTurn();
    }
    subject.notify(1);
    std::cout << "Player Turn Start\n";

}

void EnemyManager::RemoveDeadUnits()
{
    for (int i = 0; i < enemies.size(); i++)
    {
        if (enemies[i]->isDead)
        {
            enemies.erase(enemies.begin() + i);
            i--;
        }
    }
}

void EnemyManager::Clear()
{
    for (int i = 0; i < enemies.size(); i++)
    {
        delete enemies[i];
    }
    enemies.clear();
}

Unit* EnemyManager::GetCurrentUnit()
{
    if (currentEnemy < enemies.size())
    {
        return enemies[currentEnemy];
    }
    return nullptr;
}

std::vector<AttackPosition> EnemyManager::ValidAdjacentPositions(Unit* toAttack, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path,
    int minRange, int maxRange)
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
        glm::vec2 checkPosition = current.position;

        glm::vec2 up = glm::vec2(checkPosition.x, checkPosition.y - 1);
        glm::vec2 down = glm::vec2(checkPosition.x, checkPosition.y + 1);
        glm::vec2 left = glm::vec2(checkPosition.x - 1, checkPosition.y);
        glm::vec2 right = glm::vec2(checkPosition.x + 1, checkPosition.y);

        CheckAdjacentTiles(up, checked, checking, current, costs, foundTiles, rangeTiles, path, minRange, maxRange);
        CheckAdjacentTiles(down, checked, checking, current, costs, foundTiles, rangeTiles, path, minRange, maxRange);
        CheckAdjacentTiles(right, checked, checking, current, costs, foundTiles, rangeTiles, path, minRange, maxRange);
        CheckAdjacentTiles(left, checked, checking, current, costs, foundTiles, rangeTiles, path, minRange, maxRange);
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

void EnemyManager::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, 
    pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles,
    const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, int minRange, int maxRange)
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
            if (movementCost <= maxRange)
            {
                if ((minRange == maxRange && movementCost == maxRange) ||
                    (minRange < maxRange && movementCost <= maxRange))
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

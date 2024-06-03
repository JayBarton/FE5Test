#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"
#include "TileManager.h"
#include "BattleManager.h"
#include "Camera.h"
#include "PathFinder.h"
#include "SBatch.h"
#include "Vendor.h"

#include <algorithm>
#include <fstream>  
#include "InfoDisplays.h"
#include "csv.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void EnemyManager::SetUp(std::ifstream& map, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, std::vector<Unit*>* playerUnits, std::vector<Vendor>* vendors)
{
    UVs.resize(3);
    UVs[0] = ResourceManager::GetTexture("sprites").GetUVs(0, 16, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
    auto extras = ResourceManager::GetTexture("movesprites").GetUVs(640, 128, 32, 32, 4, 4);
    UVs[0].insert(UVs[0].end(), extras.begin(), extras.end());
    UVs[1] = ResourceManager::GetTexture("sprites").GetUVs(48, 48, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
    extras = ResourceManager::GetTexture("movesprites").GetUVs(768, 128, 32, 32, 4, 4);
    UVs[1].insert(UVs[1].end(), extras.begin(), extras.end());
    UVs[2] = ResourceManager::GetTexture("sprites").GetUVs(96, 0, TileManager::TILE_SIZE, TileManager::TILE_SIZE, 3, 1);
    extras = ResourceManager::GetTexture("movesprites").GetUVs(256, 0, 32, 32, 4, 4);
    UVs[2].insert(UVs[2].end(), extras.begin(), extras.end());
    std::vector<Unit> unitBases;
    unitBases.resize(4);
    this->playerUnits = playerUnits;
    this->vendors = vendors;

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
        unitBases[currentUnit] = Unit(name, name, ID, HP, str, mag, skl, spd, lck, def, bld, mov);

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

        map >> enemies[i]->sceneID;

        for (int c = 0; c < inventorySize; c++)
        {
            enemies[i]->addItem(inventory[c]);
        }

        enemies[i]->placeUnit(position.x, position.y);
        enemies[i]->sprite.uv = &UVs[enemies[i]->ID];
        //I'm thinking I will move the anim data to its own json, so I won't need to go through here again.
        for (const auto& enemy : bases)
        {
            int ID = enemy["ID"];
            if (ID == enemies[i]->ID)
            {
                auto animData = enemy["AnimData"];
                enemies[i]->focusedFacing = animData["FocusFace"];
            }
        }
    }
    //  enemies[14]->currentHP = 14;
    //  enemies[0]->move = 6;
   //   enemies[0]->mount = new Mount(Unit::HORSE, 1, 1, 1, 2, 3);
}

void EnemyManager::Update(float deltaTime, BattleManager& battleManager, Camera& camera, InputManager& inputManager)
{
    if (currentEnemy >= enemies.size())
    {
        EndTurn();
    }
    else
    {
        auto enemy = enemies[currentEnemy];

        if (enemyMoving)
        {
            followCamera = true;
            if (!canAct)
            {
                timer += deltaTime;
                if (timer >= actionDelay)
                {
                    timer = 0.0f;
                    canAct = true;
                }
            }
            else
            {
                enemy->UpdateMovement(deltaTime, inputManager);
                if (!enemy->movementComponent.moving)
                {
                    enemy->placeUnit(enemy->sprite.getPosition().x, enemy->sprite.getPosition().y);
                    if (state == ATTACK)
                    {
                        //When the enemy attacks, it should show an indicator of what unit it is attacking, and there should be a small delay
                        //before the battle actually starts
                        auto otherStats = otherUnit->CalculateBattleStats();
                        auto weapon = otherUnit->GetEquippedWeapon();
                        otherUnit->CalculateMagicDefense(weapon, otherStats, attackRange);
                        battleManager.SetUp(enemy, otherUnit, battleStats, otherStats, canCounter, camera, true);
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
                    else if (state == SHOPPING)
                    {
                        if (enemy->sprite.getPosition() == enemy->storeTarget->position)
                        {
                            //buy the item cheapest item this enemy can use
                            int cheapest = 10000;
                            int selectedItem = -1;
                            for (int i = 0; i < enemy->storeTarget->items.size(); i++)
                            {
                                auto currentItem = enemy->storeTarget->items[i];
                                if (enemy->canUse(currentItem))
                                {
                                    if (ItemManager::itemManager.items[currentItem].value < cheapest)
                                    {
                                        cheapest = ItemManager::itemManager.items[i].value;
                                        selectedItem = currentItem;
                                    }
                                }
                            }
                            enemy->addItem(selectedItem);
                            enemy->storeTarget = nullptr;
                            displays->EnemyBuy(this);
                        }
                        else
                        {
                            FinishMove();
                        }
                    }
                }
            }
        }
        else
        {
            timer += deltaTime;
            if (timer >= turnStartDelay)
            {
                timer = 0.0f;
                skippedUnit = false;
                while (currentEnemy < enemies.size() && !skippedUnit && !enemyMoving)
                {
                    if (enemy->isCarried)
                    {
                        NextUnit();
                        skippedUnit = true;
                    }
                    else if (enemy->stationary)
                    {
                        StationaryUpdate(enemy, battleManager, camera);
                    }
                    else
                    {
                        DefaultUpdate(deltaTime, enemy, camera, battleManager);
                    }
                }
            }
        }
    }
}


void EnemyManager::DefaultUpdate(float deltaTime, Unit* enemy, Camera& camera, BattleManager& battleManager)
{
    if (!enemyMoving)
    {
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
            //Want to do the store check here
            //Not a huge fan of this, but it works for now
            else if (enemy->GetEquippedItem() || !CheckStores(enemy))
            {
                FindUnitInAttackRange(enemy, path, camera);
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
                    camera.SetMove(enemy->sprite.getPosition());
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
                    camera.SetMove(enemy->sprite.getPosition());
                }
            }
        }
    }
}

bool EnemyManager::CheckStores(Unit* enemy)
{ 
    auto position = enemy->sprite.getPosition();

    if (!enemy->storeTarget)
    {
        std::vector<Vendor*> usableVendors;
        usableVendors.reserve(vendors->size());
        for (int i = 0; i < vendors->size(); i++)
        {
            auto vendor = (*vendors)[i];
            for (int c = 0; c < vendor.items.size(); c++)
            {
                if (enemy->canUse(vendor.items[c]))
                {
                    usableVendors.push_back(&(*vendors)[i]);
                    break;
                }
            }
        }
        float closest = 10000;
        for (int i = 0; i < usableVendors.size(); i++)
        {
            //Going to want findPath to return a path distance so I can tell which vendor is closest
            // For now I will just use good ol' Manhattan
            // pathFinder.findPath(position, usableVendors[i].position, 100);
            float distance = abs(position.x - usableVendors[i]->position.x) + abs(position.y - usableVendors[i]->position.y);
            if (distance < closest)
            {
                closest = distance;
                enemy->storeTarget = usableVendors[i];
            }
        }
        if (enemy->storeTarget)
        {
            GoShopping(position, enemy);
            return true;
        }
    }
    else
    {
        GoShopping(position, enemy);
        return true;
    }
    return false;
}

void EnemyManager::GoShopping(glm::vec2& position, Unit* enemy)
{
    TileManager::tileManager.removeUnit(position.x, position.y);
    auto path = pathFinder.findPath(position, enemy->storeTarget->position, enemy->getMove());
    enemy->startMovement(path, enemy->getMove(), false);
    enemyMoving = true;
    enemy->active = true;
    state = SHOPPING;
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
            auto otherWeapon = otherUnit->GetEquippedWeapon();
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
                    int otherDefense = tempStats.attackType == 0 ? otherUnit->getDefense() : otherUnit->getMagic();
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
                auto previousWeapon = previousUnit->GetEquippedWeapon();
                auto foeWeapon = otherUnit->GetEquippedWeapon();
                auto previousStats = previousUnit->CalculateBattleStats();
                previousUnit->CalculateMagicDefense(previousWeapon, previousStats, finalTarget.range);
                auto otherStats = otherUnit->CalculateBattleStats();
                otherUnit->CalculateMagicDefense(foeWeapon, otherStats, currentTarget.range);

                int previousDamage = previousStats.attackDamage - (previousStats.attackType == 0 ? enemy->getDefense() : enemy->getMagic());
                int damageTaken = otherStats.attackDamage - (otherStats.attackType == 0 ? enemy->getDefense() : enemy->getMagic());
                if (damageTaken < previousDamage)
                {
                    finalTarget = currentTarget;
                }
            }
        }

        //In range of units but cannot reach any of them, stay where you are
        if (finalTarget.priority == 0)
        {
            DoNothing(enemy, position);
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
            auto weapon = otherUnit->GetEquippedWeapon();
            attackRange = finalTarget.range;
            otherUnit->CalculateMagicDefense(weapon, otherStats, attackRange);
            battleStats = finalTarget.battleStats;
            bool canCounter = false;
            skippedUnit = true;
            if (weapon.maxRange >= attackRange && weapon.minRange <= attackRange)
            {
                canCounter = true;
            }
            battleManager.SetUp(enemy, otherUnit, battleStats, otherStats, canCounter, camera, true);
        }
    }
    //No units in range
    else
    {
        DoNothing(enemy, position);
    }
}

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
        auto otherWeapon = otherUnit->GetEquippedWeapon();
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

                        int otherDefense = tempStats.attackType == 0 ? otherUnit->getDefense() : otherUnit->getMagic();

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

                        int otherDefense = tempStats.attackType == 0 ? otherUnit->getDefense() : otherUnit->getMagic();

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
            //The way this is written, an equal priority would suggest this enemy will do the same amount of damage to multiple units.
            //In that case, check which unit will do less damage on counter
            //Priority of 0 at this point would suggest zero damage will be dealt
            //I do want enemies to be able to attack even if they won't do any damage, so I'll need to rethink this.
            else if (currentTarget.priority == finalTarget.priority)
            {
                auto previousUnit = otherUnits[finalTarget.ID];
                auto previousWeapon = previousUnit->GetEquippedWeapon();
                auto foeWeapon = otherUnit->GetEquippedWeapon();
                auto previousStats = previousUnit->CalculateBattleStats();
                previousUnit->CalculateMagicDefense(previousWeapon, previousStats, finalTarget.range);
                auto otherStats = otherUnit->CalculateBattleStats();
                otherUnit->CalculateMagicDefense(foeWeapon, otherStats, currentTarget.range);

                int previousDamage = previousStats.attackDamage - (previousStats.attackType == 0 ? enemy->getDefense() : enemy->getMagic());
                int damageTaken = otherStats.attackDamage - (otherStats.attackType == 0 ? enemy->getDefense() : enemy->getMagic());
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
        DoNothing(enemy, position);
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
        enemy->startMovement(followPath, path[attackPosition].moveCost, false);
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
    auto path = pathFinder.findPath(position, playerUnit->sprite.getPosition(), enemy->getMove());
    enemy->startMovement(path, enemy->getMove(), false);
    enemyMoving = true;
}

void EnemyManager::NoMove(Unit* enemy, glm::vec2& position)
{
    enemy->placeUnit(position.x, position.y);
    NextUnit();
    skippedUnit = true;
}

void EnemyManager::NextUnit()
{
    currentEnemy++;
    canAct = false;
    followCamera = false;
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

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}

void EnemyManager::Draw(SBatch* Batch)
{
    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->Draw(Batch);
    }
}

void EnemyManager::RangeActivation(Unit* enemy)
{
    std::vector<Unit*> otherUnits;
    //Not crazy about this. Works for now
    int range = enemy->getMove() + enemy->maxRange + 1;
    if (enemy->activationType > 1)
    {
        range = enemy->getMove() + enemy->maxRange;
    }
    auto path = enemy->FindApproachMoveRange(otherUnits, range);
    if (otherUnits.size() > 0)
    {
        //Not a huge fan of this, but it works for now
        if (enemy->GetEquippedItem() || !CheckStores(enemy))
        {
            //It's possible a unit has moved into the attackable range.
            //Check that by checking if the move cost to them is less than the enemy move cost + max range
            //If they are attackable, attack them and activate
            auto attackPath = enemy->FindUnitMoveRange();
            std::vector<Unit*> attackableUnits = GetOtherUnits(enemy);
            if (attackableUnits.size() > 0)
            {
                //Really, really don't like refinding the path here
                //I had initially wanted to reuse the path calculated above, but it's increased range makes that not really tenable
                GetPriority(enemy, attackPath, attackableUnits);
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
                while (path[pathPoint].moveCost > enemy->getMove())
                {
                    pathPoint = path[pathPoint].previousPosition;
                }
                //Need to make sure to cut short a path if it ends on a tile occupied by another unit
                bool blocked = true;
                while (blocked)
                {
                    blocked = false;
                    auto thisTile = TileManager::tileManager.getTile(pathPoint.x, pathPoint.y);
                    if (thisTile)
                    {
                        if (thisTile->occupiedBy)
                        {
                            pathPoint = path[pathPoint].previousPosition;
                            blocked = true;
                        }
                    }
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
                enemy->startMovement(followPath, path[otherPosition].moveCost, false);
                enemyMoving = true;
                enemy->active = true;
            }
        }
    }
    //If no enemies are in the active range, do nothing this turn
    else
    {
        auto position = enemy->sprite.getPosition();
        DoNothing(enemy, position);
    }
}

//Want to skip the current enemy's turn, so set the timer to its end value
void EnemyManager::DoNothing(Unit* enemy, glm::vec2& position)
{
    NoMove(enemy, position);
    skippedUnit = true;
    timer = turnStartDelay;
}

void EnemyManager::FindUnitInAttackRange(Unit* enemy, std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, Camera& camera)
{
    std::vector<Unit*> otherUnits = GetOtherUnits(enemy);

    //If not in range of any units, enemy approaches the nearest unit
    if (otherUnits.size() == 0)
    {
        auto position = enemy->sprite.getPosition();
        TileManager::tileManager.removeUnit(position.x, position.y);
        ApproachNearest(position, enemy);
        if (enemyMoving)
        {
            camera.SetMove(enemy->sprite.getPosition());
        }
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
        enemy->startMovement(followPath, path[closestPosition].moveCost, false);
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
            enemy->startMovement(followPath, path[bestPosition].moveCost, false);
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
        enemy->startMovement(followPath, path[bestPosition].moveCost, true);
    }
}

void EnemyManager::FinishMove()
{
    enemies[currentEnemy]->hasMoved = true;
    enemies[currentEnemy]->sprite.moveAnimate = false;
    enemyMoving = false;
    NextUnit();

    state = GET_TARGET;
}

void EnemyManager::UpdateEnemies(float deltaTime, int idleFrame)
{
    for (int i = 0; i < enemies.size(); i++)
    {
        enemies[i]->Update(deltaTime, idleFrame);
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

void EnemyManager::RemoveDeadUnits(std::unordered_map<int, Unit*>& sceneUnits)
{
    for (int i = 0; i < enemies.size(); i++)
    {
        if (enemies[i]->isDead)
        {
            sceneUnits.erase(enemies[i]->sceneID);
            delete enemies[i];
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
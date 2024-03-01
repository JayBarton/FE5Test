#include "Unit.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include "TileManager.h"
#include "Items.h"

Unit::Unit()
{

}

Unit::~Unit()
{
  /*  for (int i = 0; i < inventory.size(); i++)
    {
        delete inventory[i];
    }
    if (mount)
    {
        delete mount;
    }*/
}

void Unit::init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
    this->gen = gen;
    this->distribution = distribution;
    sprite.setSize(glm::vec2(16));
    movementComponent.owner = this;
}

void Unit::placeUnit(int x, int y)
{
    TileManager::tileManager.placeUnit(x, y, this);
    sprite.SetPosition(glm::vec2(x, y));
}

void Unit::Update(float deltaTime)
{
    if (movementComponent.moving)
    {
        movementComponent.Update(deltaTime);
    }
}

void Unit::Draw(SpriteRenderer* Renderer)
{
    ResourceManager::GetShader("sprite").Use();
    glm::vec3 color = sprite.color;
    glm::vec4 colorAndAlpha = glm::vec4(color.x, color.y, color.z, sprite.alpha);

    glm::vec2 position = sprite.getPosition();
    Renderer->setUVs(sprite.getUV());
    Texture2D texture = ResourceManager::GetTexture("sprites");

    Renderer->DrawSprite(texture, position, 0.0f, sprite.getSize(), colorAndAlpha, hasMoved);
}

void Unit::LevelUp()
{
    if (team == 0)
    {
        subject.notify(this);
    }

    level++;
    int roll[9];
    for (int i = 0; i < 9; i++)
    {
        roll[i] = (*distribution)(*gen);
    }
    //roll should be a random number, rerolled for each stat
    if (roll[0] <= growths.maxHP)
    {
        maxHP++;
        currentHP++;
    }
    if (roll[1] <= growths.strength)
    {
        strength++;
    }
    if (roll[2] <= growths.magic)
    {
        magic++;
    }
    if (roll[3] <= growths.skill)
    {
        skill++;
    }
    if (roll[4] <= growths.speed)
    {
        speed++;
    }
    if (roll[5] <= growths.defense)
    {
        defense++;
    }
    if (roll[6] <= growths.build)
    {
        build++;
    }
    if (roll[7] <= growths.move)
    {
        move++;
    }
    if (roll[8] <= growths.luck)
    {
        luck++;
    }
}

void Unit::AddExperience(int exp)
{
    experience += exp;
    if (experience >= 100)
    {
        experience -= 100;
        LevelUp();
    }
}

void Unit::LevelEnemy(int level)
{
    //According to https://serenesforest.net/thracia-776/miscellaneous/calculations/ Enemies recieve these bonuses. I do not believe these bonuses are
    //per level, as that would be absurd, so I am simply calculating them once here.
    std::uniform_int_distribution<int> enemyBonus(1, 4);
    int roll[9];
    for (int i = 0; i < 8; i++)
    {
        roll[i] = enemyBonus(*gen);
    }
    roll[8] = (*distribution)(*gen);
    maxHP += 4 - roll[0];
    currentHP = maxHP;
    strength += 4 - roll[1];
    magic += 4 - roll[2];
    skill += 4 - roll[3];
    speed += 4 - roll[4];
    defense += 4 - roll[5];
    build += 4 - roll[6];
    luck += 4 - roll[7];
    if (roll[8] <= 10)
    {
        move++;
    }
    for (int i = 0; i < level; i++)
    {
        LevelUp();
    }
}

//I don't know, this is duplicated from Cursor, I need it to check Charisma
//I'm wondering if it might be easier to just record every unit who has Charisma in the battle, and then just check if they are nearby
//This has the same bug found in the cursor's trade range function, in that it can pick up the unit I am checking nearby units for because
//The unit has technically not moved tiles until a move has been confirmed.
std::vector<Unit*> Unit::inRangeUnits(int minRange, int maxRange, int team)
{
    std::vector<Unit*> units;
    
    glm::ivec2 position = glm::ivec2(sprite.getPosition());

    for (int i = minRange; i < maxRange + 1; i++)
    {
        glm::ivec2 up = glm::ivec2(position.x, position.y - i * TileManager::TILE_SIZE);
        glm::ivec2 down = glm::ivec2(position.x, position.y + i * TileManager::TILE_SIZE);
        glm::ivec2 left = glm::ivec2(position.x - i * TileManager::TILE_SIZE, position.y);
        glm::ivec2 right = glm::ivec2(position.x + i * TileManager::TILE_SIZE, position.y);
        if (Unit* unit = TileManager::tileManager.getUnitOnTeam(up.x, up.y, team))
        {
            units.push_back(unit);
        }
        for (int c = minRange; c < maxRange + 1 - i; c++)
        {
            glm::ivec2 upLeft = glm::ivec2(up.x - c * TileManager::TILE_SIZE, up.y);
            glm::ivec2 upRight = glm::ivec2(up.x + c * TileManager::TILE_SIZE, up.y);
            if (Unit* unit = TileManager::tileManager.getUnitOnTeam(upLeft.x, upLeft.y, team))
            {
                units.push_back(unit);
            }
            if (Unit* unit = TileManager::tileManager.getUnitOnTeam(upRight.x, upRight.y, team))
            {
                units.push_back(unit);
            }
        }
        if (Unit* unit = TileManager::tileManager.getUnitOnTeam(down.x, down.y, team))
        {
            units.push_back(unit);
        }
        for (int c = minRange; c < maxRange + 1 - i; c++)
        {
            glm::ivec2 downLeft = glm::ivec2(down.x - c * TileManager::TILE_SIZE, down.y);
            glm::ivec2 downRight = glm::ivec2(down.x + c * TileManager::TILE_SIZE, down.y);
            if (Unit* unit = TileManager::tileManager.getUnitOnTeam(downLeft.x, downLeft.y, team))
            {
                units.push_back(unit);
            }
            if (Unit* unit = TileManager::tileManager.getUnitOnTeam(downRight.x, downRight.y, team))
            {
                units.push_back(unit);
            }
        }
        if (Unit* unit = TileManager::tileManager.getUnitOnTeam(left.x, left.y, team))
        {
            units.push_back(unit);
        }
        if (Unit* unit = TileManager::tileManager.getUnitOnTeam(right.x, right.y, team))
        {
            units.push_back(unit);
        }
    }

    return units;
}

void Unit::addItem(int ID)
{
    if (inventory.size() < INVENTORY_SLOTS)
    {
        auto newItem = new Item (ItemManager::itemManager.items[ID]);
        inventory.push_back(newItem);
        
        if (newItem->isWeapon)
        {
            auto weapon = GetWeaponData(newItem);
            if (canUse(weapon))
            {
                if (weapon.maxRange > maxRange)
                {
                    maxRange = weapon.maxRange;
                }
                if (weapon.minRange < minRange)
                {
                    minRange = weapon.minRange;
                }
                weapons.push_back(newItem);

                if (equippedWeapon < 0)
                {
                    equippedWeapon = inventory.size() - 1;
                 //   tryEquip(inventory.size() - 1);
                }
            }
        }
    }
}

void Unit::dropItem(int index)
{
    for (int i = 0; i < weapons.size(); i++)
    {
        if (inventory[index] == weapons[i])
        {
            weapons.erase(weapons.begin() + i);
            break;
        }
    }
    Item* i = inventory[index];
    inventory.erase(inventory.begin() + index);
    delete i;

    //Check if the range needs to be reset
    //This is a bit clumsy, but it works for now
    CalculateUnitRange();
    if (index == equippedWeapon)
    {
        equippedWeapon = -1;
        findWeapon();
    }
    else if (index < equippedWeapon)
    {
        equippedWeapon--;
    }
}

void Unit::CalculateUnitRange()
{
    maxRange = 0;
    minRange = 5;
    for (int i = 0; i < weapons.size(); i++)
    {
        auto weapon = GetWeaponData(weapons[i]);
        if (canUse(weapon))
        {
            if (weapon.maxRange > maxRange)
            {
                maxRange = weapon.maxRange;
            }
            if (weapon.minRange < minRange)
            {
                minRange = weapon.minRange;
            }
        }
    }
}

void Unit::swapItem(Unit* otherUnit, int otherIndex, int thisIndex)
{
    auto otherInventory = &otherUnit->inventory;
    auto otherItem = (*otherInventory)[otherIndex];
    Item* thisItem = nullptr;
    if (thisIndex < inventory.size())
    {
        thisItem = inventory[thisIndex];

        (*otherInventory)[otherIndex] = thisItem;
        inventory[thisIndex] = otherItem;
    }
    else
    {
        inventory.push_back(otherItem);
        otherInventory->erase(otherInventory->begin() + otherIndex);
    }
    if (otherUnit != this)
    {
        bool recalculateRange = false;
        if (thisItem && thisItem->isWeapon)
        {
            for (int i = 0; i < weapons.size(); i++)
            {
                if (thisItem == weapons[i])
                {
                    weapons.erase(weapons.begin() + i);
                    break;
                }
            }
            otherUnit->weapons.push_back(thisItem);
            recalculateRange = true;
        }
        if (otherItem->isWeapon)
        {
            for (int i = 0; i < otherUnit->weapons.size(); i++)
            {
                if (otherItem == otherUnit->weapons[i])
                {
                    otherUnit->weapons.erase(otherUnit->weapons.begin() + i);
                    break;
                }
            }
            weapons.push_back(otherItem);
            recalculateRange = true;
        }
        if (recalculateRange)
        {
            CalculateUnitRange();
            otherUnit->CalculateUnitRange();
        }
        if (thisIndex == equippedWeapon)
        {
            equippedWeapon = -1;
        }
        if (otherIndex == otherUnit->equippedWeapon)
        {
            otherUnit->equippedWeapon = -1;
        }
        findWeapon();
        otherUnit->findWeapon();
    }
}

void Unit::findWeapon()
{
    if (equippedWeapon == -1)
    {
        for (int i = 0; i < inventory.size(); i++)
        {
            if (inventory[i]->isWeapon)
            {
                if (tryEquip(i))
                {
                    break;
                }
            }
        }
    }
}

void Unit::equipWeapon(int index)
{
    //Equipped weapon will always be in the first slot
    if (inventory.size() > 1)
    {
        auto temp = inventory[0];
        inventory[0] = inventory[index];
        inventory[index] = temp;
        equippedWeapon = 0;
    }
}

bool Unit::tryEquip(int index)
{
    auto weapon = GetWeaponData(inventory[index]);
    if (canUse(weapon))
    {
        equippedWeapon = index;
        return true;
    }
    return false;
}

bool Unit::canUse(const WeaponData& weapon)
{
    if (weapon.rank > 5)
    {
        auto it = std::find(uniqueWeapons.begin(), uniqueWeapons.end(), weapon.rank);
        if (it != uniqueWeapons.end())
        {
            return true;
        }
    }
    return weapon.rank <= weaponProficiencies[weapon.type];
}

bool Unit::hasSkill(int ID)
{
    auto it = std::find(skills.begin(), skills.end(), ID);
    if (it != skills.end())
    {
        return true;
    }
    return false;
}

int Unit::getMovementType()
{
    if (mount && mount->mounted)
    {
        return mount->movementType;
    }
    return movementType;
}

void Unit::MountAction(bool on)
{
    if (mount)
    {
        if (on)
        {
            mount->mounted = true;
            strength += mount->str;
            skill += mount->skl;
            speed += mount->spd;
            defense += mount->def;
            move += mount->mov;
        }
        else
        {
            mount->mounted = false;
            strength -= mount->str;
            skill -= mount->skl;
            speed -= mount->spd;
            defense -= mount->def;
            move -= mount->mov;
        }
    }
}

Item* Unit::GetEquippedItem()
{
    if (equippedWeapon >= 0)
    {
        return inventory[equippedWeapon];
    }
    return nullptr;
}

BattleStats Unit::CalculateBattleStats(int weaponID)
{
    BattleStats stats;
    int charismaBonus = 0;

    auto nearbyUnits = inRangeUnits(1, 3, team);
    for (int i = 0; i < nearbyUnits.size(); i++)
    {
        if (nearbyUnits[i]->hasSkill(CHARISMA))
        {
            charismaBonus += 10;
        }
    }

    if (weaponID == -1)
    {
        if (equippedWeapon >= 0)
        {
            weaponID = inventory[equippedWeapon]->ID;
        }
        else
        {
            stats.attackDamage = 0;
            stats.hitAccuracy = 0;
            stats.hitCrit = 0;
            stats.attackSpeed = speed;
            stats.hitAvoid = stats.attackSpeed * 2 + luck + charismaBonus;
        }
    }
    if (weaponID >= 0)
    {
        auto weapon = ItemManager::itemManager.weaponData[weaponID];
        stats.attackDamage = weapon.might + (!weapon.isMagic ? strength : magic); //+ mag if the weapon is magic

        stats.hitAccuracy = weapon.hit + skill * 2 + luck + charismaBonus;
        stats.hitCrit = weapon.crit + skill;
        int weight = weapon.weight - (!weapon.isMagic ? build : 0); //No build included if the weapon is magic
        if (weight < 0)
        {
            weight = 0;
        }
        stats.attackSpeed = speed - weight;
        if (stats.attackSpeed < 0)
        {
            stats.attackSpeed = 0;
        }
        stats.hitAvoid = stats.attackSpeed * 2 + luck + charismaBonus;
    }
    return stats;
}

WeaponData Unit::GetWeaponData(Item* item)
{
    if (item)
    {
        return ItemManager::itemManager.weaponData[item->ID];
    }
    return WeaponData();
}

std::unordered_map<glm::vec2, pathCell, vec2Hash> Unit::FindUnitMoveRange()
{
    ClearPathData();
    auto position = sprite.getPosition();

    path[position] = { position, 0, position }; // pretty sure this is also wrong
    std::vector<pathCell> checking;
    std::vector<std::vector<bool>> checked;

    checked.resize(TileManager::tileManager.levelWidth);
    for (int i = 0; i < TileManager::tileManager.levelWidth; i++)
    {
        checked[i].resize(TileManager::tileManager.levelHeight);
    }
    //TODO consider making this a map
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
    foundTiles.push_back(position);
    costTile.push_back(0);
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

        CheckAdjacentTiles(up, checked, checking, current, costs);
        CheckAdjacentTiles(down, checked, checking, current, costs);
        CheckAdjacentTiles(right, checked, checking, current, costs);
        CheckAdjacentTiles(left, checked, checking, current, costs);
    }
    if (maxRange > 0)
    {
        checked.clear();
        checked.resize(TileManager::tileManager.levelWidth);
        for (int i = 0; i < TileManager::tileManager.levelWidth; i++)
        {
            checked[i].resize(TileManager::tileManager.levelHeight);
        }
        for (int i = 0; i < attackTiles.size(); i++)
        {
            auto current = attackTiles[i] / TileManager::TILE_SIZE;
            checked[current.x][current.y] = true;
        }
        std::vector<glm::ivec2> searchingAttacks = attackTiles;
        for (int i = 0; i < searchingAttacks.size(); i++)
        {
            auto current = searchingAttacks[i] / TileManager::TILE_SIZE;
            for (int c = 1; c < maxRange; c++)
            {
                glm::ivec2 up = glm::ivec2(current.x, current.y - c);
                glm::ivec2 down = glm::ivec2(current.x, current.y + c);
                glm::ivec2 left = glm::ivec2(current.x - c, current.y);
                glm::ivec2 right = glm::ivec2(current.x + c, current.y);
                CheckExtraRange(up, checked);
                CheckExtraRange(down, checked);
                CheckExtraRange(left, checked);
                CheckExtraRange(right, checked);
            }
        }
    }
    return path;
}

void Unit::ClearPathData()
{
    foundTiles.clear();
    attackTiles.clear();
    path.clear();
    costTile.clear();
    drawnPath.clear();
}

void Unit::addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs)
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

void Unit::removeFromOpenList(std::vector<pathCell>& checking)
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

void Unit::CheckExtraRange(glm::ivec2& checkingTile, std::vector<std::vector<bool>>& checked)
{
    glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
    if (!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
    {
        if (path.find(tilePosition) == path.end() && checked[checkingTile.x][checkingTile.y] == false)
        {
            checked[checkingTile.x][checkingTile.y] = true;
            attackTiles.push_back(tilePosition);
        }
    }
}

void Unit::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs)
{
    glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
    if (!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
    {
        int mCost = startCell.moveCost;
        auto thisTile = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y);
        int movementCost = mCost + thisTile->properties.movementCost;
        //This is just a test, will not be keeping long term
        if (getMovementType() == Unit::FLYING)
        {
            movementCost = mCost + 1;
        }

        auto distance = costs[checkingTile.x][checkingTile.y];
        if (!checked[checkingTile.x][checkingTile.y])
        {
            auto otherUnit = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y)->occupiedBy;
            //This is horrid
            if (otherUnit && otherUnit != this && otherUnit->team != team)
            {
                movementCost = 100;
                costs[checkingTile.x][checkingTile.y] = movementCost;
                checked[checkingTile.x][checkingTile.y] = true;
            }
            //This is a weird thing that is only needed to get the attack range, I hope to remove it at some point.
            if (movementCost < distance)
            {
                costs[checkingTile.x][checkingTile.y] = movementCost;
            }
            if (movementCost <= move)
            {
                pathCell newCell{ checkingTile, movementCost };
                addToOpenSet(newCell, checking, checked, costs);
                foundTiles.push_back(tilePosition);
                costTile.push_back(movementCost);
                path[tilePosition] = { tilePosition, movementCost, glm::ivec2(startCell.position) * TileManager::TILE_SIZE };
            }
            else
            {
                if (maxRange > 0)
                {
                    //U G H. Doing this to prevent it from adding dupes to the vector
                    if (distance == 50)
                    {
                        attackTiles.push_back(tilePosition);
                    }
                }
            }
        }
    }
}

void MovementComponent::startMovement(const std::vector<glm::ivec2>& path)
{
    this->path = path;
    end = 0;
    current = path.size() - 1;
    moving = true;
    getNewDirection();
}

void MovementComponent::getNewDirection()
{
    if (current == end)
    {
        owner->sprite.SetPosition(path[current]);
        moving = false;
    }
    else
    {
        glm::vec2 previousNode = path[current];
        current--;
        nextNode = path[current];

        glm::vec2 toNext = nextNode - previousNode;
        //need to check for 0, normalizing 0 returns NAN
        if (toNext.x == 0 && toNext.y == 0)
        {
            direction = glm::vec2(0);
        }
        else
        {
            direction = glm::normalize(nextNode - previousNode);
        }
    }
}

void MovementComponent::Update(float deltaTime)
{
    glm::vec2 newPosition = owner->sprite.getPosition() + direction * 5.0f;

    glm::vec2 toNextNode = newPosition - nextNode;
    if (toNextNode == glm::vec2(0))
    {
        owner->sprite.SetPosition(newPosition);
        getNewDirection();
    }
    else
    {
        glm::vec2 directionDifference = toNextNode * direction;
        //If the direction to the next node and the current direction are the same signs, object has overshot the next node
        if (directionDifference.x > 0 || directionDifference.y > 0)
        {
            //Snap object to next node's position
            owner->sprite.SetPosition(nextNode);
            getNewDirection();
            if (moving)
            {
                //Move the object along towards the new next node by the distance that it overshot
                //This is to insure that the object moves the same amount each frame
                float remainingDistance = glm::abs(toNextNode.x) + glm::abs(toNextNode.y);
                owner->sprite.Move(direction * remainingDistance);
            }
        }
        else
        {
            owner->sprite.SetPosition(newPosition);
        }
    }
}

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
    for (int i = 0; i < inventory.size(); i++)
    {
        delete inventory[i];
    }
}

void Unit::init(std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
    this->gen = gen;
    this->distribution = distribution;
    sprite.setSize(glm::vec2(16));
}

void Unit::placeUnit(int x, int y)
{
    TileManager::tileManager.placeUnit(x, y, this);
    sprite.SetPosition(glm::vec2(x, y));
}

void Unit::Update(float deltaTime)
{
}

void Unit::Draw(SpriteRenderer* Renderer)
{
    ResourceManager::GetShader("sprite").Use();
    glm::vec3 color = sprite.color;
    glm::vec4 colorAndAlpha = glm::vec4(color.x, color.y, color.z, sprite.alpha);

    glm::vec2 position = sprite.getPosition();
    Renderer->setUVs(sprite.getUV());
    Texture2D texture = ResourceManager::GetTexture("sprites");

    Renderer->DrawSprite(texture, position, 0.0f, sprite.getSize(), colorAndAlpha, false);
}

void Unit::LevelUp()
{
    if (team == 0)
    {
        subject.notify(*this);
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

void Unit::addItem(int ID)
{
    if (inventory.size() < INVENTORY_SLOTS)
    {
        auto newItem = new Item (ItemManager::itemManager.items[ID]);
        inventory.push_back(newItem);
        
        if (newItem->isWeapon)
        {
            auto weapon = ItemManager::itemManager.weaponData[ID];
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
    maxRange = 0;
    minRange = 5;
    for (int i = 0; i < weapons.size(); i++)
    {
        auto weapon = ItemManager::itemManager.weaponData[weapons[i]->ID];
        if (weapon.maxRange > maxRange)
        {
            maxRange = weapon.maxRange;
        }
        if (weapon.minRange < minRange)
        {
            minRange = weapon.minRange;
        }
    }
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

void Unit::swapItem(Unit* otherUnit, int otherIndex, int thisIndex)
{
    auto otherInventory = &otherUnit->inventory;
    auto otherItem = (*otherInventory)[otherIndex];

    if (thisIndex < inventory.size())
    {
        auto thisItem = inventory[thisIndex];
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
    else
    {
        if (otherIndex == equippedWeapon)
        {
            equippedWeapon = thisIndex;
        }
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
                equippedWeapon = i;
                break;
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

BattleStats Unit::CalculateBattleStats(int weaponID)
{
    BattleStats stats;
    if (weaponID == -1)
    {
        weaponID = inventory[equippedWeapon]->ID;
    }
    if (weaponID == -1)
    {
        return BattleStats();
    }
    else if (weaponID >= 0)
    {
        auto weapon = ItemManager::itemManager.weaponData[weaponID];
        stats.attackDamage = weapon.might + strength; //+ mag if the weapon is magic
        stats.attackDamage = weapon.might + (!weapon.isMagic ? strength : magic); //+ mag if the weapon is magic

        stats.hitAccuracy = weapon.hit + skill * 2 + luck;
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
        stats.hitAvoid = stats.attackSpeed * 2 + luck;
    }
    return stats;
}

WeaponData Unit::GetWeaponData(Item* item)
{
    return ItemManager::itemManager.weaponData[item->ID];
}

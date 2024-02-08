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

    Renderer->DrawSprite(texture, position, 0.0f, sprite.getSize(), colorAndAlpha);
}

void Unit::LevelUp()
{
    subject.notify(*this);

    std::cout << "level up\n";
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
        std::cout << "HP up\n";
    }
    if (roll[1] <= growths.strength)
    {
        strength++;
        std::cout << "str up\n";
    }
    if (roll[2] <= growths.magic)
    {
        magic++;
        std::cout << "mag up\n";
    }
    if (roll[3] <= growths.skill)
    {
        skill++;
        std::cout << "skl up\n";
    }
    if (roll[4] <= growths.speed)
    {
        speed++;
        std::cout << "spd up\n";
    }
    if (roll[5] <= growths.defense)
    {
        defense++;
        std::cout << "def up\n";
    }
    if (roll[6] <= growths.build)
    {
        build++;
        std::cout << "bld up\n";
    }
    if (roll[7] <= growths.move)
    {
        move++;
        std::cout << "mov up\n";
    }
    if (roll[8] <= growths.luck)
    {
        luck++;
        std::cout << "lck up\n";
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
}

void Unit::equipWeapon(int index)
{
    //Equipped weapon will always be in the first slot
    auto temp = inventory[0];
    inventory[0] = inventory[index];
    inventory[index] = temp;
    equippedWeapon = inventory[0]->ID;
}

BattleStats Unit::CalculateBattleStats(int weaponID)
{
    BattleStats stats;
    if (weaponID == -1)
    {
        weaponID = equippedWeapon;
    }
    if (weaponID == -1)
    {
        stats.attackDamage = 0;
        stats.attackSpeed = 0;
        stats.hitAccuracy = 0;
        stats.hitAvoid = 0;
        stats.hitCrit = 0;
    }
    else if (weaponID >= 0)
    {
        auto weapon = ItemManager::itemManager.weaponData[weaponID];
        stats.attackDamage = weapon.might + strength; //+ mag if the weapon is magic
        stats.hitAccuracy = weapon.hit + skill * 2 + luck;
        stats.hitCrit = weapon.crit + skill;
        int weight = weapon.weight - build; //No build included if the weapon is magic
        if (weight < 0)
        {
            weight = 0;
        }
        stats.attackSpeed = speed - weight;
        stats.hitAvoid = stats.attackSpeed * 2 + luck;
    }
    return stats;
}

WeaponData Unit::GetWeaponData(Item* item)
{
    return ItemManager::itemManager.weaponData[item->ID];
}

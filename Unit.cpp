#include "Unit.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include "TileManager.h"
Unit::Unit()
{

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

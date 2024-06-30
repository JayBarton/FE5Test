#include "PlacementModes.h"
#include "MenuManager.h"

void EnemyMode::leftClick(int x, int y)
{
    glm::vec2 mousePosition(x, y);
    if (objects.count(mousePosition) == 0)
    {
        //Open menu here
        MenuManager::menuManager.AddEnemyMenu(this, nullptr);
    }
    else
    {
        MenuManager::menuManager.AddEnemyMenu(this, &objects[mousePosition]);

    }
}

void VendorMode::leftClick(int x, int y)
{
    glm::vec2 mousePosition(x, y);
    if (vendors.count(mousePosition) == 0)
    {
        //Open menu here
        vendors[mousePosition] = Vendor{ std::vector<int>() };
    }
    MenuManager::menuManager.AddVendorMenu(this, &vendors[mousePosition]);
}

void EnemyEscapeMode::leftClick(int x, int y)
{
    position = glm::ivec2(x, y);
}

void SeizeMode::leftClick(int x, int y)
{
    position = glm::ivec2(x, y);
}

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

/*TileMode::TileMode(Object* obj) : EditMode(obj)
{
	maxElement = 5;
	type = TILE;
}*/

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

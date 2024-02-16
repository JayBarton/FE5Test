#include "PlacementModes.h"
#include "MenuManager.h"

void EnemyMode::leftClick(int x, int y)
{
    glm::vec2 mousePosition(x, y);

    if (objects->count(mousePosition) == 0)
    {
        //Open menu here
        MenuManager::menuManager.AddEnemyMenu(this);
    }
}

/*TileMode::TileMode(Object* obj) : EditMode(obj)
{
	maxElement = 5;
	type = TILE;
}*/
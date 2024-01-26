#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include <SDL.h>
#include <iostream>
void Cursor::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyPressed(SDLK_RETURN))
	{
		if (selectedUnit)
		{
			glm::vec2 unitCurrentPosition = selectedUnit->sprite.getPosition();
			if (unitCurrentPosition == position)
			{
				//Can't move to where you already are, this will bring up unit options once those are implemented
				std::cout << "Unit options here\n";
			}
			//Can't move to an already occupied tile
			else if (!TileManager::tileManager.getTile(position.x, position.y)->occupiedBy)
			{
				TileManager::tileManager.removeUnit(unitCurrentPosition.x, unitCurrentPosition.y);
				selectedUnit->placeUnit(position.x, position.y);
				selectedUnit = nullptr;
				//Will be bringing up unit options here once those are implemented
			}
		}
		else if (focusedUnit)
		{
			selectedUnit = focusedUnit;
			focusedUnit = nullptr;
		}
		else
		{
			//Open menu
			std::cout << "Menu opens here\n";
		}
	}
	//Movement input is all a mess
	if (inputManager.isKeyDown(SDLK_LSHIFT))
	{
		fastCursor = true;
	}
	else if (inputManager.isKeyReleased(SDLK_LSHIFT))
	{
		fastCursor = false;
	}
	int xDirection = 0;
	int yDirection = 0;
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		xDirection = 1;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		xDirection = -1;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		yDirection = -1;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		yDirection = 1;
		firstMove = true;
	}
	if (xDirection != 0 || yDirection != 0)
	{
		Move(xDirection, yDirection);
	}
	if (inputManager.isKeyDown(SDLK_RIGHT))
	{
		xDirection = 1;
	}
	if (inputManager.isKeyDown(SDLK_LEFT))
	{
		xDirection = -1;
	}
	if (inputManager.isKeyDown(SDLK_UP))
	{
		yDirection = -1;
	}
	if (inputManager.isKeyDown(SDLK_DOWN))
	{
		yDirection = 1;
	}
	if (xDirection != 0 || yDirection != 0)
	{
		movementDelay += deltaTime;
		float delayTime = normalDelay;
		if (fastCursor)
		{
			delayTime = fastDelay;
		}
		else if (firstMove)
		{
			delayTime = firstDelay;
		}
		if (movementDelay >= delayTime)
		{
			Move(xDirection, yDirection);
			movementDelay = 0.0f;
			firstMove = false;
		}
	}
	else if (xDirection == 0 && yDirection == 0)
	{
		movementDelay = 0.0f;
		firstMove = true;
	}

	CheckBounds();
}

void Cursor::CheckBounds()
{
	if (position.x < 0)
	{
		position.x = 0;
	}
	else if (position.x + TileManager::TILE_SIZE > TileManager::tileManager.levelWidth)
	{
		position.x = TileManager::tileManager.levelWidth - TileManager::TILE_SIZE;
	}
	if (position.y < 0)
	{
		position.y = 0;
	}
	else if (position.y + TileManager::TILE_SIZE > TileManager::tileManager.levelHeight)
	{
		position.y = TileManager::tileManager.levelHeight - TileManager::TILE_SIZE;
	}
}

void Cursor::Move(int x, int y)
{
	position += glm::ivec2(x, y) * TileManager::TILE_SIZE;
	if (!selectedUnit)
	{
		if (auto unit = TileManager::tileManager.getTile(position.x, position.y)->occupiedBy)
		{
			focusedUnit = unit;
		}
		else
		{
			focusedUnit = nullptr;
		}
	}
}

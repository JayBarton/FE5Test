#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include <SDL.h>
void Cursor::CheckInput(InputManager& inputManager, float deltaTime)
{
	if (inputManager.isKeyDown(SDLK_LSHIFT))
	{
		fastCursor = true;
	}
	else if (inputManager.isKeyReleased(SDLK_LSHIFT))
	{
		fastCursor = false;
	}
	if (inputManager.isKeyPressed(SDLK_RIGHT))
	{
		position.x += TileManager::TILE_SIZE;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_LEFT))
	{
		position.x -= TileManager::TILE_SIZE;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_UP))
	{
		position.y -= TileManager::TILE_SIZE;
		firstMove = true;
	}
	if (inputManager.isKeyPressed(SDLK_DOWN))
	{
		position.y += TileManager::TILE_SIZE;
		firstMove = true;
	}
	int xSpeed = 0;
	int ySpeed = 0;
	if (inputManager.isKeyDown(SDLK_RIGHT))
	{
		xSpeed = 1;
	}
	if (inputManager.isKeyDown(SDLK_LEFT))
	{
		xSpeed = -1;
	}
	if (inputManager.isKeyDown(SDLK_UP))
	{
		ySpeed = -1;
	}
	if (inputManager.isKeyDown(SDLK_DOWN))
	{
		ySpeed = 1;
	}
	if (xSpeed != 0 || ySpeed != 0)
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
			position += glm::ivec2(xSpeed, ySpeed) * TileManager::TILE_SIZE;
			movementDelay = 0.0f;
			firstMove = false;
		}
	}
	else if (xSpeed == 0 && ySpeed == 0)
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

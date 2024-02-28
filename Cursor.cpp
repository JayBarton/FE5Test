#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include "Camera.h"
#include "MenuManager.h"
#include <SDL.h>
#include <iostream>
#include <algorithm>
/*/bool compareMoveCost(const searchCell& a, const searchCell& b) {
	return a.moveCost < b.moveCost;
}*/

//Cursor should have a reference to menumanager
void Cursor::CheckInput(InputManager& inputManager, float deltaTime, Camera& camera)
{
	if (movingUnit)
	{
		if (!selectedUnit->movementComponent.moving)
		{
			movingUnit = false;
			GetUnitOptions();
		}
	}
	else
	{
		if (inputManager.isKeyPressed(SDLK_RETURN))
		{
			if (selectedUnit)
			{
				//If this is an enemy unit, stop drawing its range, and if over another unit focus on it
				if (selectedUnit->team == 1)
				{
					selectedUnit = nullptr;
					foundTiles.clear();
					attackTiles.clear();
					path.clear();
					costTile.clear();
					if (auto tile = TileManager::tileManager.getTile(position.x, position.y))
					{
						focusedUnit = tile->occupiedBy;
					}
				}
				else
				{
					glm::vec2 unitCurrentPosition = selectedUnit->sprite.getPosition();
					//Can't move to where you already are, so just treat as a movement of 0 and open options
					if (unitCurrentPosition == position)
					{
						std::cout << "Unit options here\n";
						foundTiles.clear();
						costTile.clear();
						attackTiles.clear();
						GetUnitOptions();
					}
					//Can't move to an already occupied tile
					else if (!TileManager::tileManager.getTile(position.x, position.y)->occupiedBy)
					{
						//If clicking on a valid position, move the selected unit there
						if (path.find(position) != path.end())
						{
							glm::vec2 pathPoint = position;
							drawnPath.clear();
							drawnPath.push_back(pathPoint);
							//This is just to make sure I can find a path to follow once I begin animating the units
							while (pathPoint != unitCurrentPosition)
							{
								auto previous = path[pathPoint].previousPosition;
								drawnPath.push_back(previous);
								pathPoint = previous;
							}
							foundTiles.clear();
							attackTiles.clear();
							costTile.clear();
							movingUnit = true;

							selectedUnit->movementComponent.startMovement(drawnPath);
						}
					}
				}
			}
			else if (focusedUnit && !focusedUnit->hasMoved)
			{
				previousPosition = position;
				selectedUnit = focusedUnit;
				focusedUnit = nullptr;
				//path[position] = { position, position };
				path = selectedUnit->FindUnitMoveRange();
				foundTiles = selectedUnit->foundTiles;
				attackTiles = selectedUnit->attackTiles;
				costTile = selectedUnit->costTile;
				//FindUnitMoveRange();
			}
			else
			{
				//Open menu
				MenuManager::menuManager.AddMenu(3);
			}
		}
		else if (inputManager.isKeyPressed(SDLK_SPACE))
		{
			if (focusedUnit)
			{
				MenuManager::menuManager.AddUnitStatMenu(focusedUnit);
			}
		}
		//Cancel
		else if (inputManager.isKeyPressed(SDLK_z))
		{
			if (selectedUnit)
			{
				selectedUnit->ClearPathData();
				focusedUnit = selectedUnit;
				selectedUnit = nullptr;
				foundTiles.clear();
				attackTiles.clear();
				path.clear();
				costTile.clear();
				position = previousPosition;
				camera.moving = true;
				camera.SetMove(position);
			}
		}

		//Movement input is all a mess
		MovementInput(inputManager, deltaTime);

		CheckBounds();
	}
}

void Cursor::MovementInput(InputManager& inputManager, float deltaTime)
{
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
			Move(xDirection, yDirection, true);
			movementDelay = 0.0f;
			firstMove = false;
		}
	}
	else if (xDirection == 0 && yDirection == 0)
	{
		movementDelay = 0.0f;
		firstMove = true;
	}
}

void Cursor::GetUnitOptions()
{
	MenuManager::menuManager.AddMenu(0);
}

//Not sure about passing the team here. Not sure how finding healable units should work, 
//since healing and attack range could be different, so I can't really reuse this without copying the code.
std::vector<Unit*> Cursor::inRangeUnits()
{
	std::vector<Unit*> units;
	glm::ivec2 position = glm::ivec2(selectedUnit->sprite.getPosition());

	std::vector<glm::vec2> foundTiles2;
	std::vector<glm::vec2> rangeTiles;
	std::vector<searchCell> checking;
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
	searchCell first = { normalPosition, 0 };
	selectedUnit->addToOpenSet(first, checking, checked, costs);
	while (checking.size() > 0)
	{
		auto current = checking[0];
		selectedUnit->removeFromOpenList(checking);
		int cost = current.moveCost;
		glm::vec2 checkPosition = current.position;

		glm::vec2 up = glm::vec2(checkPosition.x, checkPosition.y - 1);
		glm::vec2 down = glm::vec2(checkPosition.x, checkPosition.y + 1);
		glm::vec2 left = glm::vec2(checkPosition.x - 1, checkPosition.y);
		glm::vec2 right = glm::vec2(checkPosition.x + 1, checkPosition.y);

		CheckAdjacentTiles(up, checked, checking, current, costs, foundTiles2, units);
		CheckAdjacentTiles(down, checked, checking, current, costs, foundTiles2, units);
		CheckAdjacentTiles(right, checked, checking, current, costs, foundTiles2, units);
		CheckAdjacentTiles(left, checked, checking, current, costs, foundTiles2, units);
	}
	return units;
}

void Cursor::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<Unit*>& units)
{
	glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
	if (!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
	{
		int mCost = startCell.moveCost;
		auto thisTile = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y);
		int movementCost = mCost + 1;

		auto distance = costs[checkingTile.x][checkingTile.y];
		if (!checked[checkingTile.x][checkingTile.y])
		{
			//This is a weird thing that is only needed to get the attack range, I hope to remove it at some point.
			//No idea if this is even needed here, this is what happens when you have the exact same code pasted in three locations!!!
			if (movementCost < distance)
			{
				costs[checkingTile.x][checkingTile.y] = movementCost;
			}
			if (movementCost <= selectedUnit->maxRange)
			{
				//If the attack range goes from 1-2, we need to add every unit that is within 2 tiles. However,
				//if the attack range is just 2, such as with bows, we only want to add units that are exactly 2 tiles away
				//This does NOT account for cases in which the range is not a real range, but is 1, 3. So if you have one weapon with a range of 1-1, and
				//Another with a range of 3-3, this breaks down. Not sure how to resolve that at this time.
				if ((selectedUnit->minRange == selectedUnit->maxRange && movementCost == selectedUnit->maxRange) ||
					(selectedUnit->minRange < selectedUnit->maxRange && movementCost <= selectedUnit->maxRange))
				{
					if (Unit * unit = TileManager::tileManager.getUnitOnTeam(tilePosition.x, tilePosition.y, 1))
					{
						units.push_back(unit);
					}
				}
				searchCell newCell{ checkingTile, movementCost };
				selectedUnit->addToOpenSet(newCell, checking, checked, costs);
				foundTiles.push_back(tilePosition);
			}
		}
	}
}
//There is a bug here caused by the fact that the selected unit does not actually move tiles until the move has been confirmed,
//which means the selected unit can actually show up as an in range trade unit if it only moves one space.
std::vector<Unit*> Cursor::tradeRangeUnits()
{
	std::vector<Unit*> units;
	glm::ivec2 position = glm::ivec2(selectedUnit->sprite.getPosition());

	glm::ivec2 up = glm::ivec2(position.x, position.y - 1 * TileManager::TILE_SIZE);
	glm::ivec2 down = glm::ivec2(position.x, position.y + 1 * TileManager::TILE_SIZE);
	glm::ivec2 left = glm::ivec2(position.x - 1 * TileManager::TILE_SIZE, position.y);
	glm::ivec2 right = glm::ivec2(position.x + 1 * TileManager::TILE_SIZE, position.y);
	if (Unit* unit = TileManager::tileManager.getUnitOnTeam(up.x, up.y, 0))
	{
		units.push_back(unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnitOnTeam(down.x, down.y, 0))
	{
		units.push_back(unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnitOnTeam(left.x, left.y, 0))
	{
		units.push_back(unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnitOnTeam(right.x, right.y, 0))
	{
		units.push_back(unit);
	}
	return units;
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

//Not happy with this, but it works for now
void Cursor::Move(int x, int y, bool held)
{
	glm::vec2 moveTo = glm::ivec2(position) + glm::ivec2(x, y) * TileManager::TILE_SIZE;
	bool move = true;
	if (held && !fastCursor)
	{
		if (path.find(position) != path.end() && path.find(moveTo) == path.end())
		{
			move = false;
		}
	}
	if (move)
	{
		position = moveTo;
		if (!selectedUnit)
		{
			if (auto tile = TileManager::tileManager.getTile(position.x, position.y))
			{
				focusedUnit = tile->occupiedBy;
			}
			else
			{
				focusedUnit = nullptr;
			}
		}
	}
}

void Cursor::Wait()
{
	selectedUnit->ClearPathData();
	TileManager::tileManager.removeUnit(previousPosition.x, previousPosition.y);
	selectedUnit->placeUnit(position.x, position.y);
	focusedUnit = selectedUnit;
	selectedUnit = nullptr;
	drawnPath.clear();
	path.clear();
}

void Cursor::UndoMove()
{
	selectedUnit->ClearPathData();
	position = previousPosition;
	selectedUnit->sprite.SetPosition(glm::vec2(position.x, position.y));
	focusedUnit = selectedUnit;
	selectedUnit = nullptr;
	foundTiles.clear();
	attackTiles.clear();
	path.clear();
	costTile.clear();
	drawnPath.clear();
}
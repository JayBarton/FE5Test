#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include "Camera.h"
#include "MenuManager.h"
#include <SDL.h>
#include <iostream>
#include <algorithm>
bool compareMoveCost(const searchCell& a, const searchCell& b) {
	return a.moveCost < b.moveCost;
}
//Cursor should have a reference to menumanager
void Cursor::CheckInput(InputManager& inputManager, float deltaTime, Camera& camera)
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
				if (placingUnit)
				{
					TileManager::tileManager.removeUnit(previousPosition.x, previousPosition.y);
					selectedUnit->placeUnit(position.x, position.y);
					selectedUnit = nullptr;
					drawnPath.clear();
					path.clear();
					placingUnit = false;
				}
				//Can't move to where you already are, so just treat as a movement of 0 and open options
				else if (unitCurrentPosition == position)
				{
					std::cout << "Unit options here\n";
					placingUnit = true;
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

						selectedUnit->sprite.SetPosition(glm::vec2(position.x, position.y));
						placingUnit = true;
						foundTiles.clear();
						attackTiles.clear();
						costTile.clear();
						//Will be bringing up unit options here once those are implemented
						GetUnitOptions();
					}
				}
			}
		}
		else if (focusedUnit)
		{
			previousPosition = position;
			selectedUnit = focusedUnit;
			focusedUnit = nullptr;
			path[position] = { position, position };
			FindUnitMoveRange();
		}
		else
		{
			//Open menu
			std::cout << "Main Menu opens here\n";
		}
	}
	//Cancel
	else if (inputManager.isKeyPressed(SDLK_z))
	{
		if (placingUnit)
		{
			UndoMove();
		}
		else if (selectedUnit)
		{
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
	if (!placingUnit)
	{
		//Movement input is all a mess
		MovementInput(inputManager, deltaTime);
	}
	CheckBounds();
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

void Cursor::FindUnitMoveRange()
{
	std::vector<searchCell> checking;
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
	searchCell first = { normalPosition, 0 };
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
	//if unit attack range is > 1
	//Attack range not implemented yet, hardsetting here to test
	if (selectedUnit->maxRange > 0)
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
			for (int c = 1; c < selectedUnit->maxRange; c++)
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
}

void Cursor::CheckExtraRange(glm::ivec2& checkingTile, std::vector<std::vector<bool>>& checked)
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

void Cursor::CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs)
{
	glm::ivec2 tilePosition = glm::ivec2(checkingTile) * TileManager::TILE_SIZE;
	if (!TileManager::tileManager.outOfBounds(tilePosition.x, tilePosition.y))
	{
		int mCost = startCell.moveCost;
		auto thisTile = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y);
		int movementCost = mCost + thisTile->properties.movementCost;

		auto distance = costs[checkingTile.x][checkingTile.y];
		if (!checked[checkingTile.x][checkingTile.y])
		{
			auto otherUnit = TileManager::tileManager.getTile(tilePosition.x, tilePosition.y)->occupiedBy;
			//This is horrid
			if (otherUnit && otherUnit != selectedUnit && otherUnit->team != selectedUnit->team)
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
			if (movementCost <= selectedUnit->move)
			{
				searchCell newCell{ checkingTile, movementCost };
				addToOpenSet(newCell, checking, checked, costs);
				foundTiles.push_back(tilePosition);
				costTile.push_back(movementCost);
				path[tilePosition] = { tilePosition, glm::ivec2(startCell.position) * TileManager::TILE_SIZE };
			}
			else
			{
				if (selectedUnit->maxRange > 0)
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



void Cursor::addToOpenSet(searchCell newCell, std::vector<searchCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs)
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
			searchCell tempNode = checking[position];
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

void Cursor::removeFromOpenList(std::vector<searchCell>& checking)
{
	int position = 1;
	int position2;
	searchCell temp;
	std::vector<searchCell>::iterator it = checking.end() - 1;
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

	int maxRange = selectedUnit->maxRange;
	int minRange = selectedUnit->minRange;
	for (int i = minRange; i < maxRange + 1; i++)
	{
		glm::ivec2 up = glm::ivec2(position.x, position.y - i * TileManager::TILE_SIZE);
		glm::ivec2 down = glm::ivec2(position.x, position.y + i * TileManager::TILE_SIZE);
		glm::ivec2 left = glm::ivec2(position.x - i * TileManager::TILE_SIZE, position.y);
		glm::ivec2 right = glm::ivec2(position.x + i * TileManager::TILE_SIZE, position.y);
		if (Unit* unit = TileManager::tileManager.getUnitOnTeam(up.x, up.y, 1))
		{
			units.push_back(unit);
		}
		for (int c = minRange; c < maxRange + 1 - i; c++)
		{
			glm::ivec2 upLeft = glm::ivec2(up.x - c * TileManager::TILE_SIZE, up.y);
			glm::ivec2 upRight = glm::ivec2(up.x + c * TileManager::TILE_SIZE, up.y);
			if (Unit* unit = TileManager::tileManager.getUnitOnTeam(upLeft.x, upLeft.y, 1))
			{
				units.push_back(unit);
			}
			if (Unit* unit = TileManager::tileManager.getUnitOnTeam(upRight.x, upRight.y, 1))
			{
				units.push_back(unit);
			}
		}
		if (Unit* unit = TileManager::tileManager.getUnitOnTeam(down.x, down.y, 1))
		{
			units.push_back(unit);
		}
		for (int c = minRange; c < maxRange + 1 - i; c++)
		{
			glm::ivec2 downLeft = glm::ivec2(down.x - c * TileManager::TILE_SIZE, down.y);
			glm::ivec2 downRight = glm::ivec2(down.x + c * TileManager::TILE_SIZE, down.y);
			if (Unit* unit = TileManager::tileManager.getUnitOnTeam(downLeft.x, downLeft.y, 1))
			{
				units.push_back(unit);
			}
			if (Unit* unit = TileManager::tileManager.getUnitOnTeam(downRight.x, downRight.y, 1))
			{
				units.push_back(unit);
			}
		}
		if (Unit* unit = TileManager::tileManager.getUnitOnTeam(left.x, left.y, 1))
		{
			units.push_back(unit);
		}
		if (Unit* unit = TileManager::tileManager.getUnitOnTeam(right.x, right.y, 1))
		{
			units.push_back(unit);
		}
	}

	return units;
}

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
	TileManager::tileManager.removeUnit(previousPosition.x, previousPosition.y);
	selectedUnit->placeUnit(position.x, position.y);
	selectedUnit = nullptr;
	drawnPath.clear();
	path.clear();
	placingUnit = false;
}

void Cursor::UndoMove()
{
	placingUnit = false;
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
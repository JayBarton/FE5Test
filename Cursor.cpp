#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include "Camera.h"
#include "MenuManager.h"
#include <SDL.h>
#include <iostream>
#include <algorithm>

//Cursor should have a reference to menumanager
void Cursor::CheckInput(InputManager& inputManager, float deltaTime, Camera& camera)
{
	if (movingUnit)
	{
		selectedUnit->UpdateMovement(deltaTime, inputManager);
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
					ClearTiles();
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
						if (selectedUnit->isMounted())
						{
							selectedUnit->mount->remainingMoves = selectedUnit->getMove();
						}
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
							auto unitPosition = selectedUnit->sprite.getPosition();
							TileManager::tileManager.removeUnit(unitPosition.x, unitPosition.y);
							selectedUnit->startMovement(drawnPath, path[position].moveCost, remainingMove);
						}
					}
				}
			}
			else if (focusedUnit && !focusedUnit->hasMoved)
			{
				previousPosition = position;
				selectedUnit = focusedUnit;
				selectedUnit->SetFocus();
				focusedUnit = nullptr;
				path = selectedUnit->FindUnitMoveRange();
				foundTiles = selectedUnit->foundTiles;
				attackTiles = selectedUnit->attackTiles;
				costTile = selectedUnit->costTile;
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
				if (!remainingMove)
				{
					Undo();
				}
				position = previousPosition;
				camera.SetMove(position);
			}
		}

		//Movement input is all a mess
		MovementInput(inputManager, deltaTime);
	}
}

void Cursor::ClearTiles()
{
	ClearSelected();
	foundTiles.clear();
	attackTiles.clear();
	path.clear();
	costTile.clear();
}

void Cursor::ClearSelected()
{
	selectedUnit->sprite.moveAnimate = false;
	selectedUnit = nullptr;
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
	if (remainingMove)
	{
		MenuManager::menuManager.AddMenu(4);
	}
	else
	{
		MenuManager::menuManager.AddMenu(0);
	}
}

void Cursor::GetAdjacentUnits(std::vector<Unit*>& tradeUnits, std::vector<Unit*>& talkUnits)
{
	tradeUnits.clear();
	talkUnits.clear();
	glm::ivec2 position = glm::ivec2(selectedUnit->sprite.getPosition());

	glm::ivec2 up = glm::ivec2(position.x, position.y - 1 * TileManager::TILE_SIZE);
	glm::ivec2 down = glm::ivec2(position.x, position.y + 1 * TileManager::TILE_SIZE);
	glm::ivec2 left = glm::ivec2(position.x - 1 * TileManager::TILE_SIZE, position.y);
	glm::ivec2 right = glm::ivec2(position.x + 1 * TileManager::TILE_SIZE, position.y);
	if (selectedUnit->carriedUnit)
	{
		tradeUnits.push_back(selectedUnit->carriedUnit);
	}
	if (Unit* unit = TileManager::tileManager.getUnit(up.x, up.y))
	{
		PushTradeUnit(tradeUnits, unit);
		//Since you can possibly talk to a unit on any team, need this to be separate
		PushTalkUnit(talkUnits, unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnit(down.x, down.y))
	{
		PushTradeUnit(tradeUnits, unit);
		PushTalkUnit(talkUnits, unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnit(left.x, left.y))
	{
		PushTradeUnit(tradeUnits, unit);
		PushTalkUnit(talkUnits, unit);
	}
	if (Unit* unit = TileManager::tileManager.getUnit(right.x, right.y))
	{
		PushTradeUnit(tradeUnits, unit);
		PushTalkUnit(talkUnits, unit);
	}
}

std::vector<glm::ivec2> Cursor::getDropPositions()
{
	std::vector < glm::ivec2 > dropPositions;
	glm::ivec2 position = glm::ivec2(selectedUnit->sprite.getPosition());

	glm::ivec2 up = glm::ivec2(position.x, position.y - 1 * TileManager::TILE_SIZE);
	glm::ivec2 down = glm::ivec2(position.x, position.y + 1 * TileManager::TILE_SIZE);
	glm::ivec2 left = glm::ivec2(position.x - 1 * TileManager::TILE_SIZE, position.y);
	glm::ivec2 right = glm::ivec2(position.x + 1 * TileManager::TILE_SIZE, position.y);
	FindDropPosition(up, dropPositions);
	FindDropPosition(right, dropPositions);
	FindDropPosition(down, dropPositions);
	FindDropPosition(left, dropPositions);

	return dropPositions;
}

void Cursor::FindDropPosition(glm::ivec2& position, std::vector<glm::ivec2>& dropPositions)
{
	if (!TileManager::tileManager.outOfBounds(position.x, position.y) && 
		!TileManager::tileManager.getUnit(position.x, position.y) && 
		TileManager::tileManager.getTile(position.x, position.y)->properties.movementCost < 20)
	{
		dropPositions.push_back(position);
	}
}

void Cursor::PushTradeUnit(std::vector<Unit*>& units, Unit*& unit)
{
	if (unit->team == 0)
	{
		units.push_back(unit);
		if (unit->carriedUnit)
		{
			units.push_back(unit->carriedUnit);
		}
	}
}
void Cursor::PushTalkUnit(std::vector<Unit*>& units, Unit*& unit)
{
	for (int i = 0; i < selectedUnit->talkData.size(); i++)
	{
		if (unit->sceneID == selectedUnit->talkData[i].talkTarget)
		{
			units.push_back(unit);
		}
	}
}
void Cursor::CheckBounds()
{
	if (position.x < TileManager::TILE_SIZE)
	{
		position.x = TileManager::TILE_SIZE;
	}
	else if (position.x + TileManager::TILE_SIZE > TileManager::tileManager.levelWidth - TileManager::TILE_SIZE)
	{
		position.x = TileManager::tileManager.levelWidth - TileManager::TILE_SIZE * 2;
	}
	if (position.y < TileManager::TILE_SIZE)
	{
		position.y = TileManager::TILE_SIZE;
	}
	else if (position.y + TileManager::TILE_SIZE > TileManager::tileManager.levelHeight - TileManager::TILE_SIZE)
	{
		position.y = TileManager::tileManager.levelHeight - TileManager::TILE_SIZE * 2;
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
		CheckBounds();

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
//Being called erroneously in a few places
void Cursor::Wait()
{
	MoveUnitToTile();
	FinishMove();
}

void Cursor::FinishMove()
{
	path.clear();
	selectedUnit->hasMoved = true;
	focusedUnit = selectedUnit;
	ClearSelected();
	remainingMove = false;
}

//This sucks
void Cursor::MoveUnitToTile()
{
	selectedUnit->ClearPathData();
	TileManager::tileManager.removeUnit(previousPosition.x, previousPosition.y);
	selectedUnit->placeUnit(position.x, position.y);
	drawnPath.clear();
}

void Cursor::GetRemainingMove()
{
	remainingMove = true;
	MoveUnitToTile();
	drawnPath.clear();
	path.clear();

	previousPosition = position;
	path = selectedUnit->FindRemainingMoveRange();
	foundTiles = selectedUnit->foundTiles;
	attackTiles = selectedUnit->attackTiles;
	costTile = selectedUnit->costTile;
}

void Cursor::Undo()
{
	selectedUnit->ClearPathData();
	focusedUnit = selectedUnit;
	ClearTiles();
}

void Cursor::UndoMove()
{
	position = previousPosition;
	selectedUnit->placeUnit(position.x, position.y);
	selectedUnit->sprite.SetPosition(glm::vec2(position.x, position.y));
	drawnPath.clear();
	Undo();
}

void Cursor::UndoRemainingMove()
{
	selectedUnit->ClearPathData();
	position = previousPosition;
	selectedUnit->sprite.SetPosition(glm::vec2(position.x, position.y));

	GetRemainingMove();
}

#include "Cursor.h"
#include "TileManager.h"
#include "InputManager.h"
#include "Camera.h"
#include "MenuManager.h"
#include <SDL.h>
#include <iostream>
#include <algorithm>
#include "RangeBatch.h"
#include "Settings.h"
#include "PlayerManager.h"

void Cursor::CheckInput(InputManager& inputManager, float deltaTime, Camera& camera)
{
	//This check insures the cursor moves properly to it's target location
	if (moving)
	{
		if (!fastCursor)
		{
			if (inputManager.isKeyPressed(SDLK_RIGHT))
			{
				futureDirection.x = 1;
			}
			if (inputManager.isKeyPressed(SDLK_LEFT))
			{
				futureDirection.x = -1;
			}
			if (inputManager.isKeyPressed(SDLK_UP))
			{
				futureDirection.y = -1;
			}
			if (inputManager.isKeyPressed(SDLK_DOWN))
			{
				futureDirection.y = 1;
			}
		}
		position += cursorSpeed * moveDirection;
		glm::vec2 diff = ((position - movePosition) * moveDirection);
		auto distance = glm::distance(position, movePosition);
		if (distance == 0)
		{
			position = movePosition;
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
			moving = false;
		}
	}
	else if (movingUnit)
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
						if (selectedUnit->isMounted() && !remainingMove)
						{
							selectedUnit->mount->remainingMoves = selectedUnit->getMove();
						}
						ClearRange();
						GetUnitOptions();
					}
					//Can't move to an already occupied tile
					else if (!TileManager::tileManager.getTile(position.x, position.y)->occupiedBy)
					{
						//If clicking on a valid position, move the selected unit there
						if (path.find(position) != path.end())
						{
							glm::vec2 pathPoint = position;
							std::vector<glm::ivec2> drawnPath;
							drawnPath.push_back(pathPoint);
							while (pathPoint != unitCurrentPosition)
							{
								auto previous = path[pathPoint].previousPosition;
								drawnPath.push_back(previous);
								pathPoint = previous;
							}
							ClearRange();
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
				ResourceManager::PlaySound("select1");
				previousPosition = position;
				selectedUnit = focusedUnit;
				selectedUnit->SetFocus();
				focusedUnit = nullptr;
				path = selectedUnit->FindUnitMoveRange();
				foundTiles = selectedUnit->foundTiles;
				attackTiles = selectedUnit->attackTiles;
				drawRanges = true;
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
				ResourceManager::PlaySound("cancel", -1, true);
			}
		}
		else if (!selectedUnit)
		{
			if (inputManager.isKeyPressed(SDLK_ESCAPE))
			{
				MenuManager::menuManager.AddMenu(3);
			}
			else if (playerManager->unmovedUnits > 0)
			{
				if (inputManager.isKeyPressed(SDLK_a))
				{
					playerIndex--;
					if (playerIndex < 0)
					{
						playerIndex = playerManager->units.size() - 1;
					}
					while (playerManager->units[playerIndex]->hasMoved)
					{
						playerIndex--;
						if (playerIndex < 0)
						{
							playerIndex = playerManager->units.size() - 1;
						}
					}
					SetFocus(playerManager->units[playerIndex]);
					camera.SetMove(position);
				}
				else if (inputManager.isKeyPressed(SDLK_s))
				{
					playerIndex++;
					if (playerIndex >= playerManager->units.size())
					{
						playerIndex = 0;
					}
					while (playerManager->units[playerIndex]->hasMoved)
					{
						playerIndex++;
						if (playerIndex >= playerManager->units.size())
						{
							playerIndex = 0;
						}
					}
					SetFocus(playerManager->units[playerIndex]);
					camera.SetMove(position);
				}
			}
		}

		//Movement input is all a mess
		MovementInput(inputManager, deltaTime);

		if (Settings::settings.unitWindow)
		{
			if (!settled)
			{
				settleTimer += deltaTime;
				if (settleTimer >= firstDelay)
				{
					settleTimer = 0.0f;
					settled = true;
				}
			}
		}
	}
	frameTimer += deltaTime;
	if (frameTimer >= 0.25f)
	{
		frameTimer = 0.0f;
		currentFrame++;
		if (currentFrame > 1)
		{
			currentFrame = 0;
		}
	}
	if (drawRanges)
	{
		rangeDelay += deltaTime;
		if (rangeDelay >= 0.05f)
		{
			rangeDelay = 0.0f;
			offset += 1.0f / 16.0f;
			offset = fmod(offset, sqrt(2.0f));
		}
	}
}

void Cursor::ClearTiles()
{
	ClearSelected();
	ClearRange();
	path.clear();
}

void Cursor::ClearSelected()
{
	if (selectedUnit)
	{
		selectedUnit->sprite.moveAnimate = false;
		selectedUnit = nullptr;
	}
}

void Cursor::MovementInput(InputManager& inputManager, float deltaTime)
{
	cursorSpeed = 4;
	if (inputManager.isKeyDown(SDLK_LSHIFT))
	{
		fastCursor = true;
		cursorSpeed = 8;
		settled = false;
		settleTimer = 0.0f;
	}
	else if (inputManager.isKeyUp(SDLK_LSHIFT))
	{
		fastCursor = false;
	}
	int xDirection = 0;
	int yDirection = 0;
	if (!fastCursor)
	{
		if (futureDirection.x != 0 || futureDirection.y != 0)
		{
			Move(futureDirection.x, futureDirection.y);
			firstMove = true;
			futureDirection = glm::vec2(0);
		}
		else
		{
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
		}
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
		float delayTime = 0;
		if (fastCursor)
		{
			delayTime = 0;
		}
		else if (firstMove)
		{
			delayTime = firstDelay;
		}
		if (movementDelay >= delayTime)
		{
			Move(xDirection, yDirection, true);
			firstMove = false;
		}
	}
	else if (xDirection == 0 && yDirection == 0)
	{
		movementDelay = 0.0f;
		firstMove = true;
	}
}

//Not happy with this, but it works for now
void Cursor::Move(int x, int y, bool held)
{
	movementDelay = 0.0f;
	glm::vec2 moveTo = glm::ivec2(position) + glm::ivec2(x, y) * TileManager::TILE_SIZE;
	bool move = true;
	//This is to check if I want to stop movement for a moment when the cursor hits the edge of a unit's movement range
	if (held && !fastCursor)
	{
		if (path.find(position) != path.end() && path.find(moveTo) == path.end())
		{
			move = false;
		}
	}
	if (move)
	{
		if(CheckBounds(moveTo))
		{
			moveDirection = glm::vec2(x, y);
			movePosition = moveTo;
			//Want to move on this first frame
			position += cursorSpeed * moveDirection;

			ResourceManager::PlaySound("cursorMove", 1);
			moving = true;
			focusedUnit = nullptr;
			settled = false;
			settleTimer = 0.0f;
		}
	}
}

bool Cursor::CheckBounds(glm::vec2 pos)
{
	return pos.x >= TileManager::TILE_SIZE &&
		pos.x + TileManager::TILE_SIZE <= TileManager::tileManager.levelWidth - TileManager::TILE_SIZE &&
		pos.y >= TileManager::TILE_SIZE &&
		pos.y + TileManager::TILE_SIZE <= TileManager::tileManager.levelHeight - TileManager::TILE_SIZE;
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
	playerManager->unmovedUnits--;
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
}

void Cursor::GetRemainingMove()
{
	ResourceManager::PlaySound("select1");

	remainingMove = true;
	MoveUnitToTile();
	path.clear();

	previousPosition = position;
	path = selectedUnit->FindRemainingMoveRange();
	foundTiles = selectedUnit->foundTiles;
	attackTiles = selectedUnit->attackTiles;
	selectedUnit->SetFocus();
	drawRanges = true;
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
	Undo();
}

void Cursor::ClearRange()
{
	foundTiles.clear();
	attackTiles.clear();
	drawRanges = false;
	offset = 0.0f;
}

void Cursor::UndoRemainingMove()
{
	selectedUnit->ClearPathData();
	position = previousPosition;
	selectedUnit->sprite.SetPosition(glm::vec2(position.x, position.y));
	selectedUnit->SetFocus();
	GetRemainingMove();
}

void Cursor::SetFocus(Unit* unit)
{
	Unit* toFocus = unit;
	if (toFocus->carryingUnit)
	{
		toFocus = toFocus->carryingUnit;
	}
	position = toFocus->sprite.getPosition();
	focusedUnit = toFocus;
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
	if (Unit* unit = TileManager::tileManager.getUnit(right.x, right.y))
	{
		PushTradeUnit(tradeUnits, unit);
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
}

std::vector<glm::ivec2> Cursor::getDropPositions(Unit* heldUnit)
{
	std::vector < glm::ivec2 > dropPositions;
	glm::ivec2 position = glm::ivec2(selectedUnit->sprite.getPosition());

	glm::ivec2 up = glm::ivec2(position.x, position.y - 1 * TileManager::TILE_SIZE);
	glm::ivec2 down = glm::ivec2(position.x, position.y + 1 * TileManager::TILE_SIZE);
	glm::ivec2 left = glm::ivec2(position.x - 1 * TileManager::TILE_SIZE, position.y);
	glm::ivec2 right = glm::ivec2(position.x + 1 * TileManager::TILE_SIZE, position.y);
	FindDropPosition(up, dropPositions, heldUnit);
	FindDropPosition(right, dropPositions, heldUnit);
	FindDropPosition(down, dropPositions, heldUnit);
	FindDropPosition(left, dropPositions, heldUnit);

	return dropPositions;
}

void Cursor::FindDropPosition(glm::ivec2& position, std::vector<glm::ivec2>& dropPositions, Unit* heldUnit)
{
	if (!TileManager::tileManager.outOfBounds(position.x, position.y) &&
		!TileManager::tileManager.getUnit(position.x, position.y) &&
		TileManager::tileManager.getTile(position.x, position.y)->properties.movementCost[heldUnit->getMovementType()] < 20)
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

void Cursor::DrawUnitRanges(int shapeVAO, Camera& camera)
{
	if (drawRanges)
	{
		RangeBatch rBatch;
		ResourceManager::GetShader("range").Use().SetMatrix4("projection", camera.getCameraMatrix());
		ResourceManager::GetShader("range").SetFloat("alpha", 0.35f);
		ResourceManager::GetShader("range").SetVector2f("offset", glm::vec2(offset, -offset));
		rBatch.init();
		rBatch.begin();

		for (int i = 0; i < foundTiles.size(); i++)
		{
			rBatch.addToBatch(foundTiles[i], glm::vec3(0.1f, 0.5f, 1.0f), glm::vec3(0.71f, 0.94f, 1.0f));
		}

		for (int i = 0; i < attackTiles.size(); i++)
		{
			rBatch.addToBatch(attackTiles[i], glm::vec3(1.0f, 0.42f, 0.0f), glm::vec3(1.0f, 0.65f, 0.45f));
		}

		rBatch.end();
		rBatch.renderBatch();
	}
}

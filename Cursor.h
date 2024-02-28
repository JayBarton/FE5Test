#pragma once
#include "TileManager.h"
#include <unordered_map>

struct Cursor
{
	glm::vec2 position;
	glm::vec2 dimensions = glm::vec2(TileManager::TILE_SIZE, TileManager::TILE_SIZE);
	std::vector<glm::vec4> uvs;

	//Both of these are used to draw the movement and attack range of a unit
	std::vector<glm::ivec2> foundTiles;
	std::vector<glm::ivec2> attackTiles;
	//Temporary, just using to visualize tile costs
	std::vector<int> costTile;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathPoint, vec2Hash> path;
	//Temporary, just using to visualize the path taken
	std::vector<glm::ivec2> drawnPath;

	glm::vec2 previousPosition;

	float movementDelay = 0.0f;
	float normalDelay = 0.05f;
	float fastDelay = 0.025f;
	float firstDelay = 0.2f;

	bool firstMove = true; //Not sure how to handle this. I want a slightly longer delay the first time the player moves, unless they are moving fast
	bool fastCursor = false;
	bool movingUnit = false;

	class Unit* focusedUnit = nullptr;
	class Unit* selectedUnit = nullptr;
	
	void CheckInput(class InputManager& inputManager, float deltaTime, class Camera& camera);
	void MovementInput(InputManager& inputManager, float deltaTime);
	void CheckBounds();
	void Move(int x, int y, bool held = false);

	void Wait();
	void UndoMove();

	void GetUnitOptions();

	std::vector<Unit*> inRangeUnits();
	std::vector<Unit*> tradeRangeUnits();
};
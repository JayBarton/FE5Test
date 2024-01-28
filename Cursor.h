#pragma once
#include "TileManager.h"
#include <unordered_map>
struct searchCell
{
	glm::vec2 position;
	int moveCost;
};
struct pathPoint
{
	glm::ivec2 position;
	glm::ivec2 previousPosition;
};
struct vec2Hash
{
	size_t operator()(const glm::vec2& vec) const
	{
		return ((std::hash<float>()(vec.x) ^ (std::hash<float>()(vec.y) << 1)) >> 1);
	}
};

struct Cursor
{
	glm::vec2 position;
	glm::vec2 dimensions = glm::vec2(TileManager::TILE_SIZE, TileManager::TILE_SIZE);
	std::vector<glm::vec4> uvs;

	std::vector<glm::ivec2> foundTiles;
	std::vector<glm::ivec2> attackTiles;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathPoint, vec2Hash> path;
	std::vector<glm::ivec2> drawnPath;

	std::vector<std::vector<int>> costs;

	glm::vec2 previousPosition;

	float movementDelay = 0.0f;
	float normalDelay = 0.05f;
	float fastDelay = 0.025f;
	float firstDelay = 0.2f;
	bool firstMove = true; //Not sure how to handle this. I want a slightly longer delay the first time the player moves, unless they are moving fast
	bool fastCursor = false;
	//Hopefully temporary, I expect managing different states will get more complicated over time
	bool placingUnit = false;

	class Unit* focusedUnit = nullptr;
	class Unit* selectedUnit = nullptr;

	void CheckInput(class InputManager& inputManager, float deltaTime, class Camera& camera);
	void MovementInput(InputManager& inputManager, float deltaTime);
	void FindUnitMoveRange();
	void CheckExtraRange(glm::ivec2& checkingTile, std::vector<std::vector<bool>>& checked);
	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell);
	void CheckBounds();
	void Move(int x, int y, bool held = false);
};
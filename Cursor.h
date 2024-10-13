#pragma once
#include "TileManager.h"
#include <unordered_map>

struct Cursor
{
	glm::vec2 position;
	glm::vec2 dimensions = glm::vec2(21, 21);
	std::vector<glm::vec4> uvs;

	//Both of these are used to draw the movement and attack range of a unit
	std::vector<glm::ivec2> foundTiles;
	std::vector<glm::ivec2> attackTiles;
	//Temporary, just using to visualize tile costs
	//std::vector<int> costTile;

	//This can probably be a map of vec2s rather than this pathPoint thing
	std::unordered_map<glm::vec2, pathCell, vec2Hash> path;

	class PlayerManager* playerManager = nullptr;

	glm::vec2 previousPosition;
	glm::vec2 movePosition;
	glm::vec2 moveDirection;
	glm::vec2 futureDirection;

	float settleTimer = 0.0f;
	float moveDelay = 0.0666f;
	float delayTime;

	int currentFrame = 0;

	int playerIndex = 0;

	float frameTimer = 0.0f;

	float movementDelay = 0.0f;
	float normalDelay = 0.05f;
	float fastDelay = 0.025f;
	float firstDelay = 0.2f;

	float cursorSpeed;

	float rangeDelay = 0.0f;

	float offset = 0.0f;

	bool firstMove = true; //Not sure how to handle this. I want a slightly longer delay the first time the player moves, unless they are moving fast
	bool fastCursor = false;
	bool movingUnit = false;
	//For Canto users
	bool remainingMove = false; 

	bool moving = false;
	bool settled = false;

	bool drawRanges = false;

	class Unit* focusedUnit = nullptr;
	class Unit* selectedUnit = nullptr;
	
	void CheckInput(class InputManager& inputManager, float deltaTime, class Camera& camera);
	void MovementInput(InputManager& inputManager, float deltaTime);
	bool CheckBounds(glm::vec2 pos);
	void Move(int x, int y, bool held = false);

	void ClearTiles();
	void ClearSelected();
	void ClearRange();

	void Wait();
	void FinishMove();
	void MoveUnitToTile();
	void GetRemainingMove();
	void Undo();
	void UndoMove();
	void UndoRemainingMove();

	void SetFocus(Unit* unit);

	void GetUnitOptions();

	void GetAdjacentUnits(std::vector<Unit*>& tradeUnits, std::vector<Unit*>& talkUnits);
	std::vector<glm::ivec2> getDropPositions(Unit* heldUnit);
	void FindDropPosition(glm::ivec2& up, std::vector<glm::ivec2>& dropPositions, Unit* heldUnit);
	void PushTradeUnit(std::vector<Unit*>& units, Unit*& unit);
	void PushTalkUnit(std::vector<Unit*>& units, Unit*& unit);

	void DrawUnitRanges(int shapeVAO, Camera& camera);
};
#pragma once
#include <vector>
#include <glm.hpp>
#include "Unit.h"
//Want turnsubject, move to it's own file later
#include "MenuManager.h"

class SpriteRenderer;

struct AttackPosition
{
	glm::vec2 position;
	int distance;
};

struct Target
{
	int ID;
	int priority = 0;
	int range = 0;
	Item* weaponToUse = nullptr;
	std::vector<AttackPosition> attackPositions;
};

enum ActionState
{
	GET_TARGET,
	ATTACK,
	END_ATTACK,
	CANTO
};

struct EnemyManager
{
	std::vector<Unit*> enemies;
	int currentEnemy = 0;

	bool canCounter = true;
	bool enemyMoving = false;
	Unit* otherUnit = nullptr;

	TurnSubject subject;

	ActionState state = GET_TARGET;

	std::vector<glm::vec4> UVs;

	void GetPriority(Unit* enemy);
	void NoMove(Unit* enemy, glm::vec2& position);
	void SetUp(std::ifstream& map, std::mt19937* gen, std::uniform_int_distribution<int>* distribution);
	void Draw(SpriteRenderer* renderer);
	void Update(BattleManager& battleManager);
	void CantoMove();
	void FinishMove();
	void UpdateEnemies(float deltaTime);
	void EndTurn();
	void Clear();

	std::vector<AttackPosition> ValidAttackPosition(Unit* toAttack, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path, int minRange, int maxRange);


	//horrible code duplication
	void addToOpenSet(pathCell newCell, std::vector<pathCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<pathCell>& checking);

	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<pathCell>& checking, pathCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles, const std::unordered_map<glm::vec2, pathCell, vec2Hash>& path);

};
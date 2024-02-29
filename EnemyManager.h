#pragma once
#include <vector>
#include <glm.hpp>
#include "Unit.h"

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
};



struct EnemyManager
{
	std::vector<Unit*> enemies;
	int currentEnemy = 0;

	bool canCounter = true;
	Unit* otherUnit = nullptr;

	void GetPriority();
	void Draw(SpriteRenderer* renderer);
	void Update(float deltaTime);

	std::vector<AttackPosition> ValidAttackPosition(Unit* toAttack, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path, int minRange, int maxRange);


	//horrible code duplication
	void addToOpenSet(searchCell newCell, std::vector<searchCell>& checking, std::vector<std::vector<bool>>& checked, std::vector<std::vector<int>>& costs);
	void removeFromOpenList(std::vector<searchCell>& checking);

	void CheckAdjacentTiles(glm::vec2& checkingTile, std::vector<std::vector<bool>>& checked, std::vector<searchCell>& checking, searchCell startCell, std::vector<std::vector<int>>& costs, std::vector<glm::vec2>& foundTiles, std::vector<AttackPosition>& rangeTiles, const std::unordered_map<glm::vec2, pathPoint, vec2Hash>& path);

};
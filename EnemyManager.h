#pragma once
#include <vector>
#include <glm.hpp>
#include "Unit.h"

class SpriteRenderer;

struct Target
{
	int ID;
	int priority;
	int range;
};

struct EnemyManager
{
	std::vector<Unit*> enemies;
	int currentEnemy = 0;

	void GetPriority();
	void Draw(SpriteRenderer* renderer);
};
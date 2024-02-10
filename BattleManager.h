#pragma once
#include <random>
#include "Unit.h"
class TextRenderer;
struct BattleManager
{
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack);

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void Draw(TextRenderer* text);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;

	BattleStats attackerStats;
	BattleStats defenderStats;

	int attackerDamage = 0;
	int defenderDamage = 0;

	float actionDelay = 1.0f;
	float actionTimer = 0.0f;

	bool battleActive = false;
	bool canDefenderAttack = true;
	bool attackerTurn = true;
	bool defenderTurn = false;
	bool checkDouble = true;
};
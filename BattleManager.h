#pragma once
#include <random>
#include "Unit.h"
class TextRenderer;

struct Attack
{
	bool firstAttacker = true;
	bool continuedAttack = false;
	bool vantageAttack = false;
	bool wrathAttack = false;
};

struct BattleManager
{
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack);

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	void PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, int firstDamage, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void Draw(TextRenderer* text);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;

	BattleStats attackerStats;
	BattleStats defenderStats;

	float actionDelay = 1.0f;
	float actionTimer = 0.0f;

	bool battleActive = false;
	bool canDefenderAttack = true;
	bool attackerTurn = true;
	bool defenderTurn = false;
	bool checkDouble = true;

	bool accostFired = false;

	std::vector<Attack> battleQueue;
	std::vector<Attack> accostQueue;
};
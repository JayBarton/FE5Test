#pragma once
#include <random>
#include "Unit.h"

class TextRenderer;

struct BattleObserver
{
	virtual ~BattleObserver() {}
	virtual void onNotify(Unit* attacker, Unit* defender) = 0;
};

struct BattleSubject
{
	std::vector<BattleObserver*> observers;

	void addObserver(BattleObserver* observer)
	{
		observers.push_back(observer);
	}
	void removeObserver(BattleObserver* observer)
	{
		auto it = std::find(observers.begin(), observers.end(), observer);
		if (it != observers.end())
		{
			delete* it;
			*it = observers.back();
			observers.pop_back();
		}
	}
	void notify(Unit* attacker, Unit* defender)
	{
		for (int i = 0; i < observers.size(); i++)
		{
			observers[i]->onNotify(attacker, defender);
		}
	}
};

struct Attack
{
	bool firstAttacker = true;
	bool continuedAttack = false;
	bool vantageAttack = false;
	bool wrathAttack = false;
};
class Camera;
struct BattleManager
{
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack, Camera& camera, bool aiAttack = false);

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution);

	void PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, int firstDamage, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void EndAttack();
	
	void EndBattle(class Cursor* cursor, class EnemyManager* enemyManager, Camera& camera);

	void Draw(TextRenderer* text, Camera& camera, class SpriteRenderer* Renderer, class Cursor* cursor);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;
	Unit* deadUnit = nullptr;

	BattleStats attackerStats;
	BattleStats defenderStats;

	glm::vec2 cameraStart;

	float actionDelay = 1.0f;
	float actionTimer = 0.0f;

	bool battleActive = false;
	bool canDefenderAttack = true;
	bool attackerTurn = true;
	bool defenderTurn = false;
	bool checkDouble = true;
	bool accostFired = false;
	bool unitDied = false;
	bool aiDelay = false;

	float delayTimer = 0.0f;
	float delay = 0.75f;

	std::vector<Attack> battleQueue;
	std::vector<Attack> accostQueue;

	BattleSubject subject;
};
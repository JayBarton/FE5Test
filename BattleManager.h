#pragma once
#include <random>
#include "Unit.h"
#include "TextAdvancer.h"

class TextRenderer;

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
	void SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack, Camera& camera, bool aiAttack = false, bool capturing = false);

	void Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, class InfoDisplays& displays);

	void PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, BattleStats& theseStats, Attack& attack, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen);

	void EndAttack();
	
	void EndBattle(class Cursor* cursor, class EnemyManager* enemyManager, Camera& camera);

	void DropHeldUnit();

	void Draw(TextRenderer* text, Camera& camera, class SpriteRenderer* Renderer, class Cursor* cursor);

	Unit* attacker = nullptr;
	Unit* defender = nullptr;
	Unit* deadUnit = nullptr;
	Unit* unitToDrop = nullptr;

	BattleStats attackerStats;
	BattleStats defenderStats;

	glm::vec2 cameraStart;
	glm::vec2 battleDirection;

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
	bool capturing = false;
	bool unitCaptured = false;
	bool droppingUnit = false;

	float delayTimer = 0.0f;
	float delay = 0.75f;

	std::vector<Attack> battleQueue;
	std::vector<Attack> accostQueue;

	Subject<Unit*, Unit*> subject;
	Subject<Unit*> unitDiedSubject;

};